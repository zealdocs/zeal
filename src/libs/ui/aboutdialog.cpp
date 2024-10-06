/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <core/application.h>

using namespace Zeal::WidgetUi;

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->versionLabel->setText(Core::Application::versionString());
    ui->buttonBox->setFocus(Qt::OtherFocusReason);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
