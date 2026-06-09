// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_DOCSETSDIALOG_H
#define ZEAL_WIDGETUI_DOCSETSDIALOG_H

#include <registry/docsetmetadata.h>
#include <util/caseinsensitivemap.h>

#include <QDialog>
#include <QHash>
#include <QMap>

class QDateTime;
class QListWidgetItem;
class QNetworkReply;
class QTemporaryFile;
class QUrl;

namespace Zeal {

namespace Registry {
class DocsetRegistry;
} // namespace Registry

namespace Core {
class Application;
} // namespace Core

namespace WidgetUi {

namespace Ui {
class DocsetsDialog;
} // namespace Ui

class EmptyStateLabel;

class DocsetsDialog : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DocsetsDialog)
public:
    explicit DocsetsDialog(Core::Application *app, QWidget *parent = nullptr);
    ~DocsetsDialog() override;

private:
    void addDashFeed();
    void updateSelectedDocsets();
    void updateAllDocsets();
    void removeSelectedDocsets();
    void updateDocsetFilter(const QString &filterString);

    void downloadSelectedDocsets();

    void downloadCompleted();
    void downloadProgress(qint64 received, qint64 total);

    void extractionCompleted(const QString &filePath);
    void extractionError(const QString &filePath, const QString &errorString);
    void extractionProgress(const QString &filePath, qint64 extracted, qint64 total);

    void loadDocsetList();

    Ui::DocsetsDialog *ui = nullptr;
    Core::Application *m_application = nullptr;
    Registry::DocsetRegistry *m_docsetRegistry = nullptr;
    EmptyStateLabel *m_installedDocsetsEmptyState = nullptr;
    EmptyStateLabel *m_availableDocsetsEmptyState = nullptr;

    bool m_isStorageReadOnly = false;

    QList<QNetworkReply *> m_replies;

    // TODO: Create a special model
    Util::CaseInsensitiveMap<Registry::DocsetMetadata> m_availableDocsets;
    QMap<QString, Registry::DocsetMetadata> m_userFeeds;

    QHash<QString, QTemporaryFile *> m_tmpFiles;
    QHash<QString, QTemporaryFile *> m_tarixIndexFiles;

    void setupInstalledDocsetsTab();
    void setupAvailableDocsetsTab();
    void updateInstalledDocsetsEmptyState();
    void updateAvailableDocsetsEmptyState();

    void enableControls();
    void disableControls();

    QListWidgetItem *findDocsetListItem(const QString &name) const;
    bool updatesAvailable() const;

    QNetworkReply *download(const QUrl &url);
    void cancelDownloads();

    void loadUserFeedList();
    void downloadDocsetList();
    void processDocsetListReply(QNetworkReply *reply);
    void processDocsetList(const QJsonArray &list);
    void updateDocsetListDownloadTimeLabel(const QDateTime &modifiedTime);

    void downloadDashDocset(const QModelIndex &index);
    void downloadTarixIndex(const QString &docsetName, const QUrl &indexUrl, int attempt);
    void onTarixIndexFailed(QNetworkReply *reply);
    void installDownloadedDocset(const QString &docsetName);
    void removeDocset(const QString &name);

    void updateStatus();

    // Returns the download buffer for a docset, creating it on first use.
    // Returns nullptr if the temporary file cannot be opened.
    QTemporaryFile *docsetTemporaryFile(const QString &docsetName);

    // FIXME: Come up with a better approach
    QString docsetNameForTmpFilePath(const QString &filePath) const;

    static inline int percent(qint64 fraction, qint64 total);

    static QString cacheLocation(const QString &fileName);
    static bool isDirWritable(const QString &path);
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_DOCSETSDIALOG_H
