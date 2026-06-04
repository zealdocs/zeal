// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tabbar.h"

namespace {
// Tab width bounds in multiples of the 'M' advance, so they scale with the font.
constexpr int MaxTabWidthEm = 20;
constexpr int MinTabWidthEm = 6;
} // namespace

namespace Zeal::WidgetUi {

TabBar::TabBar(QWidget *parent)
    : QTabBar(parent)
{
}

QSize TabBar::tabSizeHint(int index) const
{
    QSize hint = QTabBar::tabSizeHint(index);

    const int em = fontMetrics().horizontalAdvance(QLatin1Char('M'));
    const int sharedWidth = count() > 0 ? width() / count() : MaxTabWidthEm * em;
    hint.setWidth(qBound(MinTabWidthEm * em, sharedWidth, MaxTabWidthEm * em));
    return hint;
}

} // namespace Zeal::WidgetUi
