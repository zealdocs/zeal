#ifndef ZEALSETTINGSDIALOG_H
#define ZEALSETTINGSDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QInputDialog>
#include <QtNetwork/QNetworkProxy>
#include <QSettings>
#include <QClipboard>
#include <QUrl>

#include "ui_zealsettingsdialog.h"
#include "zeallistmodel.h"
#include "zealdocsetsregistry.h"
#include "zealdocsetmetadata.h"

class ZealSettingsDialog : public QDialog
{
    Q_OBJECT

    Q_ENUMS(ProxyType)
    
public:
    explicit ZealSettingsDialog(ZealListModel &zlist, QWidget *parent = 0);
    ~ZealSettingsDialog();
    void setHotKey(const QKeySequence &keySequence);
    QKeySequence hotKey();
    
    Ui::ZealSettingsDialog *ui;

    enum ProxyType {
        NoProxy,
        SystemProxy,
        UserDefinedProxy
    };

    QNetworkProxy httpProxy() const;

protected:
    void showEvent(QShowEvent *);

private:
    void startTasks(qint8 tasks);
    void endTasks(qint8 tasks);
    void displayProgress();
    void loadSettings();
    void updateDocsets();
    void resetProgress();
    QNetworkReply *startDownload(const QUrl &url, qint8 retries = 0);
    QNetworkReply *startDownload(const ZealDocsetMetadata &meta, qint8 retries = 0);
    void stopDownloads();
    void saveSettings();

    enum DocsetProgressRoles {
        ZealDocsetDoneInstalling = Qt::UserRole + 20,
    };

signals:
    void refreshRequested();
    void minFontSizeChanged(int minFont);

private slots:
    void downloadDocsetList();
    void extractDocset();
    void downloadDocsetLists();

    void on_downloadProgress(quint64 recv, quint64 total);

    void on_downloadButton_clicked();

    //void on_docsetsList_clicked(const QModelIndex &index);

    void on_downloadDocsetButton_clicked();

    void on_storageButton_clicked();

    void on_deleteButton_clicked();

    void on_listView_clicked(const QModelIndex &index);

    void on_tabWidget_currentChanged();

    void on_buttonBox_accepted();

    void on_minFontSize_valueChanged(int arg1);

    void on_buttonBox_rejected();

    void on_docsetsList_itemSelectionChanged();

    void on_buttonBox_clicked(QAbstractButton *button);

    void on_updateButton_clicked();

    void on_addFeedButton_clicked();

private:
    ZealListModel &zealList;
    QNetworkAccessManager naManager;
    QSettings settings;
    bool downloadedDocsetsList;
    QMap<QString, ZealDocsetMetadata> availMetadata;
    QHash<QNetworkReply*, qint8> replies;
    QHash<QNetworkReply*, QPair<qint32, qint32>*> progress;
    qint32 totalDownload;
    qint32 currentDownload;
    qint32 tasksRunning;

    ZealSettingsDialog::ProxyType proxyType() const;
    void setProxyType(ZealSettingsDialog::ProxyType type);
};

Q_DECLARE_METATYPE(ZealSettingsDialog::ProxyType)

#endif // ZEALSETTINGSDIALOG_H
