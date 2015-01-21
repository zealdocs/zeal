#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "registry/listmodel.h"
#include "registry/docsetmetadata.h"

#include <QDialog>
#include <QHash>
#include <QUrl>

class QAbstractButton;
class QListWidgetItem;
class QNetworkReply;
class QTemporaryFile;

namespace Ui {
class SettingsDialog;
}

namespace Zeal {

namespace Core {
class Application;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(Core::Application *app, ListModel *listModel, QWidget *parent = 0);
    ~SettingsDialog() override;

signals:
    void refreshRequested();
    void minFontSizeChanged(int minFont);
    void webPageStyleUpdated();

private slots:
    void extractionCompleted(const QString &filePath);
    void extractionError(const QString &filePath, const QString &errorString);

    void downloadCompleted();

    void on_downloadProgress(quint64 received, quint64 total);
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

    void startTasks(qint8 tasks = 1);
    void endTasks(qint8 tasks = 1);
    void displayProgress();
    void loadSettings();
    void updateFeedDocsets();
    void resetProgress();
    QNetworkReply *startDownload(const QUrl &url);
    void stopDownloads();
    void saveSettings();

    enum DocsetProgressRoles {
        ZealDocsetDoneInstalling = Qt::UserRole + 20
    };

    Ui::SettingsDialog *ui = nullptr;
    Core::Application *m_application = nullptr;

    ListModel *m_zealListModel = nullptr;
    QList<QNetworkReply *> replies;
    QHash<QNetworkReply *, QPair<qint32, qint32> *> progress;
    qint32 totalDownload = 0;
    qint32 currentDownload = 0;
    qint32 tasksRunning = 0;
};

} // namespace Zeal

#endif // SETTINGSDIALOG_H
