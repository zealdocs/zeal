#ifndef ZEALSETTINGSDIALOG_H
#define ZEALSETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class ZealSettingsDialog;
}

class ZealSettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ZealSettingsDialog(QWidget *parent = 0);
    ~ZealSettingsDialog();
    
private:
    Ui::ZealSettingsDialog *ui;
};

#endif // ZEALSETTINGSDIALOG_H
