#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "registry/docsetmetadata.h"

#include <QDialog>
#include <QHash>
#include <QMap>
#include <QUrl>

class QAbstractButton;
class QListWidgetItem;
class QNetworkReply;
class QTemporaryFile;

namespace Ui {
class SettingsDialog;
}

namespace Zeal {

class DocsetRegistry;
class ListModel;

namespace Core {
class Application;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(Core::Application *app, ListModel *listModel, QWidget *parent = nullptr);
    ~SettingsDialog() override;

private slots:
    void extractionCompleted(const QString &filePath);
    void extractionError(const QString &filePath, const QString &errorString);

    void downloadCompleted();

    void on_downloadProgress(qint64 received, qint64 total);
    void on_downloadDocsetButton_clicked();
    void on_storageButton_clicked();
    void on_deleteButton_clicked();
    void on_listView_clicked(const QModelIndex &index);
    void on_tabWidget_currentChanged(int current);
    void on_docsetsList_itemSelectionChanged();
    void addDashFeed();

private:
    enum DownloadType {
        DownloadDashFeed,
        DownloadDocset,
        DownloadDocsetList
    };

    QListWidgetItem *findDocsetListItem(const QString &title) const;

    /// TODO: Create a special model
    QMap<QString, DocsetMetadata> m_availableDocsets;
    QMap<QString, DocsetMetadata> m_userFeeds;

    QHash<QString, QTemporaryFile *> m_tmpFiles;

    void downloadDocsetList();
    void processDocsetList(const QJsonArray &list);
    void downloadDashDocset(const QString &name);

    void startTasks();
    void endTasks();
    void displayProgress();
    void loadSettings();
    void updateFeedDocsets();
    void resetProgress();
    QNetworkReply *startDownload(const QUrl &url);
    void stopDownloads();
    void saveSettings();

    Ui::SettingsDialog *ui = nullptr;
    Core::Application *m_application = nullptr;
    DocsetRegistry *m_docsetRegistry = nullptr;

    ListModel *m_zealListModel = nullptr;
    QList<QNetworkReply *> replies;
    qint64 totalDownload = 0;
    qint64 currentDownload = 0;
};

} // namespace Zeal

#endif // SETTINGSDIALOG_H
