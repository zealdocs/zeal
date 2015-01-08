#ifndef ZEALSETTINGSDIALOG_H
#define ZEALSETTINGSDIALOG_H

#include "zeallistmodel.h"
#include "zealdocsetmetadata.h"
#include "ui_zealsettingsdialog.h"

#include <QDialog>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QSettings>
#include <QUrl>

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
    void startTasks(qint8 tasks = 1);
    void endTasks(qint8 tasks = 1);
    void displayProgress();
    void loadSettings();
    void updateDocsets();
    void resetProgress();
    QNetworkReply *startDownload(const QUrl &url, qint8 retries = 0);
    QNetworkReply *startDownload(const ZealDocsetMetadata &meta, qint8 retries = 0);
    void stopDownloads();
    void saveSettings();
    const QString tarPath() const;

    enum DocsetProgressRoles {
        ZealDocsetDoneInstalling = Qt::UserRole + 20,
    };

    // Stores prefix used to activate a specific docset.
    QHash<QString, QVariant> prefixes;

signals:
    void refreshRequested();
    void minFontSizeChanged(int minFont);
    void webPageStyleUpdated();

private slots:
    void downloadDocsetList();
    void extractDocset();
    void downloadDocsetLists();

    void on_downloadProgress(quint64 recv, quint64 total);

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
    ZealListModel &zealList;
    QNetworkAccessManager naManager;
    QSettings settings;
    bool downloadedDocsetsList;
    QMap<QString, ZealDocsetMetadata> availMetadata;
    QHash<QNetworkReply *, qint8> replies;
    QHash<QNetworkReply *, QPair<qint32, qint32> *> progress;
    qint32 totalDownload;
    qint32 currentDownload;
    qint32 tasksRunning;

    ZealSettingsDialog::ProxyType proxyType() const;
    void setProxyType(ZealSettingsDialog::ProxyType type);
};

Q_DECLARE_METATYPE(ZealSettingsDialog::ProxyType)

#endif // ZEALSETTINGSDIALOG_H
