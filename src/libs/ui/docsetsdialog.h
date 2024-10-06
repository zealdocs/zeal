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
}

namespace WidgetUi {

namespace Ui {
class DocsetsDialog;
} // namespace Ui

class DocsetsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DocsetsDialog(Core::Application *app, QWidget *parent = nullptr);
    ~DocsetsDialog() override;

private slots:
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

private:
    enum DownloadType {
        DownloadDashFeed,
        DownloadDocset,
        DownloadDocsetList
    };

    Ui::DocsetsDialog *ui = nullptr;
    Core::Application *m_application = nullptr;
    Registry::DocsetRegistry *m_docsetRegistry = nullptr;

    bool m_isStorageReadOnly = false;

    QList<QNetworkReply *> m_replies;

    // TODO: Create a special model
    Util::CaseInsensitiveMap<Registry::DocsetMetadata> m_availableDocsets;
    QMap<QString, Registry::DocsetMetadata> m_userFeeds;

    QHash<QString, QTemporaryFile *> m_tmpFiles;

    void setupInstalledDocsetsTab();
    void setupAvailableDocsetsTab();

    void enableControls();
    void disableControls();

    QListWidgetItem *findDocsetListItem(const QString &name) const;
    bool updatesAvailable() const;

    QNetworkReply *download(const QUrl &url);
    void cancelDownloads();

    void loadUserFeedList();
    void downloadDocsetList();
    void processDocsetList(const QJsonArray &list);

    void downloadDashDocset(const QModelIndex &index);
    void removeDocset(const QString &name);

    void updateStatus();

    // FIXME: Come up with a better approach
    QString docsetNameForTmpFilePath(const QString &filePath) const;

    static inline int percent(qint64 fraction, qint64 total);

    static QString cacheLocation(const QString &fileName);
    static bool isDirWritable(const QString &path);
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_DOCSETSDIALOG_H
