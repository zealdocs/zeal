// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../docsetlistitemdelegate.h"
#include "../docsetsdialog.h"

#include <core/application.h>
#include <core/settings.h>
#include <registry/docset.h>
#include <registry/docsetmetadata.h>
#include <registry/docsetregistry.h>
#include <util/database.h>

#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeView>
#include <QUuid>
#include <QtTest>

#include <archive.h>
#include <archive_entry.h>

#include <memory>

using Zeal::Core::Application;
using Zeal::Registry::DocsetMetadata;
using Zeal::WidgetUi::DocsetListItemDelegate;
using Zeal::WidgetUi::DocsetsDialog;

class DocsetsDialogTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void updateProgress_data();
    void updateProgress();
    void queuedUpdates();
    void failedUpdate();
    void completedUpdate();

private:
    QJsonObject addDocset(const QString &name);
    void openDialog(const QJsonArray &catalog);
    void cancel();
    void serve(QTcpSocket *socket);

    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<QTcpServer> m_server;
    std::unique_ptr<Application> m_application;
    std::unique_ptr<DocsetsDialog> m_dialog;
    QListWidget *m_available = nullptr;
    QList<QTcpSocket *> m_downloads;
    QByteArray m_archive;
    bool m_failDownload = false;
};

void DocsetsDialogTest::init()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("ZealTests"));
    QCoreApplication::setApplicationName(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir->path());
    QSettings settings;
    settings.setValue(QStringLiteral("docsets/path"), m_dir->path());
    settings.setValue(QStringLiteral("proxy/type"), QVariant::fromValue(Zeal::Core::Settings::ProxyType::None));
    settings.sync();

    m_archive = QByteArray(65536, 'x');
    m_failDownload = false;
    m_server = std::make_unique<QTcpServer>();
    QVERIFY(m_server->listen(QHostAddress::LocalHost));
    connect(m_server.get(), &QTcpServer::newConnection, this, [this]() {
        while (m_server->hasPendingConnections()) {
            QTcpSocket *socket = m_server->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                serve(socket);
            });
        }
    });
}

void DocsetsDialogTest::cleanup()
{
    if (m_dialog) {
        m_dialog->reject();
    }
    m_dialog.reset();
    m_application.reset();
    m_server.reset();
    m_downloads.clear();
    QDir(Application::cacheLocation()).removeRecursively();
    QDir(Application::configLocation()).removeRecursively();
    m_dir.reset();
}

QJsonObject DocsetsDialogTest::addDocset(const QString &name)
{
    const QString path = m_dir->filePath(name + QStringLiteral(".docset"));
    if (!QDir().mkpath(path + QStringLiteral("/Contents/Resources/Documents"))) {
        return {};
    }
    QFile plist(path + QStringLiteral("/Contents/Info.plist"));
    if (!plist.open(QIODevice::WriteOnly)) {
        return {};
    }
    plist.write("<plist version=\"1.0\"><dict><key>CFBundleName</key><string>Test</string></dict></plist>");
    plist.close();
    QFile document(path + QStringLiteral("/Contents/Resources/Documents/index.html"));
    if (!document.open(QIODevice::WriteOnly)) {
        return {};
    }
    document.write("<html><body>Test</body></html>");
    document.close();
    Zeal::Util::Database db(path + QStringLiteral("/Contents/Resources/docSet.dsidx"));
    if (!db.isOpen()
        || !db.execute(
            QStringLiteral("CREATE TABLE searchIndex(id INTEGER PRIMARY KEY, name TEXT, type TEXT, path TEXT)"))) {
        return {};
    }
    QJsonObject metadata{{QStringLiteral("name"), name},
                         {QStringLiteral("title"), name},
                         {QStringLiteral("versions"), QJsonArray{QStringLiteral("2")}},
                         {QStringLiteral("feed_url"),
                          QStringLiteral("http://127.0.0.1:%1/%2.xml").arg(m_server->serverPort()).arg(name)}};
    DocsetMetadata(metadata).save(path, QStringLiteral("1"));
    return metadata;
}

void DocsetsDialogTest::serve(QTcpSocket *socket)
{
    QByteArray request = socket->property("request").toByteArray() + socket->readAll();
    socket->setProperty("request", request);
    if (!request.contains("\r\n\r\n") || socket->property("handled").toBool()) {
        return;
    }
    socket->setProperty("handled", true);
    const QByteArray path = request.split(' ').value(1);
    if (path.endsWith(".xml")) {
        const QByteArray body = "<entry><version>2</version><url>http://127.0.0.1:"
                              + QByteArray::number(m_server->serverPort()) + "/archive</url></entry>";
        socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size())
                      + "\r\n\r\n" + body);
        socket->disconnectFromHost();
        return;
    }
    m_downloads.append(socket);
    if (m_failDownload) {
        socket->write("HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
        socket->disconnectFromHost();
        return;
    }
    socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: " + QByteArray::number(m_archive.size())
                  + "\r\n\r\n" + m_archive.left(8192));
    // Leave enough time between chunks for Qt's throttled progress signal.
    QTimer::singleShot(200, socket, [this, socket]() {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(m_archive.mid(8192, 8192));
        }
    });
}

void DocsetsDialogTest::openDialog(const QJsonArray &catalog)
{
    QVERIFY(QDir().mkpath(Application::cacheLocation()));
    QFile cache(Application::cacheLocation() + QStringLiteral("/com.kapeli.json"));
    QVERIFY(cache.open(QIODevice::WriteOnly));
    cache.write(QJsonDocument(catalog).toJson());
    cache.close();
    m_application = std::make_unique<Application>(0);
    QTRY_COMPARE(m_application->docsetRegistry()->storagePath(), m_dir->path());
    m_dialog = std::make_unique<DocsetsDialog>(m_application.get());
    m_available = m_dialog->findChild<QListWidget *>(QStringLiteral("availableDocsetList"));
    QVERIFY(m_available);
    m_dialog->show();
    QTRY_VERIFY(m_dialog->findChild<QPushButton *>(QStringLiteral("updateAllDocsetsButton"))->isEnabled());
    QTRY_VERIFY(m_dialog->findChild<QPushButton *>(QStringLiteral("addFeedButton"))->isEnabled());
}

void DocsetsDialogTest::cancel()
{
    auto *box = m_dialog->findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));
    QVERIFY(box->button(QDialogButtonBox::Cancel));
    box->button(QDialogButtonBox::Cancel)->click();
    QTRY_VERIFY(box->button(QDialogButtonBox::Close));
}

void DocsetsDialogTest::updateProgress_data()
{
    QTest::addColumn<bool>("inCatalog");
    QTest::addColumn<int>("action");
    for (bool inCatalog : {false, true}) {
        for (int action = 0; action < 3; ++action) {
            const QByteArray name = (inCatalog ? QByteArray("catalog-") : QByteArray("custom-feed-"))
                                  + QByteArray::number(action);
            QTest::newRow(name.constData()) << inCatalog << action;
        }
    }
}

void DocsetsDialogTest::updateProgress()
{
    QFETCH(bool, inCatalog);
    QFETCH(int, action);
    const QJsonObject metadata = addDocset(QStringLiteral("Test"));
    QVERIFY(!metadata.isEmpty());
    openDialog(inCatalog ? QJsonArray{metadata} : QJsonArray{});
    QVERIFY(m_dialog);
    auto *filter = m_dialog->findChild<QLineEdit *>(QStringLiteral("docsetFilterInput"));
    filter->setText(QStringLiteral("unrelated"));
    auto *installed = m_dialog->findChild<QTreeView *>(QStringLiteral("installedDocsetList"));
    QCOMPARE(installed->model()->rowCount(), 1);
    const QModelIndex index = installed->model()->index(0, 0);
    installed->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    if (action == 0) {
        m_dialog->findChild<QPushButton *>(QStringLiteral("updateSelectedDocsetsButton"))->click();
    } else if (action == 1) {
        m_dialog->findChild<QPushButton *>(QStringLiteral("updateAllDocsetsButton"))->click();
    } else {
        emit installed->activated(index);
    }
    QTRY_COMPARE(m_downloads.size(), 1);
    QCOMPARE(m_available->count(), 1);
    QListWidgetItem *item = m_available->item(0);
    QVERIFY(m_available->isVisible());
    QVERIFY(!item->isHidden());
    QVERIFY(item->data(DocsetListItemDelegate::ShowProgressRole).toBool());
    QTRY_COMPARE(item->data(DocsetListItemDelegate::ValueRole).toInt(), 25);
    QTest::keyClicks(filter, QStringLiteral("more"));
    QVERIFY(!item->isHidden());
    cancel();
    QVERIFY(item->isHidden());
    QVERIFY(!item->data(DocsetListItemDelegate::ShowProgressRole).toBool());
    QCOMPARE(m_application->docsetRegistry()->docset(QStringLiteral("Test"))->version(), QStringLiteral("1"));
}

void DocsetsDialogTest::queuedUpdates()
{
    QJsonArray catalog;
    for (int i = 0; i < 7; ++i) {
        const QJsonObject metadata = addDocset(QStringLiteral("Test%1").arg(i));
        QVERIFY(!metadata.isEmpty());
        catalog.append(metadata);
    }
    openDialog(catalog);
    m_dialog->findChild<QPushButton *>(QStringLiteral("updateAllDocsetsButton"))->click();
    QTRY_COMPARE(m_downloads.size(), 6);
    QCOMPARE(m_available->count(), 7);
    int queued = 0;
    for (int i = 0; i < m_available->count(); ++i) {
        const QListWidgetItem *item = m_available->item(i);
        QVERIFY(!item->isHidden());
        QVERIFY(item->data(DocsetListItemDelegate::ShowProgressRole).toBool());
        queued += item->data(DocsetListItemDelegate::FormatRole).toString() == QStringLiteral("Queued");
    }
    QCOMPARE(queued, 1);
    cancel();
    for (int i = 0; i < m_available->count(); ++i) {
        QVERIFY(m_available->item(i)->isHidden());
        QVERIFY(!m_available->item(i)->data(DocsetListItemDelegate::ShowProgressRole).toBool());
    }
    QCOMPARE(m_downloads.size(), 6);
}

void DocsetsDialogTest::failedUpdate()
{
    const QJsonObject metadata = addDocset(QStringLiteral("Test"));
    QVERIFY(!metadata.isEmpty());
    openDialog({metadata});
    m_failDownload = true;
    QTimer dismiss;
    int errors = 0;
    connect(&dismiss, &QTimer::timeout, this, [&errors]() {
        auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (box) {
            ++errors;
            box->button(QMessageBox::Cancel)->click();
        }
    });
    dismiss.start(10);
    m_dialog->findChild<QPushButton *>(QStringLiteral("updateAllDocsetsButton"))->click();
    QTRY_COMPARE(errors, 1);
    QTRY_VERIFY(m_dialog->findChild<QPushButton *>(QStringLiteral("updateAllDocsetsButton"))->isEnabled());
    QVERIFY(m_available->item(0)->isHidden());
    QVERIFY(!m_available->item(0)->data(DocsetListItemDelegate::ShowProgressRole).toBool());
    QCOMPARE(m_application->docsetRegistry()->docset(QStringLiteral("Test"))->version(), QStringLiteral("1"));
}

void DocsetsDialogTest::completedUpdate()
{
    const QJsonObject metadata = addDocset(QStringLiteral("Test"));
    QVERIFY(!metadata.isEmpty());
    const QString archivePath = m_dir->filePath(QStringLiteral("update.tar"));
    archive *writer = archive_write_new();
    archive_write_set_format_ustar(writer);
    QCOMPARE(archive_write_open_filename(writer, archivePath.toLocal8Bit().constData()), ARCHIVE_OK);
    for (const QString &relative : {QStringLiteral("Contents/Info.plist"),
                                    QStringLiteral("Contents/Resources/docSet.dsidx"),
                                    QStringLiteral("Contents/Resources/Documents/index.html")}) {
        QFile file(m_dir->filePath(QStringLiteral("Test.docset/") + relative));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray data = file.readAll();
        archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, (QStringLiteral("Test.docset/") + relative).toUtf8().constData());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, data.size());
        QCOMPARE(archive_write_header(writer, entry), ARCHIVE_OK);
        QCOMPARE(archive_write_data(writer, data.constData(), data.size()), data.size());
        archive_entry_free(entry);
    }
    QCOMPARE(archive_write_close(writer), ARCHIVE_OK);
    archive_write_free(writer);
    QFile archiveFile(archivePath);
    QVERIFY(archiveFile.open(QIODevice::ReadOnly));
    m_archive = archiveFile.readAll();
    m_archive.append(QByteArray(65536, '\0'));
    openDialog({metadata});
    bool visibleWhileInstalling = false;
    connect(m_application->docsetRegistry(),
            &Zeal::Registry::DocsetRegistry::docsetUnloaded,
            this,
            [this, &visibleWhileInstalling]() {
        const QListWidgetItem *item = m_available->item(0);
        visibleWhileInstalling = m_available->isVisible() && !item->isHidden()
                              && item->data(DocsetListItemDelegate::ShowProgressRole).toBool()
                              && item->data(DocsetListItemDelegate::FormatRole).toString()
                                     == QStringLiteral("Installing: %p%");
    });
    m_dialog->findChild<QPushButton *>(QStringLiteral("updateAllDocsetsButton"))->click();
    QTRY_COMPARE(m_downloads.size(), 1);
    QVERIFY(m_available->isVisible());
    QVERIFY(!m_available->item(0)->isHidden());
    QTRY_VERIFY(m_available->item(0)->data(DocsetListItemDelegate::ValueRole).toInt() > 0);
    m_downloads.first()->write(m_archive.mid(16384));
    m_downloads.first()->disconnectFromHost();
    QTRY_VERIFY(m_application->docsetRegistry()->contains(QStringLiteral("Test"))
                && m_application->docsetRegistry()->docset(QStringLiteral("Test"))->version() == QStringLiteral("2"));
    QVERIFY(visibleWhileInstalling);
    QVERIFY(m_available->item(0)->isHidden());
    QVERIFY(!m_available->item(0)->data(DocsetListItemDelegate::ShowProgressRole).toBool());
}

QTEST_MAIN(DocsetsDialogTest)
#include "docsetsdialog_test.moc"
