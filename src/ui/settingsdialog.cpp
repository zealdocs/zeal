#include "settingsdialog.h"

#include "docsetlistitemdelegate.h"
#include "progressitemdelegate.h"
#include "ui_settingsdialog.h"
#include "core/application.h"
#include "core/settings.h"
#include "registry/docsetregistry.h"
#include "registry/listmodel.h"

#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QInputDialog>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QWebSettings>
#include <QUrl>

#include <QtConcurrent/QtConcurrent>

using namespace Zeal;

namespace {
const char ApiUrl[] = "http://api.zealdocs.org/v1";
/// TODO: Each source plugin should have its own cache
const char DocsetListCacheFileName[] = "com.kapeli.json";

/// TODO: Make the timeout period configurable
constexpr int CacheTimeout = 24 * 60 * 60 * 1000; // 24 hours in microseconds

// QNetworkReply properties
const char DocsetNameProperty[] = "docsetName";
const char DownloadTypeProperty[] = "downloadType";
const char DownloadPreviousReceived[] = "downloadPreviousReceived";
const char ListItemIndexProperty[] = "listItem";
}

SettingsDialog::SettingsDialog(Core::Application *app, ListModel *listModel, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog()),
    m_application(app),
    m_docsetRegistry(app->docsetRegistry())
{
    ui->setupUi(this);

#ifdef Q_OS_OSX
    ui->availableDocsetList->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->installedDocsetList->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    ui->downloadableGroup->hide();
    ui->docsetsProgress->hide();

    ui->installedDocsetList->setItemDelegate(new DocsetListItemDelegate(this));
    ui->installedDocsetList->setModel(listModel);

    ui->availableDocsetList->setItemDelegate(new ProgressItemDelegate(this));

    // Setup signals & slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::loadSettings);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
            saveSettings();
    });

    connect(ui->minFontSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [](int value) {
        QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize, value);
    });

    connect(ui->addFeedButton, &QPushButton::clicked, this, &SettingsDialog::addDashFeed);
    connect(ui->updateButton, &QPushButton::clicked, this, &SettingsDialog::updateDocsets);
    connect(ui->refreshButton, &QPushButton::clicked, this, &SettingsDialog::downloadDocsetList);

    connect(m_application, &Core::Application::extractionCompleted,
            this, &SettingsDialog::extractionCompleted);
    connect(m_application, &Core::Application::extractionError,
            this, &SettingsDialog::extractionError);
    connect(m_application, &Core::Application::extractionProgress,
            this, &SettingsDialog::extractionProgress);

    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::extractionCompleted(const QString &filePath)
{
    QString docsetName;

    /// FIXME: Come up with a better approach
    for (const QString &key : m_tmpFiles.keys()) {
        if (m_tmpFiles[key]->fileName() == filePath) {
            docsetName = key;
            break;
        }
    }

    const QDir dataDir(m_application->settings()->docsetPath);
    const QString docsetPath = dataDir.absoluteFilePath(docsetName + QLatin1String(".docset"));

    // Write metadata about docset
    DocsetMetadata metadata = m_availableDocsets.contains(docsetName)
            ? m_availableDocsets[docsetName]
              : m_userFeeds[docsetName];
    metadata.save(docsetPath, metadata.latestVersion());

    m_docsetRegistry->addDocset(docsetPath);

    QListWidgetItem *listItem = findDocsetListItem(metadata.title());
    if (listItem) {
        listItem->setHidden(true);
        listItem->setCheckState(Qt::Unchecked);
        listItem->setData(ProgressItemDelegate::ShowProgressRole, false);
    }
    resetProgress();
    delete m_tmpFiles.take(docsetName);
}

void SettingsDialog::extractionError(const QString &filePath, const QString &errorString)
{
    const QString docsetName = QFileInfo(filePath).baseName() + QLatin1String(".docset");
    QMessageBox::warning(this, tr("Extraction Error"),
                         QString(tr("Cannot extract docset <b>%1</b>: %2")).arg(docsetName, errorString));
    /// TODO: Update list item state (hide progress bar)
    delete m_tmpFiles.take(docsetName);
}

void SettingsDialog::extractionProgress(const QString &filePath, qint64 extracted, qint64 total)
{
    QString docsetName;

    /// FIXME: Come up with a better approach
    for (const QString &key : m_tmpFiles.keys()) {
        if (m_tmpFiles[key]->fileName() == filePath) {
            docsetName = key;
            break;
        }
    }

    DocsetMetadata metadata = m_availableDocsets.contains(docsetName)
            ? m_availableDocsets[docsetName]
              : m_userFeeds[docsetName];

    QListWidgetItem *listItem = findDocsetListItem(metadata.title());
    if (listItem)
        listItem->setData(ProgressItemDelegate::ValueRole, percent(extracted, total));
}

/*!
  \internal
  Should be connected to all \l QNetworkReply::finished signals in order to process possible
  HTTP-redirects correctly.
*/
void SettingsDialog::downloadCompleted()
{
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(
                qobject_cast<QNetworkReply *>(sender()));

    replies.removeOne(reply.data());

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError)
            QMessageBox::warning(this, tr("Network Error"), reply->errorString());

        if (replies.isEmpty())
            resetProgress();

        return;
    }

    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectUrl.isValid()) {
        if (redirectUrl.isRelative())
            redirectUrl = reply->request().url().resolved(redirectUrl);

        /// TODO: Verify if scheme can be missing
        if (redirectUrl.scheme().isEmpty())
            redirectUrl.setScheme(reply->request().url().scheme());

        QNetworkReply *newReply = startDownload(redirectUrl);

        // Copy properties
        newReply->setProperty(DocsetNameProperty, reply->property(DocsetNameProperty));
        newReply->setProperty(DownloadTypeProperty, reply->property(DownloadTypeProperty));
        newReply->setProperty(ListItemIndexProperty, reply->property(ListItemIndexProperty));

        connect(newReply, &QNetworkReply::finished, this, &SettingsDialog::downloadCompleted);

        return;
    }

    switch (static_cast<DownloadType>(reply->property(DownloadTypeProperty).toUInt())) {
    case DownloadDocsetList: {
        const QByteArray data = reply->readAll();

        const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        QScopedPointer<QFile> file(new QFile(cacheDir.filePath(DocsetListCacheFileName)));
        if (file->open(QIODevice::WriteOnly))
            file->write(data);

        ui->lastUpdatedLabel->setText(QFileInfo(file->fileName())
                                      .lastModified().toString(Qt::SystemLocaleShortDate));

        QJsonParseError jsonError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Corrupted docset list: ") + jsonError.errorString());
            break;
        }

        processDocsetList(jsonDoc.array());
        resetProgress();
        break;
    }

    case DownloadDashFeed: {
        DocsetMetadata metadata = DocsetMetadata::fromDashFeed(reply->request().url(), reply->readAll());

        if (metadata.urls().isEmpty()) {
            QMessageBox::critical(this, QStringLiteral("Zeal"), tr("Invalid docset feed!"));
            break;
        }

        m_userFeeds[metadata.name()] = metadata;
        QNetworkReply *reply = startDownload(metadata.url());
        reply->setProperty(DocsetNameProperty, metadata.name());
        reply->setProperty(DownloadTypeProperty, DownloadDocset);
        connect(reply, &QNetworkReply::finished, this, &SettingsDialog::downloadCompleted);
        break;
    }

    case DownloadDocset: {
        const QString docsetName = reply->property(DocsetNameProperty).toString();
        const DocsetMetadata metadata = m_availableDocsets.contains(docsetName)
                ? m_availableDocsets[docsetName]
                  : m_userFeeds[docsetName];

        /// TODO: Implement an explicit and verbose docset update logic
        QDir dir(m_application->settings()->docsetPath);
        const QString docsetDirName = docsetName + QLatin1String(".docset");
        if (dir.exists(docsetDirName)) {
            m_docsetRegistry->remove(docsetName);
            const QString tmpName = QStringLiteral(".toDelete")
                    + QString::number(QDateTime::currentMSecsSinceEpoch());
            dir.rename(docsetDirName, tmpName);
            QtConcurrent::run([=] {
                QDir d(dir);
                d.cd(tmpName);
                d.removeRecursively();
            });
        }

        QTemporaryFile *tmpFile = new QTemporaryFile();
        tmpFile->open();
        while (reply->bytesAvailable())
            tmpFile->write(reply->read(1024 * 1024)); // Use small chunks
        tmpFile->close();

        QListWidgetItem *item = findDocsetListItem(metadata.title());
        if (item) {
            item->setData(ProgressItemDelegate::ValueRole, 0);
            item->setData(ProgressItemDelegate::FormatRole, tr("Installing: %p%"));
        }

        m_tmpFiles.insert(metadata.name(), tmpFile);
        m_application->extract(tmpFile->fileName(), m_application->settings()->docsetPath,
                               metadata.name() + QLatin1String(".docset"));
        break;
    }
    }

    // If all enqueued downloads have finished executing
    if (replies.isEmpty())
        resetProgress();
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
    ui->storageEdit->setText(QDir::toNativeSeparators(settings->docsetPath));

    // Network Tab
    switch (settings->proxyType) {
    case Core::Settings::ProxyType::None:
        ui->noProxySettings->setChecked(true);
        break;
    case Core::Settings::ProxyType::System:
        ui->systemProxySettings->setChecked(true);
        break;
    case Core::Settings::ProxyType::UserDefined:
        ui->manualProxySettings->setChecked(true);
        ui->httpProxy->setText(settings->proxyHost);
        ui->httpProxyPort->setValue(settings->proxyPort);
        ui->httpProxyNeedsAuth->setChecked(settings->proxyAuthenticate);
        ui->httpProxyUser->setText(settings->proxyUserName);
        ui->httpProxyPass->setText(settings->proxyPassword);
        break;
    }
}

// creates a total download progress for multiple QNetworkReplies
void SettingsDialog::on_downloadProgress(qint64 received, qint64 total)
{
    // Don't show progress for non-docset pages
    if (total == -1 || received < 10240)
        return;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    // Try to get the item associated to the request
    QListWidgetItem *item = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
    if (item)
        item->setData(ProgressItemDelegate::ValueRole, percent(received, total));

    qint64 previousReceived = 0;
    const QVariant previousReceivedVariant = reply->property(DownloadPreviousReceived);
    if (!previousReceivedVariant.isValid())
        m_combinedTotal += total;
    else
        previousReceived = previousReceivedVariant.toLongLong();

    m_combinedReceived += received - previousReceived;
    reply->setProperty(DownloadPreviousReceived, received);

    displayProgress();
}

void SettingsDialog::displayProgress()
{
    ui->docsetsProgress->setValue(percent(m_combinedReceived, m_combinedTotal));
    ui->docsetsProgress->setMaximum(100);
    ui->docsetsProgress->setVisible(!replies.isEmpty());
}

void SettingsDialog::resetProgress()
{
    if (!replies.isEmpty())
        return;

    m_combinedReceived = 0;
    m_combinedTotal = 0;
    displayProgress();

    ui->downloadDocsetButton->setText(tr("Download"));
    ui->refreshButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
    ui->addFeedButton->setEnabled(true);
    ui->availableDocsetList->setEnabled(true);
}

void SettingsDialog::updateDocsets()
{
    for (const Docset * const docset : m_docsetRegistry->docsets()) {
        if (!docset->hasUpdate)
            continue;

        downloadDashDocset(docset->name());
    }
}

void SettingsDialog::processDocsetList(const QJsonArray &list)
{
    for (const QJsonValue &v : list) {
        QJsonObject docsetJson = v.toObject();

        DocsetMetadata metadata(docsetJson);
        m_availableDocsets.insert(metadata.name(), metadata);
    }

    /// TODO: Move into a dedicated method
    for (const DocsetMetadata &metadata : m_availableDocsets) {
        QListWidgetItem *listItem = new QListWidgetItem(metadata.icon(), metadata.title(), ui->availableDocsetList);
        listItem->setData(ListModel::DocsetNameRole, metadata.name());
        listItem->setCheckState(Qt::Unchecked);

        if (m_docsetRegistry->contains(metadata.name())) {
            listItem->setHidden(true);

            Docset *docset = m_docsetRegistry->docset(metadata.name());

            if (metadata.latestVersion() != docset->version()
                    || (metadata.latestVersion() == docset->version()
                        && metadata.revision() > docset->revision())) {
                docset->hasUpdate = true;
                ui->updateButton->setEnabled(true);
            }
        }
    }

    ui->installedDocsetList->reset();

    if (!m_availableDocsets.isEmpty())
        ui->downloadableGroup->show();
}

void SettingsDialog::downloadDashDocset(const QString &name)
{
    /// TODO: Select fastest mirror
    static const QStringList kapeliUrls = {
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

    if (!m_availableDocsets.contains(name))
        return;

    const QUrl url = QString("%1/feeds/%2.tgz")
            .arg(kapeliUrls.at(qrand() % kapeliUrls.size()))
            .arg(name);

    QNetworkReply *reply = startDownload(url);
    reply->setProperty(DocsetNameProperty, name);
    reply->setProperty(DownloadTypeProperty, DownloadDocset);
    reply->setProperty(ListItemIndexProperty,
                       ui->availableDocsetList->row(findDocsetListItem(m_availableDocsets[name].title())));

    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::downloadCompleted);
}

void SettingsDialog::downloadDocsetList()
{
    ui->availableDocsetList->clear();
    m_availableDocsets.clear();

    QNetworkReply *reply = startDownload(QUrl(ApiUrl + QLatin1String("/docsets")));
    reply->setProperty(DownloadTypeProperty, DownloadDocsetList);
    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::downloadCompleted);
}

void SettingsDialog::on_availableDocsetList_itemSelectionChanged()
{
    ui->downloadDocsetButton->setEnabled(ui->availableDocsetList->selectedItems().count() > 0);
}

void SettingsDialog::on_downloadDocsetButton_clicked()
{
    if (!replies.isEmpty()) {
        stopDownloads();
        return;
    }

    // Find each checked item, and create a NetworkRequest for it.
    for (int i = 0; i < ui->availableDocsetList->count(); ++i) {
        QListWidgetItem *item = ui->availableDocsetList->item(i);
        if (item->checkState() != Qt::Checked)
            continue;

        item->setData(ProgressItemDelegate::FormatRole, tr("Downloading: %p%"));
        item->setData(ProgressItemDelegate::ValueRole, 0);
        item->setData(ProgressItemDelegate::ShowProgressRole, true);

        downloadDashDocset(item->data(ListModel::DocsetNameRole).toString());
    }

    if (replies.count() > 0)
        ui->downloadDocsetButton->setText(tr("Stop downloads"));
}

void SettingsDialog::on_storageButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(0, tr("Open Directory"));
    if (!dir.isEmpty())
        ui->storageEdit->setText(QDir::toNativeSeparators(dir));

}

void SettingsDialog::on_deleteButton_clicked()
{
    const QString docsetTitle = ui->installedDocsetList->currentIndex().data().toString();
    const int answer
            = QMessageBox::question(this, tr("Remove Docset"),
                                    QString(tr("Do you really want to remove <b>%1</b> docset?"))
                                    .arg(docsetTitle));
    if (answer == QMessageBox::No)
        return;

    const QDir dataDir(m_application->settings()->docsetPath);
    const QString docsetName = ui->installedDocsetList->currentIndex().data(ListModel::DocsetNameRole).toString();
    m_docsetRegistry->remove(docsetName);
    if (dataDir.exists()) {
        ui->docsetsProgress->show();
        ui->deleteButton->hide();
        displayProgress();

        QFuture<bool> future = QtConcurrent::run([=] {
            QDir docsetDir(dataDir);
            return docsetDir.cd(docsetName + QLatin1String(".docset")) && docsetDir.removeRecursively();
        });
        QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>();
        watcher->setFuture(future);
        connect(watcher, &QFutureWatcher<void>::finished, [=] {
            if (!watcher->result()) {
                QMessageBox::warning(this, tr("Error"),
                                     QString(tr("Cannot delete docset <b>%1</b>!")).arg(docsetTitle));
            }

            resetProgress();
            ui->deleteButton->show();

            QListWidgetItem *listItem = findDocsetListItem(docsetTitle);
            if (listItem)
                listItem->setHidden(false);

            watcher->deleteLater();
        });
    }
}

void SettingsDialog::on_installedDocsetList_clicked(const QModelIndex &index)
{
    Q_UNUSED(index)
    ui->deleteButton->setEnabled(true);
}

QNetworkReply *SettingsDialog::startDownload(const QUrl &url)
{
    displayProgress();

    QNetworkReply *reply = m_application->download(url);
    connect(reply, &QNetworkReply::downloadProgress, this, &SettingsDialog::on_downloadProgress);
    replies.append(reply);

    ui->downloadDocsetButton->setText(tr("Stop downloads"));
    ui->refreshButton->setEnabled(false);
    ui->updateButton->setEnabled(false);
    ui->addFeedButton->setEnabled(false);

    return reply;
}

void SettingsDialog::stopDownloads()
{
    for (QNetworkReply *reply : replies) {
        // Hide progress bar
        QListWidgetItem *listItem = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
        if (!listItem)
            continue;

        listItem->setData(ProgressItemDelegate::ShowProgressRole, false);
        reply->abort();
    }
    resetProgress();
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

    if (QDir::fromNativeSeparators(ui->storageEdit->text()) != settings->docsetPath) {
        settings->docsetPath = QDir::fromNativeSeparators(ui->storageEdit->text());
        m_docsetRegistry->init(settings->docsetPath);
    }

    // Network Tab
    // Proxy settings
    if (ui->noProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::None;
    else if (ui->systemProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::System;
    else if (ui->manualProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::UserDefined;

    settings->proxyHost = ui->httpProxy->text();
    settings->proxyPort = ui->httpProxyPort->text().toUInt();
    settings->proxyAuthenticate = ui->httpProxyNeedsAuth->isChecked();
    settings->proxyUserName = ui->httpProxyUser->text();
    settings->proxyPassword = ui->httpProxyPass->text();

    settings->save();
}

int SettingsDialog::percent(qint64 fraction, qint64 total)
{
    if (!total)
        return 0;

    return fraction / static_cast<double>(total) * 100;
}

void SettingsDialog::on_tabWidget_currentChanged(int current)
{
    if (ui->tabWidget->widget(current) != ui->docsetsTab || ui->availableDocsetList->count())
        return;

    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (!cacheDir.exists())
        QDir().mkpath(cacheDir.absolutePath());
    const QFileInfo fi(cacheDir.filePath(DocsetListCacheFileName));

    if (!fi.exists() || fi.lastModified().msecsTo(QDateTime::currentDateTime()) > CacheTimeout) {
        downloadDocsetList();
        return;
    }

    QScopedPointer<QFile> file(new QFile(fi.filePath()));
    if (!file->open(QIODevice::ReadOnly)) {
        downloadDocsetList();
        return;
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(file->readAll(), &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        downloadDocsetList();
        return;
    }

    /// TODO: Show more user friendly labels, like "5 hours ago"
    ui->lastUpdatedLabel->setText(fi.lastModified().toString(Qt::SystemLocaleShortDate));
    processDocsetList(jsonDoc.array());
}

void SettingsDialog::addDashFeed()
{
    QString txt = QApplication::clipboard()->text();
    if (!txt.startsWith(QLatin1String("dash-feed://")))
        txt.clear();

    QString feedUrl = QInputDialog::getText(this, QStringLiteral("Zeal"), tr("Feed URL:"),
                                            QLineEdit::Normal, txt);
    if (feedUrl.isEmpty())
        return;

    if (feedUrl.startsWith(QLatin1String("dash-feed://"))) {
        feedUrl = feedUrl.remove(0, 12);
        feedUrl = QUrl::fromPercentEncoding(feedUrl.toUtf8());
    }

    QNetworkReply *reply = startDownload(feedUrl);
    reply->setProperty(DownloadTypeProperty, DownloadDashFeed);
    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::downloadCompleted);
}

QListWidgetItem *SettingsDialog::findDocsetListItem(const QString &title) const
{
    const QList<QListWidgetItem *> items
            = ui->availableDocsetList->findItems(title, Qt::MatchFixedString);

    if (items.isEmpty())
        return nullptr;

    return items.first();
}
