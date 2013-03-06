#ifndef ZEALSETTINGSDIALOG_H
#define ZEALSETTINGSDIALOG_H

#include <QDialog>
#include "ui_zealsettingsdialog.h"

class ZealSettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ZealSettingsDialog(QWidget *parent = 0);
    ~ZealSettingsDialog();
    void setHotKey(const QKeySequence &keySequence);
    QKeySequence hotKey();
    
    Ui::ZealSettingsDialog *ui;
};

#endif // ZEALSETTINGSDIALOG_H
