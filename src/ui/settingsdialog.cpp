#include "settingsdialog.h"

#include "progressitemdelegate.h"
#include "ui_settingsdialog.h"
#include "core/settings.h"
#include "registry/docsetsregistry.h"

#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QInputDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QProcess>
#include <QTemporaryFile>
#include <QUrl>
#include <QWebElementCollection>
#include <QWebFrame>
#include <QWebSettings>
#include <QWebView>

#include <QtConcurrent/QtConcurrent>

using namespace Zeal;

SettingsDialog::SettingsDialog(Core::Settings *settings, ListModel *listModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog()),
    m_zealListModel(listModel),
    m_settings(settings),
    m_networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    ui->downloadableGroup->hide();
    ui->docsetsProgress->hide();

    ui->listView->setModel(m_zealListModel);

    ProgressItemDelegate *progressDelegate = new ProgressItemDelegate();
    ui->docsetsList->setItemDelegate(progressDelegate);
    ui->listView->setItemDelegate(progressDelegate);

    tasksRunning = 0;
    totalDownload = 0;
    currentDownload = 0;
    downloadedDocsetsList = false;
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::loadSettings()
{
    // General Tab
    ui->startMinimizedCheckBox->setChecked(m_settings->startMinimized);

    ui->systrayGroupBox->setChecked(m_settings->showSystrayIcon);
    ui->minimizeToSystrayCheckBox->setChecked(m_settings->minimizeToSystray);
    ui->hideToSystrayCheckBox->setChecked(m_settings->hideOnClose);

    ui->toolButton->setKeySequence(m_settings->showShortcut);

    //
    ui->minFontSize->setValue(m_settings->minimumFontSize);
    ui->storageEdit->setText(m_settings->docsetPath);

    // Network Tab
    ui->m_noProxySettings->setChecked(false);
    ui->m_systemProxySettings->setChecked(false);
    ui->m_manualProxySettings->setChecked(false);

    switch (m_settings->proxyType) {
    case Core::Settings::ProxyType::None:
        ui->m_noProxySettings->setChecked(true);
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        break;
    case Core::Settings::ProxyType::System:
        ui->m_systemProxySettings->setChecked(true);
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        break;
    case Core::Settings::ProxyType::UserDefined:
        ui->m_manualProxySettings->setChecked(true);
        ui->m_httpProxy->setText(m_settings->proxyHost);
        ui->m_httpProxyPort->setValue(m_settings->proxyPort);
        ui->m_httpProxyUser->setText(m_settings->proxyUserName);
        ui->m_httpProxyPass->setText(m_settings->proxyPassword);
        ui->m_httpProxyNeedsAuth->setChecked(!m_settings->proxyUserName.isEmpty()
                                             || !m_settings->proxyPassword.isEmpty());
        QNetworkProxy::setApplicationProxy(httpProxy());
        break;
    }
}

// creates a total download progress for multiple QNetworkReplies
void SettingsDialog::on_downloadProgress(quint64 recv, quint64 total)
{
    // Don't show progress for non-docset pages
    if (recv < 10240)
        return;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    // Try to get the item associated to the request
    QVariant itemId = reply->property("listItem");
    QListWidgetItem *item = ui->docsetsList->item(itemId.toInt());
    QPair<qint32, qint32> *previousProgress = progress[reply];
    if (previousProgress == nullptr) {
        previousProgress = new QPair<qint32, qint32>(0, 0);
        progress[reply] = previousProgress;
    }

    if (item != NULL) {
        item->setData(ProgressItemDelegate::ProgressMaxRole, total);
        item->setData(ProgressItemDelegate::ProgressRole, recv);
    }
    currentDownload += recv - previousProgress->first;
    totalDownload += total - previousProgress->second;
    previousProgress->first = recv;
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
        }
    }
}

void SettingsDialog::updateDocsets()
{
    ui->downloadableGroup->show();
    const QStringList docsetNames = DocsetsRegistry::instance()->names();
    bool missingMetadata = false;
    foreach (const QString &name, docsetNames) {
        DocsetMetadata metadata = DocsetsRegistry::instance()->meta(name);
        if (!metadata.isValid())
            missingMetadata = true;

        const QString feedUrl = metadata.feedUrl();
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
                downloadDocsetLists();

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
                    if (!metadata.isValid() && availMetadata.contains(name)) {
                        QNetworkReply *reply = startDownload(availMetadata[name]);

                        QList<QListWidgetItem *> items
                                = ui->docsetsList->findItems(name, Qt::MatchFixedString);
                        if (items.count() > 0) {
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
                }
            });
        }
    }
}

void SettingsDialog::downloadDocsetList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    replies.remove(reply);

    if (reply->error() != QNetworkReply::NoError) {
        endTasks();

        if (reply->error() != QNetworkReply::OperationCanceledError)
            QMessageBox::warning(this, "No docsets found",
                                 "Failed retrieving list of docsets: " + reply->errorString());
        return;
    }

    QWebView view;
    view.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
    view.setContent(reply->readAll());
    auto collection = view.page()->mainFrame()->findAllElements(".drowx");
    for (auto drowx : collection) {
        auto anchor = drowx.findFirst("a");
        auto url = anchor.attribute("href");
        auto name_list = url.split("/");
        auto name = name_list[name_list.count()-1].replace(".tgz", QString());
        if (!name.isEmpty()) {
            auto feedUrl = url;
            if (url.contains("feeds")) // TODO: There must be a better way to do this, or a valid list of available docset feeds.
                feedUrl = url.section("/", 0, -2) + "/" + name + ".xml"; // Attempt to generate a docset feed url.

            // urls[name] = feedUrl;
            DocsetMetadata meta;
            meta.setFeedUrl(feedUrl);
            availMetadata[name] = meta;
            auto url_list = url.split("/");
            auto iconfile = url_list[url_list.count()-1].replace(".tgz", ".png");

            auto *lwi = new QListWidgetItem(QIcon(QString("icons:") + iconfile), name);
            lwi->setCheckState(Qt::Unchecked);

            if (DocsetsRegistry::instance()->names().contains(name)) {
                ui->docsetsList->insertItem(0, lwi);
                lwi->setHidden(true);
            } else {
                ui->docsetsList->addItem(lwi);
            }
        }
    }
    if (availMetadata.size() > 0)
        ui->downloadableGroup->show();

    endTasks();

    // if all enqueued downloads have finished executing
    if (replies.isEmpty()) {
        downloadedDocsetsList = ui->docsetsList->count() > 0;
        resetProgress();
    }

    reply->deleteLater();
}

const QString SettingsDialog::tarPath() const
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
    replies.remove(reply);

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
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302) {
        QUrl url(reply->rawHeader("Location"));
        if (url.host().isEmpty())
            url.setHost(reply->request().url().host());
        if (url.scheme().isEmpty())
            url.setScheme(reply->request().url().scheme());
        QNetworkReply *reply3 = startDownload(url, 1);
        endTasks();

        reply3->setProperty("listItem", itemId);
        reply3->setProperty("metadata", reply->property("metadata"));
        connect(reply3, &QNetworkReply::finished, this, &SettingsDialog::extractDocset);
    } else {
        if (reply->request().url().path().endsWith("xml")) {
            endTasks();
            QXmlStreamReader feed(reply->readAll());
            DocsetMetadata metadata;
            DocsetMetadata oldMetadata;
            metadata.read(feed);

            if (!metadata.urlCount()) {
                QMessageBox::critical(this, "Zeal", "Could not read docset feed!");
            } else {

                QVariant oldMeta = reply->property("metadata");
                if (oldMeta.isValid())
                    oldMetadata = oldMeta.value<DocsetMetadata>();

                if (metadata.version().isEmpty() || oldMetadata.version() != metadata.version()) {
                    metadata.setFeedUrl(reply->request().url().toString());
                    QNetworkReply *reply2 = startDownload(metadata.primaryUrl(), 1);

                    if (listItem != NULL) {
                        listItem->setHidden(false);

                        listItem->setData(ProgressItemDelegate::ProgressVisibleRole, true);
                        listItem->setData(ProgressItemDelegate::ProgressRole, 0);
                        listItem->setData(ProgressItemDelegate::ProgressMaxRole, 1);
                    }
                    reply2->setProperty("listItem", itemId);
                    reply2->setProperty("metadata", QVariant::fromValue(metadata));
                    connect(reply2, &QNetworkReply::finished, this, &SettingsDialog::extractDocset);
                }
            }
        } else if (reply->request().url().path().endsWith(QLatin1Literal("tgz"))) {
            auto dataDir = QDir(m_settings->docsetPath);
            if (!dataDir.exists()) {
                QMessageBox::critical(this, "No docsets directory found",
                                      QString("'%1' directory not found").arg(m_settings->docsetPath));
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
                    metadata.write(dataDir.absoluteFilePath(docsetName)+"/meta.json");

                    // FIXME C&P (see "FIXME C&P" below)
                    QMetaObject::invokeMethod(DocsetsRegistry::instance(), "addDocset", Qt::BlockingQueuedConnection,
                                              Q_ARG(QString, dataDir.absoluteFilePath(docsetName)));
                    m_zealListModel->resetModulesCounts();
                    refreshRequested();
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

void SettingsDialog::downloadDocsetLists()
{
    downloadedDocsetsList = false;
    ui->downloadButton->hide();
    ui->docsetsList->clear();
    QNetworkReply *reply = startDownload(QUrl("http://kapeli.com/docset_links"));
    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::downloadDocsetList);
}

void SettingsDialog::on_downloadButton_clicked()
{
    downloadDocsetLists();
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

        QNetworkReply *reply = startDownload(availMetadata[item->text()], 1);
        reply->setProperty("listItem", i);
        connect(reply, &QNetworkReply::finished, this, &SettingsDialog::extractDocset);

        item->setData(ProgressItemDelegate::ProgressVisibleRole, true);
        item->setData(ProgressItemDelegate::ProgressRole, 0);
        item->setData(ProgressItemDelegate::ProgressMaxRole, 1);
        if (reply->url().path().endsWith((".tgz"))) {
            // Dash's docsets don't redirect, so we can start showing progress instantly
            connect(reply, &QNetworkReply::downloadProgress,
                    this, &SettingsDialog::on_downloadProgress);
        }
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

    QDir dataDir(m_settings->docsetPath);
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

            QList<QListWidgetItem *> items = ui->docsetsList->findItems(docsetDisplayName, Qt::MatchExactly);
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

QNetworkReply *SettingsDialog::startDownload(const QUrl &url, qint8 retries)
{
    startTasks(1);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Zeal/" ZEAL_VERSION));

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, &SettingsDialog::on_downloadProgress);
    replies.insert(reply, retries);

    if (!replies.isEmpty()) {
        ui->downloadDocsetButton->setText("Stop downloads");
        ui->downloadButton->setEnabled(false);
        ui->updateButton->setEnabled(false);
        ui->addFeedButton->setEnabled(false);
    }

    return reply;
}

QNetworkReply *SettingsDialog::startDownload(const DocsetMetadata &meta, qint8 retries)
{
    const QUrl url = meta.feedUrl().isEmpty() ? meta.primaryUrl() : meta.feedUrl();

    QNetworkReply *reply = startDownload(url, retries);
    reply->setProperty("metadata", QVariant::fromValue(meta));
    return reply;
}

void SettingsDialog::stopDownloads()
{
    for (QNetworkReply *reply: replies.keys()) {
        // Hide progress bar
        QListWidgetItem *listItem = ui->docsetsList->item(reply->property("listItem").toInt());
        listItem->setData(ProgressItemDelegate::ProgressVisibleRole, false);
        reply->abort();
    }
}

void SettingsDialog::saveSettings()
{
    // General Tab
    m_settings->startMinimized = ui->startMinimizedCheckBox->isChecked();

    m_settings->showSystrayIcon = ui->systrayGroupBox->isChecked();
    m_settings->minimizeToSystray = ui->minimizeToSystrayCheckBox->isChecked();
    m_settings->hideOnClose = ui->hideToSystrayCheckBox->isChecked();

    m_settings->showShortcut = ui->toolButton->keySequence();

    //
    m_settings->minimumFontSize = ui->minFontSize->text().toInt();

    if (ui->storageEdit->text() != m_settings->docsetPath) {
        m_settings->docsetPath = ui->storageEdit->text();
        DocsetsRegistry::instance()->initialiseDocsets(m_settings->docsetPath);
        emit refreshRequested();
    }

    // Network Tab
    // Proxy settings
    if (ui->m_noProxySettings->isChecked())
        m_settings->proxyType = Core::Settings::ProxyType::None;
    else if (ui->m_systemProxySettings->isChecked())
        m_settings->proxyType = Core::Settings::ProxyType::System;
    else if (ui->m_manualProxySettings->isChecked())
        m_settings->proxyType = Core::Settings::ProxyType::UserDefined;

    m_settings->proxyHost = ui->m_httpProxy->text();
    m_settings->proxyPort = ui->m_httpProxyPort->text().toUInt();
    m_settings->proxyAuthenticate = ui->m_httpProxyNeedsAuth->isChecked();
    m_settings->proxyUserName = ui->m_httpProxyUser->text();
    m_settings->proxyPassword = ui->m_httpProxyPass->text();

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
        downloadDocsetLists();
}

void SettingsDialog::showEvent(QShowEvent *)
{
    on_tabWidget_currentChanged(0);
}

void SettingsDialog::on_buttonBox_accepted()
{
    saveSettings();
}

void SettingsDialog::on_minFontSize_valueChanged(int arg1)
{
    minFontSizeChanged(arg1);
}

void SettingsDialog::on_buttonBox_rejected()
{
    loadSettings();
}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
        saveSettings();
}

void SettingsDialog::on_updateButton_clicked()
{
    updateDocsets();
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

QNetworkProxy SettingsDialog::httpProxy() const
{
    QNetworkProxy proxy;

    switch (m_settings->proxyType) {
    case Core::Settings::ProxyType::None:
        proxy = QNetworkProxy::NoProxy;
        break;

    case Core::Settings::ProxyType::System: {
        QNetworkProxyQuery npq(QUrl("http://www.google.com"));
        QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
        if (listOfProxies.size())
            proxy = listOfProxies[0];
        break;
    }

    case Core::Settings::ProxyType::UserDefined:
        proxy = QNetworkProxy(QNetworkProxy::HttpProxy,
                              ui->m_httpProxy->text(), ui->m_httpProxyPort->text().toShort());
        if (ui->m_httpProxyNeedsAuth->isChecked()) {
            proxy.setUser(ui->m_httpProxyUser->text());
            proxy.setPassword(ui->m_httpProxyPass->text());
        }
        break;
    }

    return proxy;
}
