#include "settingsdialog.h"

#include "progressitemdelegate.h"
#include "ui_settingsdialog.h"
#include "core/application.h"
#include "core/settings.h"
#include "registry/docsetsregistry.h"

#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QInputDialog>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QProcess>
#include <QTemporaryFile>
#include <QUrl>

#include <QtConcurrent/QtConcurrent>

using namespace Zeal;

namespace {
const char *ApiUrl = "http://api.zealdocs.org";
}

SettingsDialog::SettingsDialog(Core::Application *app, ListModel *listModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog()),
    m_application(app),
    m_zealListModel(listModel)
{
    ui->setupUi(this);

    ui->downloadableGroup->hide();
    ui->docsetsProgress->hide();

    ui->listView->setModel(m_zealListModel);

    ProgressItemDelegate *progressDelegate = new ProgressItemDelegate();
    ui->docsetsList->setItemDelegate(progressDelegate);
    ui->listView->setItemDelegate(progressDelegate);

    // Setup signals & slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::loadSettings);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
            saveSettings();
    });

    connect(ui->minFontSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &SettingsDialog::minFontSizeChanged);

    connect(ui->downloadButton, &QPushButton::clicked, this, &SettingsDialog::downloadDocsetList);
    connect(ui->updateButton, &QPushButton::clicked, this, &SettingsDialog::updateDocsets);

    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::loadSettings()
{
    const Core::Settings * const settings = m_application->settings();
    // General Tab
    ui->startMinimizedCheckBox->setChecked(settings->startMinimized);

    ui->systrayGroupBox->setChecked(settings->showSystrayIcon);
    ui->minimizeToSystrayCheckBox->setChecked(settings->minimizeToSystray);
    ui->hideToSystrayCheckBox->setChecked(settings->hideOnClose);

    ui->toolButton->setKeySequence(settings->showShortcut);

    //
    ui->minFontSize->setValue(settings->minimumFontSize);
    ui->storageEdit->setText(settings->docsetPath);

    // Network Tab
    switch (settings->proxyType) {
    case Core::Settings::ProxyType::None:
        ui->m_noProxySettings->setChecked(true);
        break;
    case Core::Settings::ProxyType::System:
        ui->m_systemProxySettings->setChecked(true);
        break;
    case Core::Settings::ProxyType::UserDefined:
        ui->m_manualProxySettings->setChecked(true);
        ui->m_httpProxy->setText(settings->proxyHost);
        ui->m_httpProxyPort->setValue(settings->proxyPort);
        ui->m_httpProxyNeedsAuth->setChecked(settings->proxyAuthenticate);
        ui->m_httpProxyUser->setText(settings->proxyUserName);
        ui->m_httpProxyPass->setText(settings->proxyPassword);
        break;
    }
}

// creates a total download progress for multiple QNetworkReplies
void SettingsDialog::on_downloadProgress(quint64 received, quint64 total)
{
    // Don't show progress for non-docset pages
    if (received < 10240)
        return;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    // Try to get the item associated to the request
    QListWidgetItem *item = ui->docsetsList->item(reply->property("listItem").toInt());
    QPair<qint32, qint32> *previousProgress = progress[reply];
    if (previousProgress == nullptr) {
        previousProgress = new QPair<qint32, qint32>(0, 0);
        progress[reply] = previousProgress;
    }

    if (item) {
        item->setData(ProgressItemDelegate::ProgressMaxRole, total);
        item->setData(ProgressItemDelegate::ProgressRole, received);
    }

    currentDownload += received - previousProgress->first;
    totalDownload += total - previousProgress->second;
    previousProgress->first = received;
    previousProgress->second = total;
    displayProgress();
}

void SettingsDialog::displayProgress()
{
    ui->docsetsProgress->setValue(currentDownload);
    ui->docsetsProgress->setMaximum(totalDownload);
    ui->docsetsProgress->setVisible(tasksRunning > 0);
}

void SettingsDialog::startTasks(qint8 tasks)
{
    tasksRunning += tasks;
    if (!tasksRunning)
        resetProgress();

    displayProgress();
}

void SettingsDialog::endTasks(qint8 tasks)
{
    startTasks(-tasks);

    if (tasksRunning > 0)
        return;

    // Remove completed items
    for (int i = ui->docsetsList->count() - 1; i >= 0; --i) {
        QListWidgetItem *item = ui->docsetsList->item(i);
        if (item->data(ZealDocsetDoneInstalling).toBool()) {
            item->setCheckState(Qt::Unchecked);
            item->setHidden(true);
            item->setData(ProgressItemDelegate::ProgressVisibleRole, false);
            item->setData(ZealDocsetDoneInstalling, false);
            item->setData(ProgressItemDelegate::ProgressFormatRole, QVariant());
            item->setData(ProgressItemDelegate::ProgressRole, QVariant());
            item->setData(ProgressItemDelegate::ProgressMaxRole, QVariant());
        }
    }
}

void SettingsDialog::updateDocsets()
{
    ui->downloadableGroup->show();
    const QStringList docsetNames = DocsetsRegistry::instance()->names();
    bool missingMetadata = false;
    foreach (const QString &name, docsetNames) {
        const DocsetMetadata metadata = DocsetsRegistry::instance()->meta(name);
        if (!metadata.source().isEmpty())
            missingMetadata = true;

        const QUrl feedUrl = metadata.feedUrl();
        if (feedUrl.isEmpty())
            continue;

        QNetworkReply *reply = startDownload(feedUrl);

        QList<QListWidgetItem *> items = ui->docsetsList->findItems(name, Qt::MatchFixedString);
        if (!items.isEmpty()) {
            reply->setProperty("listItem", ui->docsetsList->row(items[0]));
        } else {
            QListWidgetItem *item = new QListWidgetItem(name, ui->docsetsList);
            item->setCheckState(Qt::Checked);
            item->setHidden(true);
            ui->docsetsList->addItem(item);
            reply->setProperty("listItem", ui->docsetsList->row(item));
        }

        reply->setProperty("metadata", QVariant::fromValue(metadata));
        connect(reply, &QNetworkReply::finished, this, &SettingsDialog::extractDocset);
    }
    if (missingMetadata) {
        int r = QMessageBox::information(this, "Zeal",
                                         "Some docsets are missing metadata, would you like to redownload all docsets with missing metadata?",
                                         QMessageBox::Yes | QMessageBox::No);
        if (r == QMessageBox::Yes) {
            if (!downloadedDocsetsList)
                downloadDocsetList();

            // There must be a better way to do this.
            auto future = QtConcurrent::run([=] {
                while (!downloadedDocsetsList || replies.size())
                    QThread::yieldCurrentThread();
            });
            QFutureWatcher<void> *watcher = new QFutureWatcher<void>;
            watcher->setFuture(future);
            connect(watcher, &QFutureWatcher<void>::finished, [=] {
                foreach (const QString &name, docsetNames) {
                    DocsetMetadata metadata = DocsetsRegistry::instance()->meta(name);
                    if (!metadata.source().isEmpty() && m_availableDocsets.contains(name))
                        downloadDocset(name);
                }
            });
        }
    }
}

void SettingsDialog::processDocsetList(const QJsonArray &list)
{
    for (const QJsonValue &v : list) {
        QJsonObject docsetJson = v.toObject();
        docsetJson[QStringLiteral("source")] = QStringLiteral("kapeli");

        DocsetMetadata metadata(docsetJson);
        m_availableDocsets.insert(metadata.name(), metadata);
    }

    /// TODO: Move into a dedicated method
    for (const DocsetMetadata &metadata : m_availableDocsets) {
        const QIcon icon(QString(QStringLiteral("icons:%1.png")).arg(metadata.icon()));

        QListWidgetItem *listItem = new QListWidgetItem(icon, metadata.title(), ui->docsetsList);
        listItem->setData(ListModel::DocsetNameRole, metadata.name());
        listItem->setCheckState(Qt::Unchecked);

        if (DocsetsRegistry::instance()->names().contains(metadata.name()))
            listItem->setHidden(true);
    }
}

void SettingsDialog::downloadDocset(const QString &name)
{
    static QStringList urls = {
        QStringLiteral("http://sanfrancisco.kapeli.com"),
        QStringLiteral("http://sanfrancisco2.kapeli.com"),
        QStringLiteral("http://london.kapeli.com"),
        QStringLiteral("http://london2.kapeli.com"),
        QStringLiteral("http://london3.kapeli.com"),
        QStringLiteral("http://newyork.kapeli.com"),
        QStringLiteral("http://newyork2.kapeli.com"),
        QStringLiteral("http://sydney.kapeli.com"),
        QStringLiteral("http://tokyo.kapeli.com"),
        QStringLiteral("http://tokyo2.kapeli.com")
    };

    /// TODO: Select fastest mirror
    QNetworkReply *reply = startDownload(QString(QStringLiteral("%1/feeds/%2.tgz"))
                                         .arg(urls.at(qrand() % urls.size()))
                                         .arg(name));

    QList<QListWidgetItem *> items
            = ui->docsetsList->findItems(name, Qt::MatchFixedString);
    if (items.count()) {
        items[0]->setCheckState(Qt::Checked);
        items[0]->setHidden(false);
        reply->setProperty("listItem", ui->docsetsList->row(items[0]));
    } else {
        QListWidgetItem *item = new QListWidgetItem(name, ui->docsetsList);
        item->setCheckState(Qt::Checked);
        ui->docsetsList->addItem(item);
        reply->setProperty("listItem", ui->docsetsList->row(item));
    }

    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::extractDocset);
}

QString SettingsDialog::tarPath() const
{
#ifdef Q_OS_WIN32
    QDir tardir(QCoreApplication::applicationDirPath());
    // quotes required for paths with spaces, like "C:\Program Files"
    return "\"" + tardir.filePath("bsdtar.exe") + "\"";
#else
    return "bsdtar";
#endif
}

void SettingsDialog::extractDocset()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    replies.removeOne(reply);

    if (reply->error() != QNetworkReply::NoError) {
        endTasks();

        if (reply->error() != QNetworkReply::OperationCanceledError)
            QMessageBox::warning(this, "Network Error",
                                 "Failed to retrieve docset: " + reply->errorString());
        return;
    }

    // Try to get the item associated to the request
    QVariant itemId = reply->property("listItem");
    QListWidgetItem *listItem = ui->docsetsList->item(itemId.toInt());
    /// TODO: Use QNetworkRequest::RedirectionTargetAttribute
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302) {
        QUrl url(reply->header(QNetworkRequest::LocationHeader).toUrl());
        if (url.host().isEmpty())
            url.setHost(reply->request().url().host());
        if (url.scheme().isEmpty())
            url.setScheme(reply->request().url().scheme());
        QNetworkReply *reply3 = startDownload(url);
        endTasks();

        reply3->setProperty("listItem", itemId);
        reply3->setProperty("metadata", reply->property("metadata"));
        connect(reply3, &QNetworkReply::finished, this, &SettingsDialog::extractDocset);
    } else {
        if (reply->request().url().path().endsWith("xml")) {
            /*endTasks();
            DocsetMetadata metadata = DocsetMetadata::fromDashFeed(reply->request().url(), reply->readAll());
            DocsetMetadata oldMetadata;

            if (metadata.urls().isEmpty()) {
                QMessageBox::critical(this, "Zeal", "Could not read docset feed!");
            } else {

                QVariant oldMeta = reply->property("metadata");
                if (oldMeta.isValid())
                    oldMetadata = oldMeta.value<DocsetMetadata>();

                if (metadata.version().isEmpty() || oldMetadata.version() != metadata.version()) {
                    metadata.setFeedUrl(reply->request().url().toString());
                    QNetworkReply *reply2 = startDownload(metadata.primaryUrl());

                    if (listItem) {
                        listItem->setHidden(false);

                        listItem->setData(ProgressItemDelegate::ProgressVisibleRole, true);
                        listItem->setData(ProgressItemDelegate::ProgressRole, 0);
                        listItem->setData(ProgressItemDelegate::ProgressMaxRole, 1);
                    }
                    reply2->setProperty("listItem", itemId);
                    reply2->setProperty("metadata", QVariant::fromValue(metadata));
                    connect(reply2, &QNetworkReply::finished, this, &SettingsDialog::extractDocset);
                }
            }*/
        } else if (reply->request().url().path().endsWith(QLatin1Literal("tgz"))) {
            auto dataDir = QDir(m_application->settings()->docsetPath);
            if (!dataDir.exists()) {
                QMessageBox::critical(this, "No docsets directory found",
                                      QString("'%1' directory not found")
                                      .arg(m_application->settings()->docsetPath));
                endTasks();
            } else {
                const QString program = tarPath();
                QTemporaryFile *tmp = new QTemporaryFile;
                tmp->open();
                qint64 read_len = 1;
                const int BUFSIZE = 1024*1024;
                char buf[BUFSIZE];
                while (read_len > 0) {
                    read_len = reply->read(buf, BUFSIZE);
                    if (read_len > 0)
                        tmp->write(buf, read_len);
                }
                tmp->flush();

                QProcess *tar = new QProcess();
                tar->setWorkingDirectory(dataDir.absolutePath());
                QStringList args(QStringLiteral("-zqtf"));
                args.append(tmp->fileName());
                args.append("*docset");
                tar->start(program, args);
                tar->waitForFinished();
                auto line_buf = tar->readLine();
                auto outDir = QString::fromLocal8Bit(line_buf).split("/")[0];

                args = QStringList(QStringLiteral("-zxf"));
                args.append(tmp->fileName());

                DocsetMetadata metadata;
                QVariant metavariant = reply->property("metadata");
                if (metavariant.isValid())
                    metadata = metavariant.value<DocsetMetadata>();

                const QString fileName = reply->request().url().fileName();
                connect(tar, (void (QProcess::*)(int))&QProcess::finished, [=](int) {
                    QString docsetName = fileName;
                    /// TODO: Use QFileInfo::baseName();
                    docsetName.replace(QStringLiteral(".tgz"), QStringLiteral(".docset"));

                    if (outDir != docsetName)
                        QDir(dataDir).rename(outDir, docsetName);

                    // Write metadata about docset
                    metadata.toFile(dataDir.absoluteFilePath(docsetName) + QStringLiteral("/meta.json"));

                    // FIXME C&P (see "FIXME C&P" below)
                    QMetaObject::invokeMethod(DocsetsRegistry::instance(), "addDocset", Qt::BlockingQueuedConnection,
                                              Q_ARG(QString, dataDir.absoluteFilePath(docsetName)));
                    m_zealListModel->resetModulesCounts();
                    emit refreshRequested();
                    ui->listView->reset();
                    for (int i = 0; i < ui->docsetsList->count(); ++i) {
                        if (ui->docsetsList->item(i)->text()+".docset" == docsetName) {
                            listItem->setData(ZealDocsetDoneInstalling, true);
                            listItem->setData(ProgressItemDelegate::ProgressFormatRole, "Done");
                            listItem->setData(ProgressItemDelegate::ProgressRole, 1);
                            listItem->setData(ProgressItemDelegate::ProgressMaxRole, 1);
                            break;
                        }
                    }
                    endTasks();
                    delete tar;
                });
                if (listItem) {
                    listItem->setData(ProgressItemDelegate::ProgressRole, 0);
                    listItem->setData(ProgressItemDelegate::ProgressMaxRole, 0);
                }
                tar->start(program, args);
            }
        }
    }

    // if all enqueued downloads have finished executing
    if (replies.isEmpty())
        resetProgress();

    reply->deleteLater();
}

void SettingsDialog::downloadDocsetList()
{
    downloadedDocsetsList = false;
    ui->downloadButton->hide();
    ui->docsetsList->clear();

    QNetworkReply *reply = startDownload(QUrl(ApiUrl + QStringLiteral("/docsets")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> replyGuard(reply);
        Q_UNUSED(replyGuard);

        replies.removeOne(reply);

        if (reply->error() != QNetworkReply::NoError) {
            endTasks();

            if (reply->error() != QNetworkReply::OperationCanceledError) {
                QMessageBox::warning(this, "No docsets found",
                                     "Failed retrieving list of docsets: " + reply->errorString());
            }
            return;
        }

        QJsonParseError jsonError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, QStringLiteral("Error"),
                                 QStringLiteral("Corrupted docset list: ")
                                 + jsonError.errorString());
            return;
        }

        processDocsetList(jsonDoc.array());

        if (!m_availableDocsets.isEmpty())
            ui->downloadableGroup->show();

        endTasks();

        // if all enqueued downloads have finished executing
        if (replies.isEmpty()) {
            downloadedDocsetsList = ui->docsetsList->count() > 0;
            resetProgress();
        }
    });
}

void SettingsDialog::on_docsetsList_itemSelectionChanged()
{
    ui->downloadDocsetButton->setEnabled(ui->docsetsList->selectedItems().count() > 0);
}

void SettingsDialog::on_downloadDocsetButton_clicked()
{
    if (!replies.isEmpty()) {
        stopDownloads();
        return;
    }

    // Idle run of bsdtar to check if it is available
    QScopedPointer<QProcess> tar(new QProcess());
    tar->start(tarPath());
    if (!tar->waitForFinished()) {
        QMessageBox::critical(this, "bsdtar executable not found",
                              (QString("'%1' executable not found. It is required to allow extracting docsets. ")
                               + QString("Please install it if you want to extract docsets from within Zeal.")).arg(tarPath()));
        stopDownloads();
        return;
    }

    // Find each checked item, and create a NetworkRequest for it.
    for (int i = 0; i < ui->docsetsList->count(); ++i) {
        QListWidgetItem *item = ui->docsetsList->item(i);
        if (item->checkState() != Qt::Checked)
            continue;

        downloadDocset(item->data(ListModel::DocsetNameRole).toString());
    }

    if (replies.count() > 0)
        ui->downloadDocsetButton->setText("Stop downloads");
}

void SettingsDialog::on_storageButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(0, "Open Directory");
    if (!dir.isEmpty())
        ui->storageEdit->setText(dir);

}

void SettingsDialog::on_deleteButton_clicked()
{
    const QString docsetDisplayName = ui->listView->currentIndex().data().toString();
    const int answer
            = QMessageBox::question(this, tr("Remove Docset"),
                                    QString("Do you want to permanently delete the '%1' docset? ")
                                    .arg(docsetDisplayName));
    if (answer == QMessageBox::No)
        return;

    QDir dataDir(m_application->settings()->docsetPath);
    const QString docsetName = ui->listView->currentIndex().data(ListModel::DocsetNameRole).toString();
    m_zealListModel->removeRow(ui->listView->currentIndex().row());
    if (dataDir.exists()) {
        ui->docsetsProgress->show();
        ui->deleteButton->hide();
        startTasks();
        auto future = QtConcurrent::run([=] {
            QDir docsetDir(dataDir);
            bool isDeleted = false;

            if (docsetDir.cd(docsetName) || docsetDir.cd(docsetName + ".docset"))
                isDeleted = docsetDir.removeRecursively();
            if (!isDeleted) {
                QMessageBox::information(nullptr, QString(),
                                         QString("Delete docset %1 failed!").arg(docsetDisplayName));
            }
        });
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>;
        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, [=] {
            endTasks();
            ui->deleteButton->show();

            QList<QListWidgetItem *> items = ui->docsetsList->findItems(docsetName, Qt::MatchExactly);
            if (!items.isEmpty())
                items[0]->setHidden(false);

            watcher->deleteLater();
        });
    }
}

void SettingsDialog::on_listView_clicked(const QModelIndex &index)
{
    Q_UNUSED(index)
    ui->deleteButton->setEnabled(true);
}

void SettingsDialog::resetProgress()
{
    progress.clear();
    totalDownload = 0;
    currentDownload = 0;
    ui->downloadButton->setVisible(!downloadedDocsetsList);
    ui->downloadDocsetButton->setText("Download");
    ui->downloadButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
    ui->addFeedButton->setEnabled(true);
    ui->docsetsList->setEnabled(true);
    displayProgress();
}

QNetworkReply *SettingsDialog::startDownload(const QUrl &url)
{
    startTasks(1);

    QNetworkReply *reply = m_application->download(url);
    connect(reply, &QNetworkReply::downloadProgress, this, &SettingsDialog::on_downloadProgress);
    replies.append(reply);

    if (!replies.isEmpty()) {
        ui->downloadDocsetButton->setText("Stop downloads");
        ui->downloadButton->setEnabled(false);
        ui->updateButton->setEnabled(false);
        ui->addFeedButton->setEnabled(false);
    }

    return reply;
}

void SettingsDialog::stopDownloads()
{
    for (QNetworkReply *reply: replies) {
        // Hide progress bar
        QListWidgetItem *listItem = ui->docsetsList->item(reply->property("listItem").toInt());
        listItem->setData(ProgressItemDelegate::ProgressVisibleRole, false);
        reply->abort();
    }
}

void SettingsDialog::saveSettings()
{
    Core::Settings * const settings = m_application->settings();
    // General Tab
    settings->startMinimized = ui->startMinimizedCheckBox->isChecked();

    settings->showSystrayIcon = ui->systrayGroupBox->isChecked();
    settings->minimizeToSystray = ui->minimizeToSystrayCheckBox->isChecked();
    settings->hideOnClose = ui->hideToSystrayCheckBox->isChecked();

    settings->showShortcut = ui->toolButton->keySequence();

    //
    settings->minimumFontSize = ui->minFontSize->text().toInt();

    if (ui->storageEdit->text() != settings->docsetPath) {
        settings->docsetPath = ui->storageEdit->text();
        DocsetsRegistry::instance()->initialiseDocsets(settings->docsetPath);
        emit refreshRequested();
    }

    // Network Tab
    // Proxy settings
    if (ui->m_noProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::None;
    else if (ui->m_systemProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::System;
    else if (ui->m_manualProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::UserDefined;

    settings->proxyHost = ui->m_httpProxy->text();
    settings->proxyPort = ui->m_httpProxyPort->text().toUInt();
    settings->proxyAuthenticate = ui->m_httpProxyNeedsAuth->isChecked();
    settings->proxyUserName = ui->m_httpProxyUser->text();
    settings->proxyPassword = ui->m_httpProxyPass->text();

    settings->save();

    emit webPageStyleUpdated();
}

void SettingsDialog::on_tabWidget_currentChanged(int current)
{
    if (ui->tabWidget->widget(current) != ui->docsetsTab)
        return;

    // Ensure the list is completely up to date
    QModelIndex index = ui->listView->currentIndex();
    ui->listView->reset();

    if (index.isValid())
        ui->listView->setCurrentIndex(index);

    if (!ui->docsetsList->count())
        downloadDocsetList();
}

void SettingsDialog::on_addFeedButton_clicked()
{
    QString txt = QApplication::clipboard()->text();
    if (!txt.startsWith("dash-feed://"))
        txt.clear();

    QString feedUrl = QInputDialog::getText(this, "Zeal", "Feed URL:", QLineEdit::Normal, txt);
    if (feedUrl.isEmpty())
        return;

    if (feedUrl.startsWith("dash-feed://")) {
        feedUrl = feedUrl.remove(0, 12);
        feedUrl = QUrl::fromPercentEncoding(feedUrl.toUtf8());
    }

    QNetworkReply *reply = startDownload(feedUrl);
    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::extractDocset);
}
