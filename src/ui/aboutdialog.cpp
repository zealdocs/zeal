/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Contact: http://zealdocs.org/contact.html
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QCoreApplication>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(std::unique_ptr<Ui::AboutDialog>(new Ui::AboutDialog))
{
    ui->setupUi(this);

    const QString buildInfo = QString(tr("<strong>Version:</strong> %1<br>"))
            .arg(QCoreApplication::applicationVersion());

    ui->buildInfoLabel->setText(buildInfo);
    ui->buttonBox->setFocus(Qt::OtherFocusReason);
}

AboutDialog::~AboutDialog()
{
}
