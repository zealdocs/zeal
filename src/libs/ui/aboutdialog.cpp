// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <core/application.h>

#include <QIcon>

namespace Zeal::WidgetUi {

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    const QIcon logo = QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.svg")));
    ui->logoLabel->setPixmap(logo.pixmap(QSize(64, 64)));

    ui->versionLabel->setText(Core::Application::versionString());
    ui->buttonBox->setFocus(Qt::OtherFocusReason);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

} // namespace Zeal::WidgetUi
