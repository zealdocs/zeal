// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_ABOUTDIALOG_H
#define ZEAL_WIDGETUI_ABOUTDIALOG_H

#include <QDialog>

namespace Zeal::WidgetUi {

class AboutDialog : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AboutDialog)
public:
    explicit AboutDialog(QWidget *parent = nullptr);
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_ABOUTDIALOG_H
