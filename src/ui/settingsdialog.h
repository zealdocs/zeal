#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "registry/listmodel.h"
#include "registry/docsetmetadata.h"

#include <QDialog>
#include <QHash>
#include <QUrl>

class QAbstractButton;
class QNetworkReply;

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
    void downloadDocsetList();
    void extractDocset();
    void downloadDocsetLists();

    void on_downloadProgress(quint64 received, quint64 total);
    void on_downloadButton_clicked();
    // void on_docsetsList_clicked(const QModelIndex &index);
    void on_downloadDocsetButton_clicked();
    void on_storageButton_clicked();
    void on_deleteButton_clicked();
    void on_listView_clicked(const QModelIndex &index);
    void on_tabWidget_currentChanged(int current);
    void on_buttonBox_accepted();
    void on_minFontSize_valueChanged(int arg1);
    void on_buttonBox_rejected();
    void on_docsetsList_itemSelectionChanged();
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_updateButton_clicked();
    void on_addFeedButton_clicked();

private:
    void startTasks(qint8 tasks = 1);
    void endTasks(qint8 tasks = 1);
    void displayProgress();
    void loadSettings();
    void updateDocsets();
    void resetProgress();
    QNetworkReply *startDownload(const QUrl &url);
    QNetworkReply *startDownload(const DocsetMetadata &meta);
    void stopDownloads();
    void saveSettings();
    QString tarPath() const;

    enum DocsetProgressRoles {
        ZealDocsetDoneInstalling = Qt::UserRole + 20,
    };

    Ui::SettingsDialog *ui = nullptr;
    Core::Application *m_application = nullptr;

    ListModel *m_zealListModel = nullptr;
    bool downloadedDocsetsList = false;
    QMap<QString, DocsetMetadata> availMetadata;
    QList<QNetworkReply *> replies;
    QHash<QNetworkReply *, QPair<qint32, qint32> *> progress;
    qint32 totalDownload = 0;
    qint32 currentDownload = 0;
    qint32 tasksRunning = 0;
};

} // namespace Zeal

#endif // SETTINGSDIALOG_H
