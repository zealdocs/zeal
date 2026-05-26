// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iconhelper.h"

#include <QApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPixmapCache>
#include <QSvgRenderer>

#include <utility>

namespace Zeal::WidgetUi::IconHelper {

namespace {

// A QIconEngine that renders a monochrome SVG and recolors it to the active
// palette. Unlike QIcon's built-in SVG engine, which paints stroke="currentColor"
// as opaque black, this keeps the glyph visible on dark color schemes and fades
// it correctly when the owning widget is disabled.
class TintedSvgIconEngine final : public QIconEngine
{
public:
    explicit TintedSvgIconEngine(QString resourcePath)
        : m_resourcePath(std::move(resourcePath))
    {
    }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        painter->drawPixmap(rect, scaledPixmap(rect.size(), mode, state, painter->device()->devicePixelRatioF()));
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        // Without an explicit scale, render at the application's device pixel ratio
        // so menu and toolbar icons stay sharp on high-DPI displays.
        return scaledPixmap(size, mode, state, qApp ? qApp->devicePixelRatio() : 1.0);
    }

    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale) override
    {
        Q_UNUSED(state)

        const QSize deviceSize = size * scale;
        if (deviceSize.isEmpty()) {
            return {};
        }

        const QColor tint = tintColor(mode);
        const QString key = QStringLiteral("zeal:tinted-svg:%1:%2x%3@%4:%5")
                                .arg(m_resourcePath)
                                .arg(deviceSize.width())
                                .arg(deviceSize.height())
                                .arg(scale)
                                .arg(tint.rgba());

        QPixmap pixmap;
        if (!QPixmapCache::find(key, &pixmap)) {
            pixmap = QPixmap(deviceSize);
            pixmap.fill(Qt::transparent);

            QSvgRenderer renderer(m_resourcePath);
            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            // Render into explicit device-pixel bounds so the 24×24 viewBox always
            // fills the icon rect rather than relying on the painter's viewport.
            renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(deviceSize)));
            // Replace the rendered (black) glyph with the palette color, keeping its alpha.
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(pixmap.rect(), tint);
            painter.end();

            pixmap.setDevicePixelRatio(scale);
            QPixmapCache::insert(key, pixmap);
        }

        return pixmap;
    }

    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        Q_UNUSED(mode)
        Q_UNUSED(state)
        return size;
    }

    QIconEngine *clone() const override { return new TintedSvgIconEngine(*this); }

private:
    static QColor tintColor(QIcon::Mode mode)
    {
        const QPalette palette = QApplication::palette();
        switch (mode) {
        case QIcon::Disabled:
            return palette.color(QPalette::Disabled, QPalette::ButtonText);
        case QIcon::Selected:
            return palette.color(QPalette::Active, QPalette::HighlightedText);
        case QIcon::Active:
        case QIcon::Normal:
            break;
        }
        return palette.color(QPalette::Normal, QPalette::ButtonText);
    }

    QString m_resourcePath;
};

} // namespace

bool useThemeIcons()
{
#ifdef Q_OS_WINDOWS
    // Windows has no desktop icon theme, and Qt 6.8+ ships a built-in one that
    // would otherwise mask our glyphs. Always use Tabler there.
    return false;
#else
    // Honor a real desktop icon theme when one is active (e.g. Linux KDE/GNOME);
    // otherwise (macOS, themeless Linux) use Tabler.
    return !QIcon::themeName().isEmpty();
#endif
}

QIcon fromTheme(const QString &themeName, const QString &fallbackSvgResource)
{
    if (useThemeIcons()) {
        return QIcon::fromTheme(themeName, QIcon(new TintedSvgIconEngine(fallbackSvgResource)));
    }
    return QIcon(new TintedSvgIconEngine(fallbackSvgResource));
}

} // namespace Zeal::WidgetUi::IconHelper
