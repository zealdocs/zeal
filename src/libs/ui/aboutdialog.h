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

#ifndef ZEAL_WIDGETUI_ABOUTDIALOG_H
#define ZEAL_WIDGETUI_ABOUTDIALOG_H

#include <QDialog>

namespace Zeal {
namespace WidgetUi {

namespace Ui {
class AboutDialog;
} // namespace Ui

class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() override;

private:
    Ui::AboutDialog *ui;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_ABOUTDIALOG_H
