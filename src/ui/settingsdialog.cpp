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
#include <QUrl>

#include <QtConcurrent/QtConcurrent>

#ifdef USE_WEBENGINE
#include <QWebEngineSettings>
#define QWebSettings QWebEngineSettings
#else
#include <QWebSettings>
#endif

using namespace Zeal;

namespace {
const char ApiServerUrl[] = "http://api.zealdocs.org/v1";
const char RedirectServerUrl[] = "http://go.zealdocs.org";
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
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::WindowMaximizeButtonHint;
    setWindowFlags( flags );

#ifdef Q_OS_OSX
    ui->availableDocsetList->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->installedDocsetList->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    ui->downloadableGroup->hide();
    ui->docsetsProgress->hide();

    ui->installedDocsetList->setItemDelegate(new DocsetListItemDelegate(this));
    ui->installedDocsetList->setModel(listModel);

    ui->installedDocsetList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            [this, selectionModel]() {
        if (!m_replies.isEmpty())
            return;

        ui->removeDocsetsButton->setEnabled(selectionModel->hasSelection());

        for (const QModelIndex &index : selectionModel->selectedIndexes()) {
            if (index.data(Zeal::ListModel::UpdateAvailableRole).toBool()) {
                ui->updateSelectedDocsetsButton->setEnabled(true);
                return;
            }
        }
        ui->updateSelectedDocsetsButton->setEnabled(false);
    });
    connect(ui->updateSelectedDocsetsButton, &QPushButton::clicked,
            this, &SettingsDialog::updateSelectedDocsets);
    connect(ui->updateAllDocsetsButton, &QPushButton::clicked,
            this, &SettingsDialog::updateAllDocsets);
    connect(ui->removeDocsetsButton, &QPushButton::clicked,
            this, &SettingsDialog::removeSelectedDocsets);

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
    connect(ui->refreshButton, &QPushButton::clicked, this, &SettingsDialog::downloadDocsetList);

    connect(m_application, &Core::Application::extractionCompleted,
            this, &SettingsDialog::extractionCompleted);
    connect(m_application, &Core::Application::extractionError,
            this, &SettingsDialog::extractionError);
    connect(m_application, &Core::Application::extractionProgress,
            this, &SettingsDialog::extractionProgress);

    loadSettings();
    ui->tabWidget->setCurrentIndex(0);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
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

    QNetworkReply *reply = download(feedUrl);
    reply->setProperty(DownloadTypeProperty, DownloadDashFeed);
    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::downloadCompleted);
}

void SettingsDialog::updateSelectedDocsets()
{
    for (const QModelIndex &index : ui->installedDocsetList->selectionModel()->selectedIndexes()) {
        if (!index.data(Zeal::ListModel::UpdateAvailableRole).toBool())
            continue;

        downloadDashDocset(index.data(Zeal::ListModel::DocsetNameRole).toString());
    }
}

void SettingsDialog::updateAllDocsets()
{
    for (const Docset * const docset : m_docsetRegistry->docsets()) {
        if (!docset->hasUpdate)
            continue;

        downloadDashDocset(docset->name());
    }
}

void SettingsDialog::removeSelectedDocsets()
{
    QItemSelectionModel *selectonModel = ui->installedDocsetList->selectionModel();
    if (!selectonModel->hasSelection())
        return;

    int ret;
    if (selectonModel->selectedIndexes().count() == 1) {
        const QString docsetTitle = selectonModel->selectedIndexes().first().data().toString();
        ret = QMessageBox::question(this, tr("Remove Docset"),
                                    QString(tr("Do you really want to remove <b>%1</b> docset?"))
                                    .arg(docsetTitle));
    } else {
        ret = QMessageBox::question(this, tr("Remove Docsets"),
                                    QString(tr("Do you really want to remove <b>%1</b> docsets?"))
                                    .arg(selectonModel->selectedIndexes().count()));
    }

    if (ret == QMessageBox::No)
        return;

    QStringList names;
    for (const QModelIndex &index : selectonModel->selectedIndexes())
        names << index.data(ListModel::DocsetNameRole).toString();
    removeDocsets(names);
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

    m_replies.removeOne(reply.data());

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            const int ret = QMessageBox::warning(this, tr("Network Error"), reply->errorString(),
                                                 QMessageBox::Ok | QMessageBox::Retry);

            if (ret == QMessageBox::Retry) {
                QNetworkReply *newReply = download(reply->request().url());

                // Copy properties
                newReply->setProperty(DocsetNameProperty, reply->property(DocsetNameProperty));
                newReply->setProperty(DownloadTypeProperty, reply->property(DownloadTypeProperty));
                newReply->setProperty(ListItemIndexProperty,
                                      reply->property(ListItemIndexProperty));

                connect(newReply, &QNetworkReply::finished,
                        this, &SettingsDialog::downloadCompleted);
                return;
            }

            bool ok;
            QListWidgetItem *listItem = ui->availableDocsetList->item(
                        reply->property(ListItemIndexProperty).toInt(&ok));
            if (ok && listItem)
                listItem->setData(ProgressItemDelegate::ShowProgressRole, false);
        }

        if (m_replies.isEmpty())
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

        QNetworkReply *newReply = download(redirectUrl);

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
        DocsetMetadata metadata
                = DocsetMetadata::fromDashFeed(reply->request().url(), reply->readAll());

        if (metadata.urls().isEmpty()) {
            QMessageBox::critical(this, QStringLiteral("Zeal"), tr("Invalid docset feed!"));
            break;
        }

        m_userFeeds[metadata.name()] = metadata;
        QNetworkReply *reply = download(metadata.url());
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
                if (!d.cd(tmpName))
                    return;
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
    if (m_replies.isEmpty())
        resetProgress();
}

// creates a total download progress for multiple QNetworkReplies
void SettingsDialog::downloadProgress(qint64 received, qint64 total)
{
    // Don't show progress for non-docset pages
    if (total == -1 || received < 10240)
        return;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    // Try to get the item associated to the request
    QListWidgetItem *item
            = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
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

void SettingsDialog::on_downloadDocsetButton_clicked()
{
    if (!m_replies.isEmpty()) {
        cancelDownloads();
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
}

void SettingsDialog::on_storageButton_clicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"));
    if (!dir.isEmpty())
        ui->storageEdit->setText(QDir::toNativeSeparators(dir));

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

QListWidgetItem *SettingsDialog::findDocsetListItem(const QString &title) const
{
    const QList<QListWidgetItem *> items
            = ui->availableDocsetList->findItems(title, Qt::MatchFixedString);

    if (items.isEmpty())
        return nullptr;

    return items.first();
}

bool SettingsDialog::updatesAvailable() const
{
    for (Docset *docset : m_docsetRegistry->docsets()) {
        if (docset->hasUpdate)
            return true;
    }

    return false;
}

QNetworkReply *SettingsDialog::download(const QUrl &url)
{
    displayProgress();

    QNetworkReply *reply = m_application->download(url);
    connect(reply, &QNetworkReply::downloadProgress, this, &SettingsDialog::downloadProgress);
    m_replies.append(reply);

    // Installed docsets
    ui->addFeedButton->setEnabled(false);
    ui->updateSelectedDocsetsButton->setEnabled(false);
    ui->updateAllDocsetsButton->setEnabled(false);
    ui->removeDocsetsButton->setEnabled(false);

    // Available docsets
    ui->availableDocsetList->setEnabled(false);
    ui->refreshButton->setEnabled(false);
    ui->downloadDocsetButton->setText(tr("Stop downloads"));

    return reply;
}

void SettingsDialog::cancelDownloads()
{
    for (QNetworkReply *reply : m_replies) {
        // Hide progress bar
        QListWidgetItem *listItem
                = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
        if (!listItem)
            continue;

        listItem->setData(ProgressItemDelegate::ShowProgressRole, false);
        reply->abort();
    }
    resetProgress();
}

void SettingsDialog::downloadDocsetList()
{
    ui->availableDocsetList->clear();
    m_availableDocsets.clear();

    QNetworkReply *reply = download(QUrl(ApiServerUrl + QLatin1String("/docsets")));
    reply->setProperty(DownloadTypeProperty, DownloadDocsetList);
    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::downloadCompleted);
}

void SettingsDialog::processDocsetList(const QJsonArray &list)
{
    for (const QJsonValue &v : list) {
        QJsonObject docsetJson = v.toObject();

        DocsetMetadata metadata(docsetJson);
        m_availableDocsets.insert(metadata.name(), metadata);
    }

    /// TODO: Move into dedicated method
    for (const DocsetMetadata &metadata : m_availableDocsets) {
        QListWidgetItem *listItem
                = new QListWidgetItem(metadata.icon(), metadata.title(), ui->availableDocsetList);
        listItem->setData(ListModel::DocsetNameRole, metadata.name());
        listItem->setCheckState(Qt::Unchecked);

        if (m_docsetRegistry->contains(metadata.name())) {
            listItem->setHidden(true);

            Docset *docset = m_docsetRegistry->docset(metadata.name());

            if (metadata.latestVersion() != docset->version()
                    || (metadata.latestVersion() == docset->version()
                        && metadata.revision() > docset->revision())) {
                docset->hasUpdate = true;
                ui->updateAllDocsetsButton->setEnabled(true);
            }
        }
    }

    ui->installedDocsetList->reset();

    if (!m_availableDocsets.isEmpty())
        ui->downloadableGroup->show();
}

void SettingsDialog::downloadDashDocset(const QString &name)
{
    if (!m_availableDocsets.contains(name))
        return;

    const QString urlString = RedirectServerUrl + QStringLiteral("/d/com.kapeli/%1/latest");
    QNetworkReply *reply = download(urlString.arg(name));
    reply->setProperty(DocsetNameProperty, name);
    reply->setProperty(DownloadTypeProperty, DownloadDocset);
    reply->setProperty(ListItemIndexProperty,
                       ui->availableDocsetList->row(findDocsetListItem(m_availableDocsets[name].title())));

    connect(reply, &QNetworkReply::finished, this, &SettingsDialog::downloadCompleted);
}

void SettingsDialog::removeDocsets(const QStringList &names)
{
    for (const QString &name : names) {
        const QString title = m_docsetRegistry->docset(name)->title();
        m_docsetRegistry->remove(name);

        const QDir dataDir(m_application->settings()->docsetPath);
        if (dataDir.exists()) {
            ui->docsetsProgress->show();
            ui->removeDocsetsButton->setEnabled(false);
            displayProgress();

            QFuture<bool> future = QtConcurrent::run([=] {
                QDir docsetDir(dataDir);
                return docsetDir.cd(name + QLatin1String(".docset"))
                        && docsetDir.removeRecursively();
            });
            QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>();
            watcher->setFuture(future);
            connect(watcher, &QFutureWatcher<void>::finished, [=] {
                if (!watcher->result()) {
                    QMessageBox::warning(this, tr("Error"),
                                         QString(tr("Cannot delete docset <b>%1</b>!")).arg(title));
                }

                resetProgress();

                QListWidgetItem *listItem = findDocsetListItem(title);
                if (listItem)
                    listItem->setHidden(false);

                watcher->deleteLater();
            });
        }
    }
}

void SettingsDialog::displayProgress()
{
    ui->docsetsProgress->setValue(percent(m_combinedReceived, m_combinedTotal));
    ui->docsetsProgress->setMaximum(100);
    ui->docsetsProgress->setVisible(!m_replies.isEmpty());
}

void SettingsDialog::resetProgress()
{
    if (!m_replies.isEmpty())
        return;

    m_combinedReceived = 0;
    m_combinedTotal = 0;
    displayProgress();

    // Installed docsets
    ui->addFeedButton->setEnabled(true);
    QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    bool hasSelectedUpdates = false;
    for (const QModelIndex &index : selectionModel->selectedIndexes()) {
        if (index.data(Zeal::ListModel::UpdateAvailableRole).toBool()) {
            hasSelectedUpdates = true;
            break;
        }
    }
    ui->updateSelectedDocsetsButton->setEnabled(hasSelectedUpdates);
    ui->updateAllDocsetsButton->setEnabled(updatesAvailable());
    ui->removeDocsetsButton->setEnabled(selectionModel->hasSelection());

    // Available docsets
    ui->availableDocsetList->setEnabled(true);
    ui->refreshButton->setEnabled(true);
    ui->downloadDocsetButton->setText(tr("Download"));
}

void SettingsDialog::loadSettings()
{
    const Core::Settings * const settings = m_application->settings();
    // General Tab
    ui->startMinimizedCheckBox->setChecked(settings->startMinimized);
    ui->checkForUpdateCheckBox->setChecked(settings->checkForUpdate);

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

void SettingsDialog::saveSettings()
{
    Core::Settings * const settings = m_application->settings();
    // General Tab
    settings->startMinimized = ui->startMinimizedCheckBox->isChecked();
    settings->checkForUpdate = ui->checkForUpdateCheckBox->isChecked();

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
