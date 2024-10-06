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

#ifndef ZEAL_WIDGETUI_TOOLBARFRAME_H
#define ZEAL_WIDGETUI_TOOLBARFRAME_H

#include <QWidget>

namespace Zeal {
namespace WidgetUi {

class ToolBarFrame : public QWidget
{
    Q_OBJECT
public:
    explicit ToolBarFrame(QWidget *parent = nullptr);

private:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_TOOLBARFRAME_H
