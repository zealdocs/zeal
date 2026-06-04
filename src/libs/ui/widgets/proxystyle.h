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
// Additionally left-aligns labels in the browser tab bar.
class ProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit ProxyStyle(QStyle *baseStyle = nullptr);

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override;

    void drawControl(ControlElement element,
                     const QStyleOption *option,
                     QPainter *painter,
                     const QWidget *widget = nullptr) const override;

    void drawItemText(QPainter *painter,
                      const QRect &rect,
                      int flags,
                      const QPalette &pal,
                      bool enabled,
                      const QString &text,
                      QPalette::ColorRole textRole = QPalette::NoRole) const override;

private:
    // Set while the base style draws a browser tab label, whose centered text
    // alignment is hardcoded and only reachable via proxied drawItemText().
    mutable bool m_leftAlignItemText = false;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_PROXYSTYLE_H
