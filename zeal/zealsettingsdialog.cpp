#include "zealsettingsdialog.h"
#include "ui_zealsettingsdialog.h"

ZealSettingsDialog::ZealSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZealSettingsDialog)
{
    ui->setupUi(this);
}

ZealSettingsDialog::~ZealSettingsDialog()
{
    delete ui;
}
