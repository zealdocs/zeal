// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docsetsdialog.h"
#include "ui_docsetsdialog.h"

#include "docsetlistitemdelegate.h"
#include "widgets/emptystatelabel.h"

#include <core/application.h>
#include <core/extractor.h>
#include <core/filemanager.h>
#include <core/settings.h>
#include <registry/docset.h>
#include <registry/docsetregistry.h>
#include <registry/itemdatarole.h>
#include <util/humanizer.h>

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocale>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QTemporaryFile>
#include <QUrl>

#include <algorithm>
#include <memory>

namespace Zeal::WidgetUi {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.widgetui.docsetsdialog")

using Qt::Literals::StringLiterals::operator""_L1;

enum class DownloadType {
    DashFeed,
    Docset,
    DocsetList,
    TarixIndex
};

constexpr auto ApiServerUrl = "https://api.zealdocs.org/v1"_L1;
constexpr auto RedirectServerUrl = "https://go.zealdocs.org/d/%1/%2/latest"_L1;
// TODO: Each source plugin should have its own cache
constexpr auto DocsetListCacheFileName = "com.kapeli.json"_L1;

// TODO: Make the timeout period configurable
constexpr int CacheTimeout = 24 * 60 * 60; // 24 hours in seconds

// Read downloads in small chunks to keep the UI responsive.
constexpr qint64 DownloadChunkSize = 1024LL * 1024; // 1 MiB

// QNetworkReply properties
constexpr const char *DocsetNameProperty = "docsetName";
constexpr const char *DownloadTypeProperty = "downloadType";
constexpr const char *ListItemIndexProperty = "listItem";
constexpr const char *TarixRetryProperty = "tarixRetry";

constexpr int MaxTarixIndexRetries = 2;

void setDownloadType(QNetworkReply *reply, DownloadType type)
{
    reply->setProperty(DownloadTypeProperty, static_cast<int>(type));
}

DownloadType downloadType(const QNetworkReply *reply)
{
    return static_cast<DownloadType>(reply->property(DownloadTypeProperty).toInt());
}
} // namespace

DocsetsDialog::DocsetsDialog(Core::Application *app, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DocsetsDialog())
    , m_application(app)
    , m_docsetRegistry(app->docsetRegistry())
{
    ui->setupUi(this);

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

    connect(m_application->extractor(), &Core::Extractor::completed, this, &DocsetsDialog::extractionCompleted);
    connect(m_application->extractor(), &Core::Extractor::error, this, &DocsetsDialog::extractionError);
    connect(m_application->extractor(), &Core::Extractor::progress, this, &DocsetsDialog::extractionProgress);

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
    updateInstalledDocsetsEmptyState();
    updateAvailableDocsetsEmptyState();

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
    if (!clipboardText.startsWith(QLatin1String("dash-feed://"))) {
        clipboardText.clear();
    }

    QString feedUrl = QInputDialog::getText(this,
                                            QStringLiteral("Zeal"),
                                            tr("Feed URL:"),
                                            QLineEdit::Normal,
                                            clipboardText);
    feedUrl = feedUrl.trimmed();
    if (feedUrl.isEmpty()) {
        return;
    }

    if (feedUrl.startsWith(QLatin1String("dash-feed://"))) {
        feedUrl = feedUrl.remove(0, 12);
        feedUrl = QUrl::fromPercentEncoding(feedUrl.toUtf8());
    }

    QNetworkReply *reply = download(QUrl(feedUrl));
    setDownloadType(reply, DownloadType::DashFeed);
}

void DocsetsDialog::updateSelectedDocsets()
{
    const auto selectedRows = ui->installedDocsetList->selectionModel()->selectedRows();
    for (const QModelIndex &index : selectedRows) {
        if (!index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
            continue;
        }

        downloadDashDocset(index);
    }
}

void DocsetsDialog::updateAllDocsets()
{
    const QAbstractItemModel *model = ui->installedDocsetList->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex index = model->index(i, 0);
        if (!index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
            continue;
        }

        downloadDashDocset(index);
    }
}

void DocsetsDialog::removeSelectedDocsets()
{
    const QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    if (!selectionModel->hasSelection()) {
        return;
    }

    auto rc = QMessageBox::NoButton;
    const QModelIndexList selectedIndexes = selectionModel->selectedRows();
    if (selectedIndexes.size() == 1) {
        const QString docsetTitle = selectedIndexes.first().data().toString();
        rc = QMessageBox::question(this, QStringLiteral("Zeal"), tr("Remove <b>%1</b> docset?").arg(docsetTitle));
    } else {
        rc = QMessageBox::question(this,
                                   QStringLiteral("Zeal"),
                                   tr("Remove <b>%n</b> docset(s)?",
                                      nullptr,
                                      static_cast<int>(selectedIndexes.size())));
    }

    if (rc == QMessageBox::No) {
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
        if (m_docsetRegistry->contains(item->data(Registry::ItemDataRole::DocsetNameRole).toString())) {
            continue;
        }

        item->setHidden(doSearch && !item->text().contains(filterString, Qt::CaseInsensitive));
    }

    updateAvailableDocsetsEmptyState();
}

void DocsetsDialog::downloadSelectedDocsets()
{
    QItemSelectionModel *selectionModel = ui->availableDocsetList->selectionModel();
    const auto selectedRows = selectionModel->selectedRows();
    for (const QModelIndex &index : selectedRows) {
        selectionModel->select(index, QItemSelectionModel::Deselect);

        // Do nothing if a download is already in progress.
        if (index.data(DocsetListItemDelegate::ShowProgressRole).toBool()) {
            continue;
        }

        QAbstractItemModel *model = ui->availableDocsetList->model();
        model->setData(index, tr("Downloading: %p%"), DocsetListItemDelegate::FormatRole);
        model->setData(index, 0, DocsetListItemDelegate::ValueRole);
        model->setData(index, true, DocsetListItemDelegate::ShowProgressRole);

        downloadDashDocset(index);
    }
}

QTemporaryFile *DocsetsDialog::docsetTemporaryFile(const QString &docsetName)
{
    // A name with path separators could escape the cache and storage directories.
    if (docsetName.contains(QLatin1Char('/')) || docsetName.contains(QLatin1Char('\\'))) {
        qCWarning(log, "Refusing docset with an unsafe name '%s'.", qPrintable(docsetName));
        return nullptr;
    }

    QTemporaryFile *tmpFile = m_tmpFiles.value(docsetName);
    if (tmpFile != nullptr) {
        return tmpFile;
    }

    tmpFile = new QTemporaryFile(QStringLiteral("%1/%2.XXXXXX.tmp").arg(Core::Application::cacheLocation(), docsetName),
                                 this);
    if (!tmpFile->open()) {
        qCWarning(log, "Cannot create a temporary file for '%s'.", qPrintable(docsetName));
        delete tmpFile;
        return nullptr;
    }

    m_tmpFiles.insert(docsetName, tmpFile);
    return tmpFile;
}

/*!
  \internal
  Should be connected to all \l QNetworkReply::finished signals in order to process possible
  HTTP-redirects correctly.
*/
void DocsetsDialog::downloadCompleted()
{
    const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(qobject_cast<QNetworkReply *>(sender()));

    m_replies.removeOne(reply.data());

    if (reply->error() != QNetworkReply::NoError) {
        if (downloadType(reply.data()) == DownloadType::TarixIndex) {
            if (reply->error() != QNetworkReply::OperationCanceledError) {
                onTarixIndexFailed(reply.data());
            }

            updateStatus();
            return;
        }

        if (reply->error() != QNetworkReply::OperationCanceledError) {
            const QString msg = tr("Download failed!<br><br><b>Error:</b> %1<br><b>URL:</b> %2")
                                    .arg(reply->errorString(), reply->request().url().toString());
            const int ret = QMessageBox::warning(this,
                                                 QStringLiteral("Zeal"),
                                                 msg,
                                                 QMessageBox::Retry | QMessageBox::Cancel);

            if (ret == QMessageBox::Retry) {
                QNetworkReply *newReply = download(reply->request().url());

                // Copy properties
                newReply->setProperty(DocsetNameProperty, reply->property(DocsetNameProperty));
                setDownloadType(newReply, downloadType(reply.data()));
                newReply->setProperty(ListItemIndexProperty, reply->property(ListItemIndexProperty));
                return;
            }

            bool ok = false;
            QListWidgetItem *listItem = ui->availableDocsetList->item(
                reply->property(ListItemIndexProperty).toInt(&ok));
            if (ok && listItem != nullptr) {
                listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);
            }
        }

        updateStatus();
        return;
    }

    const auto type = downloadType(reply.data());
    switch (type) {
    case DownloadType::DocsetList:
        processDocsetListReply(reply.data());
        break;

    case DownloadType::DashFeed: {
        const Registry::DocsetMetadata metadata = Registry::DocsetMetadata::fromDashFeed(reply->request().url(),
                                                                                         reply->readAll());

        if (metadata.urls().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Zeal"), tr("Invalid docset feed!"));
            break;
        }

        m_userFeeds[metadata.name()] = metadata;
        Registry::Docset *docset = m_docsetRegistry->docset(metadata.name());
        if (docset == nullptr) {
            // Fetch docset only on first feed download,
            // since further downloads are only update checks
            QNetworkReply *mdReply = download(metadata.url());
            mdReply->setProperty(DocsetNameProperty, metadata.name());
            setDownloadType(mdReply, DownloadType::Docset);
        } else {
            // Check for feed update
            if (metadata.latestVersion() != docset->version() || metadata.revision() > docset->revision()) {
                docset->setUpdate(Registry::Docset::UpdateInfo{.version = metadata.latestVersion(),
                                                               .revision = metadata.revision(),
                                                               .size = metadata.size()});

                if (!m_isStorageReadOnly) {
                    ui->updateAllDocsetsButton->setEnabled(true);
                }

                ui->installedDocsetList->reset();
            }
        }

        break;
    }

    case DownloadType::Docset: {
        const QString docsetName = reply->property(DocsetNameProperty).toString();
        const QString docsetDirectoryName = docsetName + QLatin1String(".docset");

        if (QDir(m_application->settings()->docsetPath).exists(docsetDirectoryName)) {
            removeDocset(docsetName);
        }

        QTemporaryFile *tmpFile = docsetTemporaryFile(docsetName);
        if (tmpFile == nullptr) {
            QListWidgetItem *listItem = findDocsetListItem(docsetName);
            if (listItem != nullptr) {
                listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);
            }
            QMessageBox::warning(this,
                                 QStringLiteral("Zeal"),
                                 tr("Cannot create a temporary file to install <b>%1</b>.").arg(docsetName));
            break;
        }

        while (reply->bytesAvailable() > 0) {
            tmpFile->write(reply->read(DownloadChunkSize));
        }

        tmpFile->close();

        QListWidgetItem *item = findDocsetListItem(docsetName);
        if (item != nullptr) {
            item->setData(DocsetListItemDelegate::ValueRole, 0);
            item->setData(DocsetListItemDelegate::FormatRole, tr("Installing: %p%"));
        }

        const Registry::DocsetMetadata metadata = m_availableDocsets.contains(docsetName)
                                                    ? m_availableDocsets[docsetName]
                                                    : m_userFeeds[docsetName];

        // A tarix index lets Zeal serve documents from the archive without extraction.
        if (metadata.hasTarix()) {
            QUrl indexUrl = reply->url();
            indexUrl.setPath(indexUrl.path() + QLatin1String(".tarix"));
            downloadTarixIndex(docsetName, indexUrl, 0);
        } else {
            installDownloadedDocset(docsetName);
        }

        break;
    }

    case DownloadType::TarixIndex: {
        const QString docsetName = reply->property(DocsetNameProperty).toString();
        const QString docsetDirectoryName = docsetName + QLatin1String(".docset");

        const QTemporaryFile *tmpFile = m_tmpFiles.value(docsetName);
        if (tmpFile == nullptr) {
            break; // Installation has been canceled.
        }

        const QByteArray indexData = reply->readAll();

        // Some mirrors soft-404 with an HTML page; verify the gzip signature.
        if (indexData.startsWith(QByteArrayLiteral("\x1f\x8b"))) {
            auto *indexFile = new QTemporaryFile(QStringLiteral("%1/%2.tarix.XXXXXX.tmp")
                                                     .arg(Core::Application::cacheLocation(), docsetName),
                                                 this);
            if (indexFile->open() && indexFile->write(indexData) == indexData.size()) {
                indexFile->close();
                m_tarixIndexFiles.insert(docsetName, indexFile);
                m_application->extractor()->installTarixDocset(tmpFile->fileName(),
                                                               indexFile->fileName(),
                                                               m_application->settings()->docsetPath,
                                                               docsetDirectoryName);
                break;
            }

            delete indexFile;
        }

        installDownloadedDocset(docsetName);
        break;
    }

    default:
        qCWarning(log, "Unknown download type %d.", static_cast<int>(type));
        break;
    }

    // If all enqueued downloads have finished executing.
    updateStatus();
}

// creates a total download progress for multiple QNetworkReplies
void DocsetsDialog::downloadProgress(qint64 received, qint64 total)
{
    // Don't show progress for non-docset pages
    if (total == -1 || received < 10240) {
        return;
    }

    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply == nullptr || !reply->isOpen()) {
        return;
    }

    if (downloadType(reply) == DownloadType::Docset) {
        const QString docsetName = reply->property(DocsetNameProperty).toString();

        QTemporaryFile *tmpFile = docsetTemporaryFile(docsetName);
        if (tmpFile == nullptr) {
            return;
        }

        tmpFile->write(reply->read(received));
    }

    // Try to get the item associated to the request
    QListWidgetItem *item = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
    if (item != nullptr) {
        item->setData(DocsetListItemDelegate::ValueRole, percent(received, total));
    }
}

void DocsetsDialog::extractionCompleted(const QString &filePath)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);

    const QDir dataDir(m_application->settings()->docsetPath);
    const QString docsetPath = dataDir.filePath(docsetName + QLatin1String(".docset"));

    // Write metadata about docset
    Registry::DocsetMetadata metadata = m_availableDocsets.contains(docsetName) ? m_availableDocsets[docsetName]
                                                                                : m_userFeeds[docsetName];
    metadata.save(docsetPath, metadata.latestVersion());

    m_docsetRegistry->loadDocset(docsetPath);

    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem != nullptr) {
        listItem->setHidden(true);
        listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);
    }

    delete m_tmpFiles.take(docsetName);
    delete m_tarixIndexFiles.take(docsetName);

    updateStatus();
}

void DocsetsDialog::extractionError(const QString &filePath, const QString &errorString)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);

    QMessageBox::warning(this,
                         QStringLiteral("Zeal"),
                         tr("Cannot extract docset <b>%1</b>: %2").arg(docsetName, errorString));

    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem != nullptr) {
        listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);
    }

    delete m_tmpFiles.take(docsetName);
    delete m_tarixIndexFiles.take(docsetName);
}

void DocsetsDialog::extractionProgress(const QString &filePath, qint64 extracted, qint64 total)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);
    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem != nullptr) {
        listItem->setData(DocsetListItemDelegate::ValueRole, percent(extracted, total));
    }
}

void DocsetsDialog::loadDocsetList()
{
    loadUserFeedList();

    const QFileInfo fi(cacheLocation(DocsetListCacheFileName));
    if (!fi.exists()) {
        downloadDocsetList();
        return;
    }

    const auto age = fi.lastModified().secsTo(QDateTime::currentDateTime());
    if (age < 0 || age >= CacheTimeout) {
        downloadDocsetList();
        return;
    }

    QFile file(fi.filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        downloadDocsetList();
        return;
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        downloadDocsetList();
        return;
    }

    updateDocsetListDownloadTimeLabel(fi.lastModified());
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

    m_installedDocsetsEmptyState = new EmptyStateLabel(ui->installedDocsetList, tr("No installed docsets"));

    const QAbstractItemModel *installedModel = ui->installedDocsetList->model();
    connect(installedModel, &QAbstractItemModel::rowsInserted, this, &DocsetsDialog::updateInstalledDocsetsEmptyState);
    connect(installedModel, &QAbstractItemModel::rowsRemoved, this, &DocsetsDialog::updateInstalledDocsetsEmptyState);
    connect(installedModel, &QAbstractItemModel::modelReset, this, &DocsetsDialog::updateInstalledDocsetsEmptyState);
    connect(m_docsetRegistry,
            &Registry::DocsetRegistry::docsetLoadingStarted,
            this,
            &DocsetsDialog::updateInstalledDocsetsEmptyState);
    connect(m_docsetRegistry,
            &Registry::DocsetRegistry::docsetLoadingFinished,
            this,
            &DocsetsDialog::updateInstalledDocsetsEmptyState);

    if (m_isStorageReadOnly) {
        return;
    }

    connect(ui->installedDocsetList, &QTreeView::activated, this, [this](const QModelIndex &index) {
        if (!index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
            return;
        }

        downloadDashDocset(index);
    });

    const QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, [this, selectionModel]() {
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

    connect(ui->updateSelectedDocsetsButton, &QPushButton::clicked, this, &DocsetsDialog::updateSelectedDocsets);
    connect(ui->updateAllDocsetsButton, &QPushButton::clicked, this, &DocsetsDialog::updateAllDocsets);
    connect(ui->removeDocsetsButton, &QPushButton::clicked, this, &DocsetsDialog::removeSelectedDocsets);
}

void DocsetsDialog::setupAvailableDocsetsTab()
{
    using Registry::DocsetRegistry;

    ui->availableDocsetList->setItemDelegate(new DocsetListItemDelegate(this));

    connect(m_docsetRegistry, &DocsetRegistry::docsetUnloaded, this, [this](const QString &name) {
        QListWidgetItem *item = findDocsetListItem(name);
        if (item == nullptr) {
            return;
        }

        item->setHidden(false);
        updateAvailableDocsetsEmptyState();
    });
    connect(m_docsetRegistry, &DocsetRegistry::docsetLoaded, this, [this](const QString &name) {
        QListWidgetItem *item = findDocsetListItem(name);
        if (item == nullptr) {
            return;
        }

        item->setHidden(true);
        updateAvailableDocsetsEmptyState();
    });

    connect(ui->refreshButton, &QPushButton::clicked, this, &DocsetsDialog::downloadDocsetList);

    connect(ui->docsetFilterInput, &QLineEdit::textEdited, this, &DocsetsDialog::updateDocsetFilter);

    m_availableDocsetsEmptyState = new EmptyStateLabel(ui->availableDocsetList, tr("No available docsets"));

    if (m_isStorageReadOnly) {
        return;
    }

    connect(ui->availableDocsetList, &QListView::activated, this, [this](const QModelIndex &index) {
        // TODO: Cancel download if it's already in progress.
        if (index.data(DocsetListItemDelegate::ShowProgressRole).toBool()) {
            return;
        }

        ui->availableDocsetList->selectionModel()->select(index, QItemSelectionModel::Deselect);

        QAbstractItemModel *model = ui->availableDocsetList->model();
        model->setData(index, tr("Downloading: %p%"), DocsetListItemDelegate::FormatRole);
        model->setData(index, 0, DocsetListItemDelegate::ValueRole);
        model->setData(index, true, DocsetListItemDelegate::ShowProgressRole);

        downloadDashDocset(index);
    });

    const QItemSelectionModel *selectionModel = ui->availableDocsetList->selectionModel();
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

    connect(ui->downloadDocsetsButton, &QPushButton::clicked, this, &DocsetsDialog::downloadSelectedDocsets);
}

void DocsetsDialog::updateInstalledDocsetsEmptyState()
{
    if (m_installedDocsetsEmptyState == nullptr) {
        return;
    }

    m_installedDocsetsEmptyState->setText(m_docsetRegistry->isLoading() ? tr("Loading docsets")
                                                                        : tr("No installed docsets"));
    m_installedDocsetsEmptyState->setEmpty(ui->installedDocsetList->model()->rowCount() == 0);
}

void DocsetsDialog::updateAvailableDocsetsEmptyState()
{
    if (m_availableDocsetsEmptyState == nullptr) {
        return;
    }

    bool hasVisibleDocsets = false;
    for (int i = 0; i < ui->availableDocsetList->count(); ++i) {
        if (!ui->availableDocsetList->item(i)->isHidden()) {
            hasVisibleDocsets = true;
            break;
        }
    }

    const bool isFiltering = !ui->docsetFilterInput->text().simplified().isEmpty();
    if (isFiltering) {
        m_availableDocsetsEmptyState->setText(tr("No matching docsets"));
    } else if (!m_availableDocsets.empty()) {
        m_availableDocsetsEmptyState->setText(tr("All available docsets are installed"));
    } else {
        m_availableDocsetsEmptyState->setText(tr("No available docsets"));
    }

    m_availableDocsetsEmptyState->setEmpty(!hasVisibleDocsets && m_replies.isEmpty());
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
    const QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
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

        if (item->data(Registry::ItemDataRole::DocsetNameRole).toString() == name) {
            return item;
        }
    }

    return nullptr;
}

bool DocsetsDialog::updatesAvailable() const
{
    return std::ranges::any_of(m_docsetRegistry->docsets(), [](const Registry::Docset *docset) {
        return docset->hasUpdate();
    });
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
        QListWidgetItem *listItem = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
        if (listItem != nullptr) {
            listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);
        }

        const DownloadType type = downloadType(reply);
        if (type == DownloadType::Docset || type == DownloadType::TarixIndex) {
            delete m_tmpFiles.take(reply->property(DocsetNameProperty).toString());
        }

        reply->abort();
    }

    updateStatus();
}

void DocsetsDialog::loadUserFeedList()
{
    const auto docsets = m_docsetRegistry->docsets();
    for (const Registry::Docset *docset : docsets) {
        if (!docset->feedUrl().isEmpty()) {
            QNetworkReply *reply = download(QUrl(docset->feedUrl()));
            setDownloadType(reply, DownloadType::DashFeed);
        }
    }
}

void DocsetsDialog::downloadDocsetList()
{
    ui->availableDocsetList->clear();
    m_availableDocsets.clear();

    QNetworkReply *reply = download(QUrl(ApiServerUrl + QLatin1String("/docsets")));
    setDownloadType(reply, DownloadType::DocsetList);
}

void DocsetsDialog::processDocsetListReply(QNetworkReply *reply)
{
    const QByteArray replyData = reply->readAll();

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(replyData, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        qCWarning(log,
                  "Failed to parse docset list JSON at offset %lld: %s.",
                  static_cast<long long>(jsonError.offset),
                  qPrintable(jsonError.errorString()));
        const QMessageBox::StandardButton rc = QMessageBox::warning(this,
                                                                    QStringLiteral("Zeal"),
                                                                    tr("Server returned a corrupted docset list."),
                                                                    QMessageBox::Retry | QMessageBox::Cancel);

        if (rc == QMessageBox::Retry) {
            downloadDocsetList();
        }

        return;
    }

    QFile file(cacheLocation(DocsetListCacheFileName));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(replyData);
        file.close(); // Flush to ensure timestamp update on all systems.
        updateDocsetListDownloadTimeLabel(QFileInfo(file.fileName()).lastModified());
    }

    processDocsetList(jsonDoc.array());
}

void DocsetsDialog::processDocsetList(const QJsonArray &list)
{
    for (const auto &v : list) {
        const QJsonObject docsetJson = v.toObject();

        const Registry::DocsetMetadata metadata(docsetJson);
        m_availableDocsets.insert({metadata.name(), metadata});
    }

    // TODO: Move into dedicated method
    for (const auto &kv : m_availableDocsets) {
        const auto &metadata = kv.second;

        auto *listItem = new QListWidgetItem(metadata.icon(), metadata.title(), ui->availableDocsetList);
        listItem->setData(Registry::ItemDataRole::DocsetNameRole, metadata.name());

        QStringList tooltipLines;
        if (!metadata.latestVersion().isEmpty()) {
            tooltipLines << tr("Version: %1r%2").arg(metadata.latestVersion()).arg(metadata.revision());
        }

        if (metadata.size() > 0) {
            tooltipLines << tr("Download size: %1").arg(QLocale::system().formattedDataSize(metadata.size()));
        }

        if (!tooltipLines.isEmpty()) {
            listItem->setToolTip(tooltipLines.join(QLatin1Char('\n')));
        }

        if (!m_docsetRegistry->contains(metadata.name())) {
            continue;
        }

        listItem->setHidden(true);

        Registry::Docset *docset = m_docsetRegistry->docset(metadata.name());

        if (metadata.latestVersion() != docset->version() || metadata.revision() > docset->revision()) {
            docset->setUpdate(Registry::Docset::UpdateInfo{.version = metadata.latestVersion(),
                                                           .revision = metadata.revision(),
                                                           .size = metadata.size()});

            if (!m_isStorageReadOnly) {
                ui->updateAllDocsetsButton->setEnabled(true);
            }
        }
    }

    ui->installedDocsetList->reset();

    // Reapply the filter after repopulating the list.
    updateDocsetFilter(ui->docsetFilterInput->text());
}

void DocsetsDialog::updateDocsetListDownloadTimeLabel(const QDateTime &modifiedTime)
{
    if (!modifiedTime.isValid()) {
        ui->lastUpdatedLabel->clear();
        ui->lastUpdatedLabel->setToolTip(QString());
        return;
    }

    ui->lastUpdatedLabel->setText(tr("Last updated %1.").arg(Util::Humanizer::fromNow(modifiedTime)));

    const QString updateTime = modifiedTime.toString(QLocale::system().dateTimeFormat(QLocale::ShortFormat));
    ui->lastUpdatedLabel->setToolTip(updateTime);
}

void DocsetsDialog::downloadDashDocset(const QModelIndex &index)
{
    const QString name = index.data(Registry::ItemDataRole::DocsetNameRole).toString();

    if (!m_availableDocsets.contains(name) && !m_userFeeds.contains(name)) {
        return;
    }

    // Skip if an extraction is already in progress for this docset.
    if (m_tmpFiles.contains(name)) {
        return;
    }

    // Skip if a download is already in progress for this docset.
    for (const QNetworkReply *reply : std::as_const(m_replies)) {
        if (downloadType(reply) == DownloadType::Docset && reply->property(DocsetNameProperty).toString() == name) {
            return;
        }
    }

    QUrl url;
    if (!m_userFeeds.contains(name)) {
        // No feed present means that this is a Kapeli docset
        const QString urlString = QString(RedirectServerUrl).arg(QLatin1String("com.kapeli"), name);
        url = QUrl(urlString);
    } else {
        url = m_userFeeds[name].url();
    }

    QNetworkReply *reply = download(url);
    reply->setProperty(DocsetNameProperty, name);
    setDownloadType(reply, DownloadType::Docset);
    reply->setProperty(ListItemIndexProperty, ui->availableDocsetList->row(findDocsetListItem(name)));
}

void DocsetsDialog::downloadTarixIndex(const QString &docsetName, const QUrl &indexUrl, int attempt)
{
    QNetworkReply *reply = download(indexUrl);
    reply->setProperty(DocsetNameProperty, docsetName);
    reply->setProperty(TarixRetryProperty, attempt);
    reply->setProperty(ListItemIndexProperty, ui->availableDocsetList->row(findDocsetListItem(docsetName)));
    setDownloadType(reply, DownloadType::TarixIndex);
}

void DocsetsDialog::onTarixIndexFailed(QNetworkReply *reply)
{
    const QString docsetName = reply->property(DocsetNameProperty).toString();
    const QUrl indexUrl = reply->request().url();
    const int attempt = reply->property(TarixRetryProperty).toInt();

    if (attempt < MaxTarixIndexRetries) {
        downloadTarixIndex(docsetName, indexUrl, attempt + 1);
        return;
    }

    QMessageBox box(QMessageBox::Warning,
                    QStringLiteral("Zeal"),
                    tr("Could not download the compact index for <b>%1</b>. Installing without it will be "
                       "slower and use considerably more disk space.")
                        .arg(docsetName),
                    QMessageBox::NoButton,
                    this);
    QPushButton *retryButton = box.addButton(QMessageBox::Retry);
    const QPushButton *installButton = box.addButton(tr("Install Anyway"), QMessageBox::AcceptRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(retryButton);
    box.exec();

    if (box.clickedButton() == retryButton) {
        downloadTarixIndex(docsetName, indexUrl, 0);
    } else if (box.clickedButton() == installButton) {
        installDownloadedDocset(docsetName);
    } else {
        QListWidgetItem *listItem = findDocsetListItem(docsetName);
        if (listItem != nullptr) {
            listItem->setData(DocsetListItemDelegate::ShowProgressRole, false);
        }
        delete m_tmpFiles.take(docsetName);
    }
}

void DocsetsDialog::installDownloadedDocset(const QString &docsetName)
{
    const QTemporaryFile *tmpFile = m_tmpFiles.value(docsetName);
    if (tmpFile == nullptr) {
        return;
    }

    m_application->extractor()->extract(tmpFile->fileName(),
                                        m_application->settings()->docsetPath,
                                        docsetName + QLatin1String(".docset"));
}

void DocsetsDialog::removeDocset(const QString &name)
{
    if (!m_docsetRegistry->contains(name)) {
        return;
    }

    const QString docsetPath = m_docsetRegistry->docset(name)->path();
    m_docsetRegistry->unloadDocset(name);
    if (!Core::FileManager::removeRecursively(docsetPath)) {
        const QString error = tr("Cannot remove directory <b>%1</b>! It might be in use"
                                 " by another process.")
                                  .arg(docsetPath);
        QMessageBox::warning(this, QStringLiteral("Zeal"), error);
        return;
    }

    QListWidgetItem *listItem = findDocsetListItem(name);
    if (listItem != nullptr) {
        listItem->setHidden(false);
    }
}

void DocsetsDialog::updateStatus()
{
    QString text;

    if (!m_replies.isEmpty()) {
        text = tr("Downloading: %n.", nullptr, static_cast<int>(m_replies.size()));
    }

    if (!m_tmpFiles.isEmpty()) {
        text += QLatin1String(" ") + tr("Installing: %n.", nullptr, static_cast<int>(m_tmpFiles.size()));
    }

    ui->statusLabel->setText(text);
    updateAvailableDocsetsEmptyState();

    enableControls();
}

QString DocsetsDialog::docsetNameForTmpFilePath(const QString &filePath) const
{
    for (auto it = m_tmpFiles.cbegin(), end = m_tmpFiles.cend(); it != end; ++it) {
        if (it.value()->fileName() == filePath) {
            return it.key();
        }
    }

    return {};
}

int DocsetsDialog::percent(qint64 fraction, qint64 total)
{
    if (total == 0) {
        return 0;
    }

    return static_cast<int>(static_cast<double>(fraction) / static_cast<double>(total) * 100);
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

} // namespace Zeal::WidgetUi
