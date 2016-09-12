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

    ui->docsetsProgress->hide();

    ui->installedDocsetList->setItemDelegate(new DocsetListItemDelegate(this));
    ui->installedDocsetList->setModel(new ListModel(app->docsetRegistry(), this));

    ui->installedDocsetList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QItemSelectionModel *selectionModel = ui->installedDocsetList->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            [this, selectionModel]() {
        if (!m_replies.isEmpty())
            return;

        ui->removeDocsetsButton->setEnabled(selectionModel->hasSelection());

        for (const QModelIndex &index : selectionModel->selectedIndexes()) {
            if (index.data(ListModel::UpdateAvailableRole).toBool()) {
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

    ui->availableDocsetList->setItemDelegate(new ProgressItemDelegate(this));
    connect(m_docsetRegistry, &DocsetRegistry::docsetRemoved, this, [this](const QString name) {
        QListWidgetItem *item = findDocsetListItem(m_availableDocsets[name].title());
        if (!item)
            return;

        item->setHidden(false);
    });
    connect(m_docsetRegistry, &DocsetRegistry::docsetAdded, this, [this](const QString name) {
        QListWidgetItem *item = findDocsetListItem(m_availableDocsets[name].title());
        if (!item)
            return;

        item->setHidden(true);
    });

    // Setup signals & slots
    connect(ui->addFeedButton, &QPushButton::clicked, this, &DocsetsDialog::addDashFeed);
    connect(ui->refreshButton, &QPushButton::clicked, this, &DocsetsDialog::downloadDocsetList);

    connect(m_application, &Core::Application::extractionCompleted,
            this, &DocsetsDialog::extractionCompleted);
    connect(m_application, &Core::Application::extractionError,
            this, &DocsetsDialog::extractionError);
    connect(m_application, &Core::Application::extractionProgress,
            this, &DocsetsDialog::extractionProgress);

    loadDocsetList();
}

DocsetsDialog::~DocsetsDialog()
{
    delete ui;
}

void DocsetsDialog::reject()
{
    if (m_replies.isEmpty() && m_tmpFiles.isEmpty()) {
        QDialog::reject();
        return;
    }

    QMessageBox::information(this, QStringLiteral("Zeal"),
                             tr("An operation is in progress, wait for it to finish, or cancel."));
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
    connect(reply, &QNetworkReply::finished, this, &DocsetsDialog::downloadCompleted);
}

void DocsetsDialog::updateSelectedDocsets()
{
    for (const QModelIndex &index : ui->installedDocsetList->selectionModel()->selectedIndexes()) {
        if (!index.data(Registry::ListModel::UpdateAvailableRole).toBool())
            continue;

        downloadDashDocset(index.data(Registry::ListModel::DocsetNameRole).toString());
    }
}

void DocsetsDialog::updateAllDocsets()
{
    for (const Registry::Docset * const docset : m_docsetRegistry->docsets()) {
        if (!docset->hasUpdate)
            continue;

        downloadDashDocset(docset->name());
    }
}

void DocsetsDialog::removeSelectedDocsets()
{
    QItemSelectionModel *selectonModel = ui->installedDocsetList->selectionModel();
    if (!selectonModel->hasSelection())
        return;

    int ret;
    if (selectonModel->selectedIndexes().count() == 1) {
        const QString docsetTitle = selectonModel->selectedIndexes().first().data().toString();
        ret = QMessageBox::question(this, QStringLiteral("Zeal"),
                                    tr("Remove <b>%1</b> docset?").arg(docsetTitle));
    } else {
        ret = QMessageBox::question(this, QStringLiteral("Zeal"),
                                    tr("Remove <b>%1</b> docsets?")
                                    .arg(selectonModel->selectedIndexes().count()));
    }

    if (ret == QMessageBox::No)
        return;

    QStringList names;
    for (const QModelIndex &index : selectonModel->selectedIndexes())
        names << index.data(Registry::ListModel::DocsetNameRole).toString();
    removeDocsets(names);
}

void DocsetsDialog::updateDocsetFilter(const QString &filterString)
{
    const bool doSearch = !filterString.simplified().isEmpty();

    for (int i = 0; i < ui->availableDocsetList->count(); ++i) {
        QListWidgetItem *item = ui->availableDocsetList->item(i);

        // Skip installed docsets
        if (m_docsetRegistry->contains(item->data(Registry::ListModel::DocsetNameRole).toString()))
            continue;

        item->setHidden(doSearch && !item->text().contains(filterString, Qt::CaseInsensitive));
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

                connect(newReply, &QNetworkReply::finished,
                        this, &DocsetsDialog::downloadCompleted);
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

        // TODO: Verify if scheme can be missing
        if (redirectUrl.scheme().isEmpty())
            redirectUrl.setScheme(reply->request().url().scheme());

        QNetworkReply *newReply = download(redirectUrl);

        // Copy properties
        newReply->setProperty(DocsetNameProperty, reply->property(DocsetNameProperty));
        newReply->setProperty(DownloadTypeProperty, reply->property(DownloadTypeProperty));
        newReply->setProperty(ListItemIndexProperty, reply->property(ListItemIndexProperty));

        connect(newReply, &QNetworkReply::finished, this, &DocsetsDialog::downloadCompleted);

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
        resetProgress();
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
        connect(reply, &QNetworkReply::finished, this, &DocsetsDialog::downloadCompleted);
        break;
    }

    case DownloadDocset: {
        const QString docsetName = reply->property(DocsetNameProperty).toString();
        const Registry::DocsetMetadata metadata = m_availableDocsets.contains(docsetName)
                ? m_availableDocsets[docsetName]
                  : m_userFeeds[docsetName];

        // TODO: Implement an explicit and verbose docset update logic
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

        QTemporaryFile *tmpFile = m_tmpFiles[docsetName];
        if (!tmpFile) {
            tmpFile = new QTemporaryFile(this);
            tmpFile->open();
            m_tmpFiles.insert(docsetName, tmpFile);
        }

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

    displayProgress();
}

void DocsetsDialog::extractionCompleted(const QString &filePath)
{
    QString docsetName;

    // FIXME: Come up with a better approach
    for (const QString &key : m_tmpFiles.keys()) {
        if (m_tmpFiles[key]->fileName() == filePath) {
            docsetName = key;
            break;
        }
    }

    const QDir dataDir(m_application->settings()->docsetPath);
    const QString docsetPath = dataDir.absoluteFilePath(docsetName + QLatin1String(".docset"));

    // Write metadata about docset
    Registry::DocsetMetadata metadata = m_availableDocsets.contains(docsetName)
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

void DocsetsDialog::extractionError(const QString &filePath, const QString &errorString)
{
    const QString docsetName = QFileInfo(filePath).baseName() + QLatin1String(".docset");
    QMessageBox::warning(this, QStringLiteral("Zeal"),
                         tr("Cannot extract docset <b>%1</b>: %2").arg(docsetName, errorString));
    // TODO: Update list item state (hide progress bar)
    delete m_tmpFiles.take(docsetName);
}

void DocsetsDialog::extractionProgress(const QString &filePath, qint64 extracted, qint64 total)
{
    QString docsetName;

    // FIXME: Come up with a better approach
    for (const QString &key : m_tmpFiles.keys()) {
        if (m_tmpFiles[key]->fileName() == filePath) {
            docsetName = key;
            break;
        }
    }

    Registry::DocsetMetadata metadata = m_availableDocsets.contains(docsetName)
            ? m_availableDocsets[docsetName]
              : m_userFeeds[docsetName];

    QListWidgetItem *listItem = findDocsetListItem(metadata.title());
    if (listItem)
        listItem->setData(ProgressItemDelegate::ValueRole, percent(extracted, total));
}

void DocsetsDialog::on_downloadDocsetButton_clicked()
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

        downloadDashDocset(item->data(Registry::ListModel::DocsetNameRole).toString());
    }
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

QListWidgetItem *DocsetsDialog::findDocsetListItem(const QString &title) const
{
    for (int i = 0; i < ui->availableDocsetList->count(); ++i) {
        QListWidgetItem *item = ui->availableDocsetList->item(i);

        if (item->text() == title)
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
    displayProgress();

    QNetworkReply *reply = m_application->download(url);
    connect(reply, &QNetworkReply::downloadProgress, this, &DocsetsDialog::downloadProgress);
    m_replies.append(reply);

    // Installed docsets
    ui->addFeedButton->setEnabled(false);
    ui->updateSelectedDocsetsButton->setEnabled(false);
    ui->updateAllDocsetsButton->setEnabled(false);
    ui->removeDocsetsButton->setEnabled(false);

    // Available docsets
    ui->refreshButton->setEnabled(false);
    ui->downloadDocsetButton->setText(tr("Stop downloads"));

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
    connect(reply, &QNetworkReply::finished, this, &DocsetsDialog::downloadCompleted);
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
        listItem->setData(Registry::ListModel::DocsetNameRole, metadata.name());
        listItem->setCheckState(Qt::Unchecked);

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

    if (!m_availableDocsets.isEmpty())
        ui->downloadableGroup->show();
}

void DocsetsDialog::downloadDashDocset(const QString &name)
{
    if (!m_availableDocsets.contains(name))
        return;

    const QString urlString = RedirectServerUrl + QStringLiteral("/d/com.kapeli/%1/latest");
    QNetworkReply *reply = download(QUrl(urlString.arg(name)));
    reply->setProperty(DocsetNameProperty, name);
    reply->setProperty(DownloadTypeProperty, DownloadDocset);
    reply->setProperty(ListItemIndexProperty,
                       ui->availableDocsetList->row(findDocsetListItem(m_availableDocsets[name].title())));

    connect(reply, &QNetworkReply::finished, this, &DocsetsDialog::downloadCompleted);
}

void DocsetsDialog::removeDocsets(const QStringList &names)
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
                    QMessageBox::warning(this, QStringLiteral("Zeal"),
                                         tr("Cannot delete docset <b>%1</b>!").arg(title));
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

void DocsetsDialog::displayProgress()
{
    ui->docsetsProgress->setValue(percent(m_combinedReceived, m_combinedTotal));
    ui->docsetsProgress->setMaximum(100);
    ui->docsetsProgress->setVisible(!m_replies.isEmpty());
}

void DocsetsDialog::resetProgress()
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
        if (index.data(Registry::ListModel::UpdateAvailableRole).toBool()) {
            hasSelectedUpdates = true;
            break;
        }
    }
    ui->updateSelectedDocsetsButton->setEnabled(hasSelectedUpdates);
    ui->updateAllDocsetsButton->setEnabled(updatesAvailable());
    ui->removeDocsetsButton->setEnabled(selectionModel->hasSelection());

    // Available docsets
    ui->refreshButton->setEnabled(true);
    ui->downloadDocsetButton->setText(tr("Download"));
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
