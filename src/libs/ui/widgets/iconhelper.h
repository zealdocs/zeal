// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_ICONHELPER_H
#define ZEAL_WIDGETUI_ICONHELPER_H

#include <QIcon>

namespace Zeal::WidgetUi::IconHelper {

// Whether the platform's desktop icon theme should be preferred over the bundled
// Tabler glyphs. False on Windows (always Tabler, never the Qt-bundled theme) and
// when no desktop icon theme is active; true on a themed desktop (Linux KDE/GNOME).
bool useThemeIcons();

// Returns the platform icon theme entry named themeName when useThemeIcons() is
// true, otherwise a monochrome Tabler SVG loaded from fallbackSvgResource and
// tinted to the active palette so the glyph stays legible in both light and dark
// color schemes. Use freedesktop icon naming for themeName (e.g. "go-previous").
QIcon fromTheme(const QString &themeName, const QString &fallbackSvgResource);

// Returns a monochrome Tabler SVG loaded from svgResource and tinted to the active
// palette, without any desktop icon theme lookup. Use this for glyphs that have no
// suitable freedesktop equivalent (e.g. the tab loading placeholder).
QIcon fromResource(const QString &svgResource);

} // namespace Zeal::WidgetUi::IconHelper

#endif // ZEAL_WIDGETUI_ICONHELPER_H
