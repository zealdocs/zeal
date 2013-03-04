#include "zealsettingsdialog.h"

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

void ZealSettingsDialog::setHotKey(const QKeySequence &keySequence)
{
    ui->toolButton->setKeySequence(keySequence);
}

QKeySequence ZealSettingsDialog::hotKey()
{
    return ui->toolButton->keySequence();
}
