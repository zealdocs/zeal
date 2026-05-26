// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_PROXYSTYLE_H
#define ZEAL_WIDGETUI_PROXYSTYLE_H

#include <QProxyStyle>

namespace Zeal::WidgetUi {

// Wraps the platform style and renders selected primitives with Tabler glyphs so
// they match the rest of the application's icon set. Currently this restyles the
// tab bar close button; everything else is left to the base style, and a real
// desktop icon theme (non-empty QIcon::themeName()) still takes precedence.
class ProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit ProxyStyle(QStyle *baseStyle = nullptr);

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_PROXYSTYLE_H
