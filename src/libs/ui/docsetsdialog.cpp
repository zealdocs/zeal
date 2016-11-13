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
#include "progressitemdelegate.h"

#include <core/application.h>
#include <core/settings.h>
#include <registry/docsetregistry.h>
#include <registry/itemdatarole.h>
#include <registry/listmodel.h>

#include <QClipboard>
#include <QDir>
#include <QFutureWatcher>
#include <QInputDialog>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QUrl>

#include <QtConcurrent/QtConcurrent>

using namespace Zeal;

namespace {
const char ApiServerUrl[] = "http://api.zealdocs.org/v1";
const char RedirectServerUrl[] = "https://go.zealdocs.org";
// TODO: Each source plugin should have its own cache
const char DocsetListCacheFileName[] = "com.kapeli.json";

// TODO: Make the timeout period configurable
constexpr int CacheTimeout = 24 * 60 * 60 * 1000; // 24 hours in microseconds

// QNetworkReply properties
const char DocsetNameProperty[] = "docsetName";
const char DownloadTypeProperty[] = "downloadType";
const char DownloadPreviousReceived[] = "downloadPreviousReceived";
const char ListItemIndexProperty[] = "listItem";
}

DocsetsDialog::DocsetsDialog(Core::Application *app, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DocsetsDialog()),
    m_application(app),
    m_docsetRegistry(app->docsetRegistry())
{
    using Registry::DocsetRegistry;
    using Registry::ListModel;

    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

#ifdef Q_OS_OSX
    ui->availableDocsetList->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->installedDocsetList->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    ui->combinedProgressBar->hide();
    ui->cancelButton->hide();

    // Installed docsets tab
    ui->installedDocsetList->setItemDelegate(new DocsetListItemDelegate(this));
    ui->installedDocsetList->setModel(new ListModel(app->docsetRegistry(), this));
    connect(ui->installedDocsetList, &QListView::activated, this, [this](const QModelIndex &index) {
        if (!index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
            return;
        }

        downloadDashDocset(index);
    });

    QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            [this, selectionModel]() {
        ui->removeDocsetsButton->setEnabled(selectionModel->hasSelection());

        for (const QModelIndex &index : selectionModel->selectedRows()) {
            if (index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
                ui->updateSelectedDocsetsButton->setEnabled(true);
                return;
            }
        }

        ui->updateSelectedDocsetsButton->setEnabled(false);
    });
    connect(ui->updateSelectedDocsetsButton, &QPushButton::clicked,
            this, &DocsetsDialog::updateSelectedDocsets);
    connect(ui->updateAllDocsetsButton, &QPushButton::clicked,
            this, &DocsetsDialog::updateAllDocsets);
    connect(ui->removeDocsetsButton, &QPushButton::clicked,
            this, &DocsetsDialog::removeSelectedDocsets);
    connect(ui->docsetFilterInput, &QLineEdit::textEdited,
            this, &DocsetsDialog::updateDocsetFilter);

    // Available docsets tab
    ui->availableDocsetList->setItemDelegate(new ProgressItemDelegate(this));
    connect(ui->availableDocsetList, &QListView::activated, this, [this](const QModelIndex &index) {
        // TODO: Cancel download if it's already in progress.
        if (index.data(ProgressItemDelegate::ShowProgressRole).toBool())
            return;

        ui->availableDocsetList->selectionModel()->select(index, QItemSelectionModel::Deselect);

        QAbstractItemModel *model = ui->availableDocsetList->model();
        model->setData(index, tr("Downloading: %p%"), ProgressItemDelegate::FormatRole);
        model->setData(index, 0, ProgressItemDelegate::ValueRole);
        model->setData(index, true, ProgressItemDelegate::ShowProgressRole);

        downloadDashDocset(index);
    });

    selectionModel = ui->availableDocsetList->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged, [this, selectionModel]() {
        for (const QModelIndex &index : selectionModel->selectedRows()) {
            if (!index.data(ProgressItemDelegate::ShowProgressRole).toBool()) {
                ui->downloadDocsetsButton->setEnabled(true);
                return;
            }
        }

        ui->downloadDocsetsButton->setEnabled(false);
    });

    connect(ui->downloadDocsetsButton, &QPushButton::clicked,
            this, &DocsetsDialog::downloadSelectedDocsets);

    connect(m_docsetRegistry, &DocsetRegistry::docsetRemoved, this, [this](const QString name) {
        QListWidgetItem *item = findDocsetListItem(name);
        if (!item)
            return;

        item->setHidden(false);
    });
    connect(m_docsetRegistry, &DocsetRegistry::docsetAdded, this, [this](const QString name) {
        QListWidgetItem *item = findDocsetListItem(name);
        if (!item)
            return;

        item->setHidden(true);
    });

    connect(ui->addFeedButton, &QPushButton::clicked, this, &DocsetsDialog::addDashFeed);
    connect(ui->refreshButton, &QPushButton::clicked, this, &DocsetsDialog::downloadDocsetList);

    connect(m_application, &Core::Application::extractionCompleted,
            this, &DocsetsDialog::extractionCompleted);
    connect(m_application, &Core::Application::extractionError,
            this, &DocsetsDialog::extractionError);
    connect(m_application, &Core::Application::extractionProgress,
            this, &DocsetsDialog::extractionProgress);

    connect(ui->cancelButton, &QPushButton::clicked, this, &DocsetsDialog::cancelDownloads);

    loadDocsetList();
}

DocsetsDialog::~DocsetsDialog()
{
    delete ui;
}

void DocsetsDialog::reject()
{
    if (m_replies.isEmpty() && m_tmpFiles.isEmpty() && m_docsetsBeingDeleted.isEmpty()) {
        QDialog::reject();
        return;
    }

    QMessageBox::information(this, QStringLiteral("Zeal"),
                             tr("Please wait for all operations to finish."));
}

void DocsetsDialog::addDashFeed()
{
    QString clipboardText = QApplication::clipboard()->text();
    if (!clipboardText.startsWith(QLatin1String("dash-feed://")))
        clipboardText.clear();

    QString feedUrl = QInputDialog::getText(this, QStringLiteral("Zeal"), tr("Feed URL:"),
                                            QLineEdit::Normal, clipboardText);
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
    for (const QModelIndex &index : ui->installedDocsetList->selectionModel()->selectedRows()) {
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
    QItemSelectionModel *selectonModel = ui->installedDocsetList->selectionModel();
    if (!selectonModel->hasSelection())
        return;

    int ret;

    const QModelIndexList selectedIndexes = selectonModel->selectedRows();
    if (selectedIndexes.size() == 1) {
        const QString docsetTitle = selectedIndexes.first().data().toString();
        ret = QMessageBox::question(this, QStringLiteral("Zeal"),
                                    tr("Remove <b>%1</b> docset?").arg(docsetTitle));
    } else {
        ret = QMessageBox::question(this, QStringLiteral("Zeal"),
                                    tr("Remove <b>%n</b> docset(s)?", nullptr,
                                       selectedIndexes.size()));
    }

    if (ret == QMessageBox::No)
        return;

    // Gather names first, because model indicies become invalid when docsets are removed.
    QStringList names;
    for (const QModelIndex &index : selectedIndexes)
        names.append(index.data(Registry::ItemDataRole::DocsetNameRole).toString());

    for (const QString &name : names)
        removeDocset(name);
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
    for (const QModelIndex &index : selectionModel->selectedRows()) {
        selectionModel->select(index, QItemSelectionModel::Deselect);

        // Do nothing if a download is already in progress.
        if (index.data(ProgressItemDelegate::ShowProgressRole).toBool())
            continue;

        QAbstractItemModel *model = ui->availableDocsetList->model();
        model->setData(index, tr("Downloading: %p%"), ProgressItemDelegate::FormatRole);
        model->setData(index, 0, ProgressItemDelegate::ValueRole);
        model->setData(index, true, ProgressItemDelegate::ShowProgressRole);

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
            const int ret = QMessageBox::warning(this, QStringLiteral("Zeal"), reply->errorString(),
                                                 QMessageBox::Retry | QMessageBox::Default,
                                                 QMessageBox::Cancel | QMessageBox::Escape,
                                                 QMessageBox::NoButton);

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
                listItem->setData(ProgressItemDelegate::ShowProgressRole, false);
        }

        resetProgress();
        return;
    }

    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectUrl.isValid()) {
        if (redirectUrl.isRelative())
            redirectUrl = reply->request().url().resolved(redirectUrl);

        // TODO: Verify if scheme can be missing
        if (redirectUrl.scheme().isEmpty())
            redirectUrl.setScheme(reply->request().url().scheme());

        QNetworkReply *newReply = download(redirectUrl);

        // Copy properties
        newReply->setProperty(DocsetNameProperty, reply->property(DocsetNameProperty));
        newReply->setProperty(DownloadTypeProperty, reply->property(DownloadTypeProperty));
        newReply->setProperty(ListItemIndexProperty, reply->property(ListItemIndexProperty));

        return;
    }

    switch (reply->property(DownloadTypeProperty).toUInt()) {
    case DownloadDocsetList: {
        const QByteArray data = reply->readAll();

        QScopedPointer<QFile> file(new QFile(cacheLocation(DocsetListCacheFileName)));
        if (file->open(QIODevice::WriteOnly))
            file->write(data);

        ui->lastUpdatedLabel->setText(QFileInfo(file->fileName())
                                      .lastModified().toString(Qt::SystemLocaleShortDate));

        QJsonParseError jsonError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, QStringLiteral("Zeal"),
                                 tr("Corrupted docset list: ") + jsonError.errorString());
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
        QNetworkReply *reply = download(metadata.url());
        reply->setProperty(DocsetNameProperty, metadata.name());
        reply->setProperty(DownloadTypeProperty, DownloadDocset);

        break;
    }

    case DownloadDocset: {
        const QString docsetName = reply->property(DocsetNameProperty).toString();

        // TODO: Implement an explicit and verbose docset update logic
        const QDir dir(m_application->settings()->docsetPath);
        if (dir.exists(docsetName + QLatin1String(".docset")))
            removeDocset(docsetName);

        QTemporaryFile *tmpFile = m_tmpFiles[docsetName];
        if (!tmpFile) {
            tmpFile = new QTemporaryFile(this);
            tmpFile->open();
            m_tmpFiles.insert(docsetName, tmpFile);
        }

        while (reply->bytesAvailable())
            tmpFile->write(reply->read(1024 * 1024)); // Use small chunks
        tmpFile->close();

        QListWidgetItem *item = findDocsetListItem(docsetName);
        if (item) {
            item->setData(ProgressItemDelegate::ValueRole, 0);
            item->setData(ProgressItemDelegate::FormatRole, tr("Installing: %p%"));
        }

        m_application->extract(tmpFile->fileName(), m_application->settings()->docsetPath,
                               docsetName + QLatin1String(".docset"));
        break;
    }
    }

    // If all enqueued downloads have finished executing
    if (m_replies.isEmpty())
        resetProgress();
}

// creates a total download progress for multiple QNetworkReplies
void DocsetsDialog::downloadProgress(qint64 received, qint64 total)
{
    // Don't show progress for non-docset pages
    if (total == -1 || received < 10240)
        return;

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply || !reply->isOpen())
        return;

    if (reply->property(DownloadTypeProperty).toInt() == DownloadDocset) {
        const QString docsetName = reply->property(DocsetNameProperty).toString();

        QTemporaryFile *tmpFile = m_tmpFiles[docsetName];
        if (!tmpFile) {
            tmpFile = new QTemporaryFile(this);
            tmpFile->open();
            m_tmpFiles.insert(docsetName, tmpFile);
        }

        tmpFile->write(reply->read(received));
    }

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

    updateCombinedProgress();
}

void DocsetsDialog::extractionCompleted(const QString &filePath)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);

    const QDir dataDir(m_application->settings()->docsetPath);
    const QString docsetPath = dataDir.absoluteFilePath(docsetName + QLatin1String(".docset"));

    // Write metadata about docset
    Registry::DocsetMetadata metadata = m_availableDocsets.contains(docsetName)
            ? m_availableDocsets[docsetName]
              : m_userFeeds[docsetName];
    metadata.save(docsetPath, metadata.latestVersion());

    m_docsetRegistry->addDocset(docsetPath);

    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem) {
        listItem->setHidden(true);
        listItem->setData(ProgressItemDelegate::ShowProgressRole, false);
    }

    resetProgress();
    delete m_tmpFiles.take(docsetName);
}

void DocsetsDialog::extractionError(const QString &filePath, const QString &errorString)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);

    QMessageBox::warning(this, QStringLiteral("Zeal"),
                         tr("Cannot extract docset <b>%1</b>: %2").arg(docsetName, errorString));

    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem)
        listItem->setData(ProgressItemDelegate::ShowProgressRole, false);

    delete m_tmpFiles.take(docsetName);
}

void DocsetsDialog::extractionProgress(const QString &filePath, qint64 extracted, qint64 total)
{
    const QString docsetName = docsetNameForTmpFilePath(filePath);
    QListWidgetItem *listItem = findDocsetListItem(docsetName);
    if (listItem)
        listItem->setData(ProgressItemDelegate::ValueRole, percent(extracted, total));
}

void DocsetsDialog::loadDocsetList()
{
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
    ui->lastUpdatedLabel->setText(fi.lastModified().toString(Qt::SystemLocaleShortDate));
    processDocsetList(jsonDoc.array());
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
    for (Registry::Docset *docset : m_docsetRegistry->docsets()) {
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

    // Installed docsets
    ui->addFeedButton->setEnabled(false);
    ui->updateSelectedDocsetsButton->setEnabled(false);
    ui->updateAllDocsetsButton->setEnabled(false);
    ui->removeDocsetsButton->setEnabled(false);

    // Available docsets
    ui->refreshButton->setEnabled(false);

    updateCombinedProgress();

    return reply;
}

void DocsetsDialog::cancelDownloads()
{
    for (QNetworkReply *reply : m_replies) {
        // Hide progress bar
        QListWidgetItem *listItem
                = ui->availableDocsetList->item(reply->property(ListItemIndexProperty).toInt());
        if (listItem)
            listItem->setData(ProgressItemDelegate::ShowProgressRole, false);

        if (reply->property(DownloadTypeProperty).toInt() == DownloadDocset)
            delete m_tmpFiles.take(reply->property(DocsetNameProperty).toString());

        reply->abort();
    }

    resetProgress();
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
        m_availableDocsets.insert(metadata.name(), metadata);
    }

    // TODO: Move into dedicated method
    for (const Registry::DocsetMetadata &metadata : m_availableDocsets) {
        QListWidgetItem *listItem
                = new QListWidgetItem(metadata.icon(), metadata.title(), ui->availableDocsetList);
        listItem->setData(Registry::ItemDataRole::DocsetNameRole, metadata.name());

        if (m_docsetRegistry->contains(metadata.name())) {
            listItem->setHidden(true);

            Registry::Docset *docset = m_docsetRegistry->docset(metadata.name());

            if (metadata.latestVersion() != docset->version()
                    || (metadata.latestVersion() == docset->version()
                        && metadata.revision() > docset->revision())) {
                docset->hasUpdate = true;
                ui->updateAllDocsetsButton->setEnabled(true);
            }
        }
    }

    ui->installedDocsetList->reset();
}

void DocsetsDialog::downloadDashDocset(const QModelIndex &index)
{
    const QString name = index.data(Registry::ItemDataRole::DocsetNameRole).toString();

    if (!m_availableDocsets.contains(name))
        return;

    const QString urlString = RedirectServerUrl + QStringLiteral("/d/com.kapeli/%1/latest");
    QNetworkReply *reply = download(QUrl(urlString.arg(name)));
    reply->setProperty(DocsetNameProperty, name);
    reply->setProperty(DownloadTypeProperty, DownloadDocset);
    reply->setProperty(ListItemIndexProperty,
                       ui->availableDocsetList->row(findDocsetListItem(name)));
}

void DocsetsDialog::removeDocset(const QString &name)
{
    if (m_docsetsBeingDeleted.contains(name))
        return;

    m_docsetsBeingDeleted.append(name);

    Registry::Docset *docset = m_docsetRegistry->docset(name);
    const QString title = docset->title();
    const QString docsetPath = docset->path();
    const QString tmpPath = docsetPath + QLatin1String(".deleteme.")
            + QString::number(QDateTime::currentMSecsSinceEpoch());

    // Remove from registry first to avoid renaming files in use on Windows.
    m_docsetRegistry->remove(name);

    // Rename first to allow simultaneous installation.
    if (!QDir().rename(docsetPath, tmpPath)) {
        const QString error = tr("Cannot delete docset <b>%1</b>! Please try closing other "
                                 "applications first, as they may be accessing the docset "
                                 "files.").arg(title);
        QMessageBox::warning(this, QStringLiteral("Zeal"), error);
        m_docsetsBeingDeleted.removeOne(name);
        m_docsetRegistry->addDocset(docsetPath);
        return;
    }

    QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>();
    connect(watcher, &QFutureWatcher<void>::finished, [=] {
        if (!watcher->result()) {
            QMessageBox::warning(this, QStringLiteral("Zeal"),
                                 tr("Cannot delete docset <b>%1</b>!").arg(title));
        }

        QListWidgetItem *listItem = findDocsetListItem(name);
        if (listItem)
            listItem->setHidden(false);

        watcher->deleteLater();

        m_docsetsBeingDeleted.removeOne(name);
    });

    watcher->setFuture(QtConcurrent::run([tmpPath] {
        return QDir(tmpPath).removeRecursively();
    }));
}

void DocsetsDialog::updateCombinedProgress()
{
    if (m_replies.isEmpty()) {
        resetProgress();
        return;
    }

    ui->combinedProgressBar->show();
    ui->combinedProgressBar->setValue(percent(m_combinedReceived, m_combinedTotal));
    ui->cancelButton->show();
}

void DocsetsDialog::resetProgress()
{
    if (!m_replies.isEmpty())
        return;

    ui->cancelButton->hide();
    ui->combinedProgressBar->hide();
    ui->combinedProgressBar->setValue(0);

    m_combinedReceived = 0;
    m_combinedTotal = 0;

    // Installed docsets
    ui->addFeedButton->setEnabled(true);
    QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    bool hasSelectedUpdates = false;
    for (const QModelIndex &index : selectionModel->selectedRows()) {
        if (index.data(Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
            hasSelectedUpdates = true;
            break;
        }
    }

    ui->updateSelectedDocsetsButton->setEnabled(hasSelectedUpdates);
    ui->updateAllDocsetsButton->setEnabled(updatesAvailable());
    ui->removeDocsetsButton->setEnabled(selectionModel->hasSelection());

    // Available docsets
    ui->refreshButton->setEnabled(true);
}

QString DocsetsDialog::docsetNameForTmpFilePath(const QString &filePath) const
{
    for (const QString &key : m_tmpFiles.keys()) {
        if (m_tmpFiles[key]->fileName() == filePath) {
            return key;
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
#ifndef PORTABLE_BUILD
    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
#else
    const QDir cacheDir(QCoreApplication::applicationDirPath() + QLatin1String("/cache"));
#endif
    // TODO: Report error
    QDir().mkpath(cacheDir.path());

    return cacheDir.filePath(fileName);
}
