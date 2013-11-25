#ifndef ZEALSETTINGSDIALOG_H
#define ZEALSETTINGSDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QSettings>

#include "ui_zealsettingsdialog.h"
#include "zeallistmodel.h"
#include "zealdocsetsregistry.h"

class ZealSettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ZealSettingsDialog(ZealListModel &zlist, QWidget *parent = 0);
    ~ZealSettingsDialog();
    void setHotKey(const QKeySequence &keySequence);
    QKeySequence hotKey();
    
    Ui::ZealSettingsDialog *ui;

private:
    void loadSettings();
    void progressCb(quint64 recv, quint64 total);
    void DownloadCompleteCb(QNetworkReply *reply);
signals:
    void refreshRequested();
    void minFontSizeChanged(int minFont);
private slots:
    void on_downloadButton_clicked();

    void on_docsetsList_clicked(const QModelIndex &index);

    void on_downloadDocsetButton_clicked();

    void on_storageButton_clicked();

    void on_deleteButton_clicked();

    void on_listView_clicked(const QModelIndex &index);

    void on_tabWidget_currentChanged(int index);

    void on_buttonBox_accepted();

    void on_minFontSize_valueChanged(int arg1);

    void on_buttonBox_rejected();

private:
    ZealListModel &zealList;
    QNetworkAccessManager naManager;
    QSettings settings;
    int naCount;
    QMap<QString, QString> urls;

};

#endif // ZEALSETTINGSDIALOG_H
