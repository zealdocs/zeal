#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QCoreApplication>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    const QString buildInfo
            = QString(tr("<strong>Version:</strong> %1<br><strong>Built:</strong> %2"))
            .arg(QCoreApplication::applicationVersion(), QString("%1 %2").arg(__DATE__, __TIME__));

    ui->buildInfoLabel->setText(buildInfo);
    ui->buttonBox->setFocus(Qt::OtherFocusReason);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
