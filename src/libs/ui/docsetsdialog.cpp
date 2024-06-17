/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "docsetsdialog.h"
#include "ui_docsetsdialog.h"

#include "docsetlistitemdelegate.h"

#include <core/application.h>
#include <core/filemanager.h>
#include <core/settings.h>
#include <registry/docset.h>
#include <registry/docsetregistry.h>
#include <registry/itemdatarole.h>

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryFile>
#include <QUrl>

#include <memory>

using namespace Zeal;
using namespace Zeal::WidgetUi;

#ifdef Q_OS_WIN32
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

namespace {
constexpr char ApiServerUrl[] = "https://api.zealdocs.org/v1";
constexpr char RedirectServerUrl[] = "https://go.zealdocs.org/d/%1/%2/latest";
// TODO: Each source plugin should have its own cache
constexpr char DocsetListCacheFileName[] = "com.kapeli.json";

// TODO: Make the timeout period configurable
constexpr int CacheTimeout = 24 * 60 * 60 * 1000; // 24 hours in microseconds

// QNetworkReply properties
constexpr char DocsetNameProperty[] = "docsetName";
constexpr char DownloadTypeProperty[] = "downloadType";
constexpr char ListItemIndexProperty[] = "listItem";
} // namespace

DocsetsDialog::DocsetsDialog(Core::Application *app, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DocsetsDialog())
    , m_application(app)
    , m_docsetRegistry(app->docsetRegistry())
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    loadDocsetList();

    m_isStorageReadOnly = !isDirWritable(m_application->settings()->docsetPath);

#ifdef Q_OS_MACOS
    ui->availableDocsetList->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->installedDocsetList->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    ui->statusLabel->clear(); // Clear text shown in the designer mode.
    ui->storageStatusLabel->setVisible(m_isStorageReadOnly);

    const QFileInfo fi(m_application->settings()->docsetPath);
    ui->storageStatusLabel->setText(fi.exists() ? tr("<b>Docset storage is read only.</b>")
                                                : tr("<b>Docset storage does not exist.</b>"));

    connect(m_application, &Core::Application::extractionCompleted,
            this, &DocsetsDialog::extractionCompleted);
    connect(m_application, &Core::Application::extractionError,
            this, &DocsetsDialog::extractionError);
    connect(m_application, &Core::Application::extractionProgress,
            this, &DocsetsDialog::extractionProgress);

    // Setup signals & slots
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Cancel)) {
            cancelDownloads();
            return;
        }

        if (button == ui->buttonBox->button(QDialogButtonBox::Close)) {
            close();
            return;
        }
    });

    setupInstalledDocsetsTab();
    setupAvailableDocsetsTab();

    if (m_isStorageReadOnly) {
        disableControls();
    }
}

DocsetsDialog::~DocsetsDialog()
{
    delete ui;
}

void DocsetsDialog::addDashFeed()
{
    QString clipboardText = QApplication::clipboard()->text();
    if (!clipboardText.startsWith(QLatin1String("dash-feed://")))
        clipboardText.clear();

    QString feedUrl = QInputDialog::getText(this, QStringLiteral("Zeal"), tr("Feed URL:"),
                                            QLineEdit::Normal, clipboardText);
    feedUrl = feedUrl.trimmed();
    if (feedUrl.isEmpty())
        return;

    if (feedUrl.startsWith(QLatin1String("dash-feed://"))) {
        feedUrl = feedUrl.remove(0, 12);
        feedUrl = QUrl::fromPercentEncoding(feedUrl.toUtf8());
    }

    QNetworkReply *reply = download(QUrl(feedUrl));
    reply->setProperty(DownloadTypeProperty, DownloadDashFeed);
}

void DocsetsDialog::updateSelectedDocsets()
{
    const auto selectedRows = ui->installedDocsetList->selectionModel()->selectedRows();
    for (const QModelIndex &index : selectedRows) {
        if (!index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool())
            continue;

        downloadDashDocset(index);
    }
}

void DocsetsDialog::updateAllDocsets()
{
    QAbstractItemModel *model = ui->installedDocsetList->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        if (!index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool())
            continue;

        downloadDashDocset(index);
    }
}

void DocsetsDialog::removeSelectedDocsets()
{
    QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    if (!selectionModel->hasSelection())
        return;

    int ret;

    const QModelIndexList selectedIndexes = selectionModel->selectedRows();
    if (selectedIndexes.size() == 1) {
        const QString docsetTitle = selectedIndexes.first().data().toString();
        ret = QMessageBox::question(this, QStringLiteral("Zeal"),
                                    tr("Remove <b>%1</b> docset?").arg(docsetTitle));
    } else {
        ret = QMessageBox::question(this, QStringLiteral("Zeal"),
                                    tr("Remove <b>%n</b> docset(s)?", nullptr,
                                       selectedIndexes.size()));
    }

    if (ret == QMessageBox::No) {
        return;
    }

    // Gather names first, because model indicies become invalid when docsets are removed.
    QStringList names;
    for (const QModelIndex &index : selectedIndexes) {
        names.append(index.data(Registry::ItemDataRole::DocsetNameRole).toString());
    }

    for (const QString &name : names) {
        removeDocset(name);
    }
}

void DocsetsDialog::updateDocsetFilter(const QString &filterString)
{
    const bool doSearch = !filterString.simplified().isEmpty();

    for (int i = 0; i < ui->availableDocsetList->count(); ++i) {
        QListWidgetItem *item = ui->availableDocsetList->item(i);

        // Skip installed docsets
        if (m_docsetRegistry->contains(item->data(Registry::ItemDataRole::DocsetNameRole).toString()))
            continue;

        item->setHidden(doSearch && !item->text().contains(filterString, Qt::CaseInsensitive));
    }
}

void DocsetsDialog::downloadSelectedDocsets()
{
    QItemSelectionModel *selectionModel = ui->availableDocsetList->selectionModel();
    const auto selectedRows = selectionModel->selectedRows();
    for (const QModelIndex &index : selectedRows) {
        selectionModel->select(index, QItemSelectionModel::Deselect);

        // Do nothing if a download is already in progress.
        if (index.data(DocsetListItemDelegate::ShowProgressRole).toBool())
            continue;

        QAbstractItemModel *model = ui->availableDocsetList->model();
        model->setData(index, tr("Downloading: %p%"), DocsetListItemDelegate::FormatRole);
        model->setData(index, 0, DocsetListItemDelegate::ValueRole);
        model->setData(index, true, DocsetListItemDelegate::ShowProgressRole);

        downloadDashDocset(index);
    }
}

/*!
  \internal
  Should be connected to all \l QNetworkReply::finished signals in order to process possible
  HTTP-redirects correctly.
*/
void DocsetsDialog::downloadCompleted()
{
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(
                qobject_cast<QNetworkReply *>(sender()));

    m_replies.removeOne(reply.data());

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            const QString msg = tr("Download failed!<br><br><b>Error:</b> %1<br><b>URL:</b> %2")
                    .arg(reply->errorString(), reply->request().url().toString());
            const int ret = QMessageBox::warning(this, QStringLiteral("Zeal"), msg,
                                                 QMessageBox::Retry | QMessageBox::Cancel);

            if (ret == QMessageBox::Retry) {
                QNetworkReply *newReply = download(reply->request().url());

                // Copy properties
                newReply->setProperty(DocsetNameProperty, reply->property(DocsetNameProperty));
                newReply->setProperty(DownloadTypeProperty, reply->property(DownloadTypeProperty));
                newReply->setProperty(ListItemIndexProperty,
                                      reply->property(ListItemIndexProperty));
                return;
            }

            bool ok;
            QListWidgetItem *listItem = ui->availableDocsetList->item(
                        reply->property(ListItemIndexProperty).toInt(&ok));
            if (ok && listItem)
                listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);
        }

        updateStatus();
        return;
    }

    switch (reply->property(DownloadTypeProperty).toUInt()) {
    case DownloadDocsetList: {
        const QByteArray data = reply->readAll();

        QScopedPointer<QFile> file(new QFile(cacheLocation(DocsetListCacheFileName)));
        if (file->open(QIODevice::WriteOnly))
            file->write(data);

        const QString updateTime = QFileInfo(file->fileName())
                .lastModified().toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
        ui->lastUpdatedLabel->setText(updateTime);

        QJsonParseError jsonError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            // TODO: Log QJsonParseError.
            const QMessageBox::StandardButton rc
                    = QMessageBox::warning(this, QStringLiteral("Zeal"),
                                           tr("Server returned a corrupted docset list."),
                                           QMessageBox::Retry | QMessageBox::Cancel);

            if (rc == QMessageBox::Retry) {
                downloadDocsetList();
            }

            break;
        }

        processDocsetList(jsonDoc.array());
        break;
    }

    case DownloadDashFeed: {
        Registry::DocsetMetadata metadata
                = Registry::DocsetMetadata::fromDashFeed(reply->request().url(), reply->readAll());

        if (metadata.urls().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Zeal"), tr("Invalid docset feed!"));
            break;
        }

        m_userFeeds[metadata.name()] = metadata;
        Registry::Docset *docset = m_docsetRegistry->docset(metadata.name());
        if (docset == nullptr) {
            // Fetch docset only on first feed download,
            // since further downloads are only update checks
            QNetworkReply *reply = download(metadata.url());
            reply->setProperty(DocsetNameProperty, metadata.name());
            reply->setProperty(DownloadTypeProperty, DownloadDocset);
        } else {
            // Check for feed update
            if (metadata.latestVersion() != docset->version()
                    || metadata.revision() > docset->revision()) {
                docset->hasUpdate = true;

                if (!m_isStorageReadOnly) {
                    ui->updateAllDocsetsButton->setEnabled(true);
                }

                ui->installedDocsetList->reset();
            }
        }

        break;
    }

    case DownloadDocset: {
        const QString docsetName = reply->property(DocsetNameProperty).toString();
        const QString docsetDirectoryName = docsetName + QLatin1String(".docset");

        if (QDir(m_application->settings()->docsetPath).exists(docsetDirectoryName)) {
            removeDocset(docsetName);
        }

        QTemporaryFile *tmpFile = m_tmpFiles[docsetName];
        if (!tmpFile) {
            tmpFile = new QTemporaryFile(QStringLiteral("%1/%2.XXXXXX.tmp").arg(Core::Application::cacheLocation(), docsetName), this);
            tmpFile->open();
            m_tmpFiles.insert(docsetName, tmpFile);
        }

        while (reply->bytesAvailable()) {
            tmpFile->write(reply->read(1024 * 1024)); // Use small chunks.
        }

        tmpFile->close();

        QListWidgetItem *item = findDocsetListItem(docsetName);
        if (item) {
            item->setData(DocsetListItemDelegate::ValueRole, 0);
            item->setData(DocsetListItemDelegate::FormatRole, tr("Installing: %p%"));
        }

        m_application->extract(tmpFile->fileName(), m_application->settings()->docsetPath,
                               docsetDirectoryName);
        break;
    }
    }

    // If all enqueued downloads have finished executing.
    updateStatus();
}

// creates a total download progress for multiple QNetworkReplies
void DocsetsDialog::downloadProgress(qint64 received, qint64 total)
{
    // Don't show progress for non-docset pages
    if (total == -1 || received < 10240)
        return;

    auto reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply || !reply->isOpen())
        return;

    if (reply->property(DownloadTypeProperty).toInt() == DownloadDocset) {
        const QString docsetName = reply->property(DocsetNameProperty).toString();

        QTemporaryFile *tmpFile = m_tmpFiles[docsetName];
        if (!tmpFile) {
            tmpFile = new QTemporaryFile(QStringLiteral("%1/%2.XXXXXX.tmp").arg(Core::Application::cacheLocation(), docsetName), this);
            tmpFile->open();
            m_tmpFiles.insert(docsetName, tmpFile);
        }

        tmpFile->write(reply->read(received));
    }

    // Try to get the item associated to the request
    QListWidgetItem *item
            = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
    if (item) {
        item->setData(DocsetListItemDelegate::ValueRole, percent(received, total));
    }
}

void DocsetsDialog::extractionCompleted(const QString &filePath)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);

    const QDir dataDir(m_application->settings()->docsetPath);
    const QString docsetPath = dataDir.filePath(docsetName + QLatin1String(".docset"));

    // Write metadata about docset
    Registry::DocsetMetadata metadata = m_availableDocsets.count(docsetName)
            ? m_availableDocsets[docsetName] : m_userFeeds[docsetName];
    metadata.save(docsetPath, metadata.latestVersion());

    m_docsetRegistry->loadDocset(docsetPath);

    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem) {
        listItem->setHidden(true);
        listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);
    }

    delete m_tmpFiles.take(docsetName);

    updateStatus();
}

void DocsetsDialog::extractionError(const QString &filePath, const QString &errorString)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);

    QMessageBox::warning(this, QStringLiteral("Zeal"),
                         tr("Cannot extract docset <b>%1</b>: %2").arg(docsetName, errorString));

    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem)
        listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);

    delete m_tmpFiles.take(docsetName);
}

void DocsetsDialog::extractionProgress(const QString &filePath, qint64 extracted, qint64 total)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);
    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem)
        listItem->setData(DocsetListItemDelegate::ValueRole, percent(extracted, total));
}

void DocsetsDialog::loadDocsetList()
{
    loadUserFeedList();

    const QFileInfo fi(cacheLocation(DocsetListCacheFileName));
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

    // TODO: Show more user friendly labels, like "5 hours ago"
    const QString updateTime
            = fi.lastModified().toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    ui->lastUpdatedLabel->setText(updateTime);
    processDocsetList(jsonDoc.array());
}

void DocsetsDialog::setupInstalledDocsetsTab()
{
    ui->installedDocsetList->setItemDelegate(new DocsetListItemDelegate(this));
    ui->installedDocsetList->setModel(m_docsetRegistry->model());

    ui->installedDocsetList->setItemsExpandable(false);
    ui->installedDocsetList->setRootIsDecorated(false);

    ui->installedDocsetList->header()->setStretchLastSection(true);
    ui->installedDocsetList->header()->setSectionsMovable(false);
    ui->installedDocsetList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->installedDocsetList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    if (m_isStorageReadOnly) {
        return;
    }

    connect(ui->installedDocsetList, &QTreeView::activated, this, [this](const QModelIndex &index) {
        if (!index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
            return;
        }

        downloadDashDocset(index);
    });

    QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, [this, selectionModel]() {
        ui->removeDocsetsButton->setEnabled(selectionModel->hasSelection());

        const auto selectedRows = selectionModel->selectedRows();
        for (const QModelIndex &index : selectedRows) {
            if (index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
                ui->updateSelectedDocsetsButton->setEnabled(true);
                return;
            }
        }

        ui->updateSelectedDocsetsButton->setEnabled(false);
    });

    connect(ui->addFeedButton, &QPushButton::clicked, this, &DocsetsDialog::addDashFeed);

    connect(ui->updateSelectedDocsetsButton, &QPushButton::clicked,
            this, &DocsetsDialog::updateSelectedDocsets);
    connect(ui->updateAllDocsetsButton, &QPushButton::clicked,
            this, &DocsetsDialog::updateAllDocsets);
    connect(ui->removeDocsetsButton, &QPushButton::clicked,
            this, &DocsetsDialog::removeSelectedDocsets);
}

void DocsetsDialog::setupAvailableDocsetsTab()
{
    using Registry::DocsetRegistry;

    ui->availableDocsetList->setItemDelegate(new DocsetListItemDelegate(this));

    connect(m_docsetRegistry, &DocsetRegistry::docsetUnloaded, this, [this](const QString &name) {
        QListWidgetItem *item = findDocsetListItem(name);
        if (!item)
            return;

        item->setHidden(false);
    });
    connect(m_docsetRegistry, &DocsetRegistry::docsetLoaded, this, [this](const QString &name) {
        QListWidgetItem *item = findDocsetListItem(name);
        if (!item)
            return;

        item->setHidden(true);
    });

    connect(ui->refreshButton, &QPushButton::clicked, this, &DocsetsDialog::downloadDocsetList);

    connect(ui->docsetFilterInput, &QLineEdit::textEdited,
            this, &DocsetsDialog::updateDocsetFilter);

    if (m_isStorageReadOnly) {
        return;
    }

    connect(ui->availableDocsetList, &QListView::activated, this, [this](const QModelIndex &index) {
        // TODO: Cancel download if it's already in progress.
        if (index.data(DocsetListItemDelegate::ShowProgressRole).toBool())
            return;

        ui->availableDocsetList->selectionModel()->select(index, QItemSelectionModel::Deselect);

        QAbstractItemModel *model = ui->availableDocsetList->model();
        model->setData(index, tr("Downloading: %p%"), DocsetListItemDelegate::FormatRole);
        model->setData(index, 0, DocsetListItemDelegate::ValueRole);
        model->setData(index, true, DocsetListItemDelegate::ShowProgressRole);

        downloadDashDocset(index);
    });

    QItemSelectionModel *selectionModel = ui->availableDocsetList->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, [this, selectionModel]() {
        const auto selectedRows = selectionModel->selectedRows();
        for (const QModelIndex &index : selectedRows) {
            if (!index.data(DocsetListItemDelegate::ShowProgressRole).toBool()) {
                ui->downloadDocsetsButton->setEnabled(true);
                return;
            }
        }

        ui->downloadDocsetsButton->setEnabled(false);
    });

    connect(ui->downloadDocsetsButton, &QPushButton::clicked,
            this, &DocsetsDialog::downloadSelectedDocsets);
}

void DocsetsDialog::enableControls()
{
    if (m_isStorageReadOnly || !m_replies.isEmpty() || !m_tmpFiles.isEmpty()) {
        return;
    }

    // Dialog buttons.
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Close);

    // Available docsets
    ui->refreshButton->setEnabled(true);

    // Installed docsets
    ui->addFeedButton->setEnabled(true);
    QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    bool hasSelectedUpdates = false;
    const auto selectedRows = selectionModel->selectedRows();
    for (const QModelIndex &index : selectedRows) {
        if (index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
            hasSelectedUpdates = true;
            break;
        }
    }

    ui->updateSelectedDocsetsButton->setEnabled(hasSelectedUpdates);
    ui->updateAllDocsetsButton->setEnabled(updatesAvailable());
    ui->removeDocsetsButton->setEnabled(selectionModel->hasSelection());
}

void DocsetsDialog::disableControls()
{
    // Dialog buttons.
    if (!m_isStorageReadOnly) {
        // Always show the close button if storage is read only.
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    }

    // Installed docsets
    ui->addFeedButton->setEnabled(false);
    ui->updateSelectedDocsetsButton->setEnabled(false);
    ui->updateAllDocsetsButton->setEnabled(false);
    ui->downloadDocsetsButton->setEnabled(false);
    ui->removeDocsetsButton->setEnabled(false);

    // Available docsets
    ui->refreshButton->setEnabled(false);
}

QListWidgetItem *DocsetsDialog::findDocsetListItem(const QString &name) const
{
    for (int i = 0; i < ui->availableDocsetList->count(); ++i) {
        QListWidgetItem *item = ui->availableDocsetList->item(i);

        if (item->data(Registry::ItemDataRole::DocsetNameRole).toString() == name)
            return item;
    }

    return nullptr;
}

bool DocsetsDialog::updatesAvailable() const
{
    const auto docsets = m_docsetRegistry->docsets();
    for (Registry::Docset *docset : docsets) {
        if (docset->hasUpdate)
            return true;
    }

    return false;
}

QNetworkReply *DocsetsDialog::download(const QUrl &url)
{
    QNetworkReply *reply = m_application->download(url);
    connect(reply, &QNetworkReply::downloadProgress, this, &DocsetsDialog::downloadProgress);
    connect(reply, &QNetworkReply::finished, this, &DocsetsDialog::downloadCompleted);
    m_replies.append(reply);

    disableControls();
    updateStatus();

    return reply;
}

void DocsetsDialog::cancelDownloads()
{
    for (QNetworkReply *reply : std::as_const(m_replies)) {
        // Hide progress bar
        QListWidgetItem *listItem
                = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
        if (listItem)
            listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);

        if (reply->property(DownloadTypeProperty).toInt() == DownloadDocset)
            delete m_tmpFiles.take(reply->property(DocsetNameProperty).toString());

        reply->abort();
    }

    updateStatus();
}

void DocsetsDialog::loadUserFeedList()
{
    const auto docsets = m_docsetRegistry->docsets();
    for (Registry::Docset *docset : docsets) {
        if (!docset->feedUrl().isEmpty()) {
            QNetworkReply *reply = download(QUrl(docset->feedUrl()));
            reply->setProperty(DownloadTypeProperty, DownloadDashFeed);
        }
    }
}

void DocsetsDialog::downloadDocsetList()
{
    ui->availableDocsetList->clear();
    m_availableDocsets.clear();

    QNetworkReply *reply = download(QUrl(ApiServerUrl + QLatin1String("/docsets")));
    reply->setProperty(DownloadTypeProperty, DownloadDocsetList);
}

void DocsetsDialog::processDocsetList(const QJsonArray &list)
{
    for (const QJsonValue &v : list) {
        QJsonObject docsetJson = v.toObject();

        Registry::DocsetMetadata metadata(docsetJson);
        m_availableDocsets.insert({metadata.name(), metadata});
    }

    // TODO: Move into dedicated method
    for (const auto &kv : m_availableDocsets) {
        const auto &metadata = kv.second;

        auto listItem = new QListWidgetItem(metadata.icon(), metadata.title(), ui->availableDocsetList);
        listItem->setData(Registry::ItemDataRole::DocsetNameRole, metadata.name());

        if (!m_docsetRegistry->contains(metadata.name())) {
            continue;
        }

        listItem->setHidden(true);

        Registry::Docset *docset = m_docsetRegistry->docset(metadata.name());

        if (metadata.latestVersion() != docset->version()
                || metadata.revision() > docset->revision()) {
            docset->hasUpdate = true;

            if (!m_isStorageReadOnly) {
                ui->updateAllDocsetsButton->setEnabled(true);
            }
        }
    }

    ui->installedDocsetList->reset();
}

void DocsetsDialog::downloadDashDocset(const QModelIndex &index)
{
    const QString name = index.data(Registry::ItemDataRole::DocsetNameRole).toString();

    if (m_availableDocsets.count(name) == 0 && !m_userFeeds.contains(name))
        return;

    QUrl url;
    if (!m_userFeeds.contains(name)) {
        // No feed present means that this is a Kapeli docset
        QString urlString = QString(RedirectServerUrl).arg("com.kapeli").arg(name);
        url = QUrl(urlString);
    } else {
        url = m_userFeeds[name].url();
    }

    QNetworkReply *reply = download(url);
    reply->setProperty(DocsetNameProperty, name);
    reply->setProperty(DownloadTypeProperty, DownloadDocset);
    reply->setProperty(ListItemIndexProperty,
                       ui->availableDocsetList->row(findDocsetListItem(name)));
}

void DocsetsDialog::removeDocset(const QString &name)
{
    if (m_docsetRegistry->contains(name)) {
        m_docsetRegistry->unloadDocset(name);
    }

    const QString docsetPath
            = QDir(m_application->settings()->docsetPath).filePath(name + QLatin1String(".docset"));
    if (!m_application->fileManager()->removeRecursively(docsetPath)) {
        const QString error = tr("Cannot remove directory <b>%1</b>! It might be in use"
                                 " by another process.").arg(docsetPath);
        QMessageBox::warning(this, QStringLiteral("Zeal"), error);
        return;
    }

    QListWidgetItem *listItem = findDocsetListItem(name);
    if (listItem) {
        listItem->setHidden(false);
    }
}

void DocsetsDialog::updateStatus()
{
    QString text;

    if (!m_replies.isEmpty()) {
        text = tr("Downloading: %n.", nullptr, m_replies.size());
    }

    if (!m_tmpFiles.isEmpty()) {
        text += QLatin1String(" ") + tr("Installing: %n.", nullptr, m_tmpFiles.size());
    }

    ui->statusLabel->setText(text);

    enableControls();
}

QString DocsetsDialog::docsetNameForTmpFilePath(const QString &filePath) const
{
    for (auto it = m_tmpFiles.cbegin(), end = m_tmpFiles.cend(); it != end; ++it) {
        if (it.value()->fileName() == filePath) {
            return it.key();
        }
    }

    return QString();
}

int DocsetsDialog::percent(qint64 fraction, qint64 total)
{
    if (!total)
        return 0;

    return static_cast<int>(fraction / static_cast<double>(total) * 100);
}

QString DocsetsDialog::cacheLocation(const QString &fileName)
{
    return QDir(Core::Application::cacheLocation()).filePath(fileName);
}

bool DocsetsDialog::isDirWritable(const QString &path)
{
    auto file = std::make_unique<QTemporaryFile>(path + QLatin1String("/.zeal_writable_check_XXXXXX.tmp"));
    return file->open();
}
