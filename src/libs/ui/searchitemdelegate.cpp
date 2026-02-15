// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchitemdelegate.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

using namespace Zeal::WidgetUi;

namespace {
    constexpr double SCORE_GAP_LEADING = -0.005;
    constexpr double SCORE_GAP_TRAILING = -0.005;
    constexpr double SCORE_GAP_INNER = -0.01;
    constexpr double SCORE_MATCH_CONSECUTIVE = 1.0;
    constexpr double SCORE_MATCH_SLASH = 0.9;
    constexpr double SCORE_MATCH_WORD = 0.8;
    constexpr double SCORE_MATCH_CAPITAL = 0.7;
    constexpr double SCORE_MATCH_DOT = 0.6;
    constexpr int FZY_MAX_LEN = 1024;

    inline bool isLower(char c) { return c >= 'a' && c <= 'z'; }
    inline bool isUpper(char c) { return c >= 'A' && c <= 'Z'; }

    void precomputeBonus(const char *haystack, int length, double *matchBonus)
    {
        char lastCh = '/';
        for (int i = 0; i < length; ++i) {
            const char ch = haystack[i];

            if (lastCh == '/') {
                matchBonus[i] = SCORE_MATCH_SLASH;
            } else if (lastCh == '-' || lastCh == '_' || lastCh == ' ') {
                matchBonus[i] = SCORE_MATCH_WORD;
            } else if (lastCh == '.') {
                matchBonus[i] = SCORE_MATCH_DOT;
            } else if (isLower(lastCh) && isUpper(ch)) {
                matchBonus[i] = SCORE_MATCH_CAPITAL;
            } else {
                matchBonus[i] = 0.0;
            }

            lastCh = ch;
        }
    }

    // Compute fuzzy match positions using a simpler greedy approach.
    // Returns true if a match is found, and fills matchPositions with the haystack indices.
    bool matchPositionsFzy(const char *needle, int needleLen, const char *haystack, int haystackLen, int *matchPositions)
    {
        if (needleLen == 0 || haystackLen == 0 || needleLen > haystackLen) {
            return false;
        }

        if (haystackLen > FZY_MAX_LEN || needleLen > FZY_MAX_LEN) {
            return false;
        }

        // Simple greedy matching: find each needle character in order
        int haystackPos = 0;
        for (int i = 0; i < needleLen; ++i) {
            bool found = false;
            for (int j = haystackPos; j < haystackLen; ++j) {
                if (needle[i] == haystack[j]) {
                    matchPositions[i] = j;
                    haystackPos = j + 1;
                    found = true;
                    break;
                }
            }

            if (!found) {
                return false;
            }
        }

        return true;
    }
}

SearchItemDelegate::SearchItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QList<int> SearchItemDelegate::decorationRoles() const
{
    return m_decorationRoles;
}

void SearchItemDelegate::setDecorationRoles(const QList<int> &roles)
{
    m_decorationRoles = roles;
}

bool SearchItemDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view,
                                   const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() != QEvent::ToolTip)
        return QStyledItemDelegate::helpEvent(event, view, option, index);

    if (sizeHint(option, index).width() < view->visualRect(index).width()) {
        QToolTip::hideText();
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }

    QToolTip::showText(event->globalPos(), index.data().toString(), view);
    return true;
}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);

    QStyle *style = opt.widget->style();

    // Find decoration roles with data present.
    QList<int> roles;
    for (int role : m_decorationRoles) {
        if (!index.data(role).isNull())
            roles.append(role);
    }

    // TODO: Implemented via initStyleOption() overload
    if (!roles.isEmpty()) {
        opt.features |= QStyleOptionViewItem::HasDecoration;
        opt.icon = index.data(roles.first()).value<QIcon>();

        const QSize actualSize = opt.icon.actualSize(opt.decorationSize);
        opt.decorationSize = {std::min(opt.decorationSize.width(), actualSize.width()),
                              std::min(opt.decorationSize.height(), actualSize.height())};
    }

    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, opt.widget) + 1;

    if (!roles.isEmpty()) {
        QIcon::Mode mode = QIcon::Normal;
        if (!(opt.state & QStyle::State_Enabled))
            mode = QIcon::Disabled;
        else if (opt.state & QStyle::State_Selected)
            mode = QIcon::Selected;
        const QIcon::State state = (opt.state & QStyle::State_Open) ? QIcon::On : QIcon::Off;

        // All icons are sized after the first one.
        QRect iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, opt.widget);
        // Undo RTL mirroring
        iconRect = style->visualRect(opt.direction, opt.rect, iconRect);
        const int dx = iconRect.width() + margin;

        for (int i = 1; i < roles.size(); ++i) {
            opt.decorationSize.rwidth() += dx;
            iconRect.translate(dx, 0);
            // Redo RTL mirroring
            const auto iconVisualRect = style->visualRect(opt.direction, opt.rect, iconRect);

            const QIcon icon = index.data(roles[i]).value<QIcon>();
            icon.paint(painter, iconVisualRect, opt.decorationAlignment, mode, state);
        }
    }

    // This should not happen unless a docset is corrupted.
    if (index.data().isNull())
        return;

    // Match QCommonStyle behavior.
    opt.features |= QStyleOptionViewItem::HasDisplay;
    opt.text = index.data().toString();
    const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget)
            .adjusted(margin, 0, -margin, 0);
    const QFontMetrics &fm = opt.fontMetrics;
    const QString elidedText = fm.elidedText(opt.text, opt.textElideMode, textRect.width());

    if (!m_highlight.isEmpty()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QColor::fromRgb(255, 253, 0));

        const QColor highlightColor
                = (opt.state & (QStyle::State_Selected | QStyle::State_HasFocus))
                ? QColor::fromRgb(255, 255, 100, 20) : QColor::fromRgb(255, 255, 100, 120);

        // Normalize needle and haystack the same way as in docset.cpp
        const QByteArray needleBytes = m_highlight.toUtf8();
        const QByteArray haystackBytes = opt.text.toUtf8();
        const int needleLength = needleBytes.length();
        const int haystackLength = haystackBytes.length();

        if (needleLength > 0 && haystackLength > 0 && needleLength <= FZY_MAX_LEN && haystackLength <= FZY_MAX_LEN) {
            char needle[FZY_MAX_LEN];
            char haystack[FZY_MAX_LEN];

            // Normalize needle
            for (int i = 0; i < needleLength; ++i) {
                const char c = needleBytes[i];
                if ((i > 0 && needleBytes[i - 1] == ':' && c == ':') // C++ (::)
                        || c == '/' || c == '_' || c == ' ') {
                    needle[i] = '.';
                } else if (c >= 'A' && c <= 'Z') {
                    needle[i] = c + 32;
                } else {
                    needle[i] = c;
                }
            }

            // Normalize haystack
            for (int i = 0; i < haystackLength; ++i) {
                const char c = haystackBytes[i];
                if ((i > 0 && haystackBytes[i - 1] == ':' && c == ':') // C++ (::)
                        || c == '/' || c == '_' || c == ' ') {
                    haystack[i] = '.';
                } else if (c >= 'A' && c <= 'Z') {
                    haystack[i] = c + 32;
                } else {
                    haystack[i] = c;
                }
            }

            // Try exact substring match first
            int exactMatchIndex = -1;
            for (int i = 0; i <= haystackLength - needleLength; ++i) {
                if (std::memcmp(haystack + i, needle, needleLength) == 0) {
                    exactMatchIndex = i;
                    break;
                }
            }

            if (exactMatchIndex != -1) {
                // Exact match: highlight the contiguous substring
                const int elidedLen = static_cast<int>(elidedText.length());
                if (exactMatchIndex < elidedLen - 1) {
                    QRect highlightRect = textRect.adjusted(fm.horizontalAdvance(elidedText.left(exactMatchIndex)), 2, 0, -2);
                    const int highlightLen = std::min(needleLength, elidedLen - exactMatchIndex);
                    highlightRect.setWidth(fm.horizontalAdvance(elidedText.mid(exactMatchIndex, highlightLen)));

                    QPainterPath path;
                    path.addRoundedRect(highlightRect, 2, 2);
                    painter->fillPath(path, highlightColor);
                    painter->drawPath(path);
                }
            } else {
                // Fuzzy match: highlight individual character positions
                int matchPositions[FZY_MAX_LEN];
                if (matchPositionsFzy(needle, needleLength, haystack, haystackLength, matchPositions)) {
                    // Group consecutive positions to reduce number of highlight rectangles
                    const int elidedLen = static_cast<int>(elidedText.length());
                    int startPos = matchPositions[0];
                    int endPos = matchPositions[0];

                    for (int i = 1; i <= needleLength; ++i) {
                        const bool isLast = (i == needleLength);
                        const bool isConsecutive = !isLast && (matchPositions[i] == endPos + 1);

                        if (isConsecutive) {
                            endPos = matchPositions[i];
                        } else {
                            // Draw highlight for [startPos, endPos]
                            if (startPos < elidedLen) {
                                const int highlightStart = startPos;
                                const int highlightLen = std::min(endPos - startPos + 1, elidedLen - startPos);

                                QRect highlightRect = textRect.adjusted(fm.horizontalAdvance(elidedText.left(highlightStart)), 2, 0, -2);
                                highlightRect.setWidth(fm.horizontalAdvance(elidedText.mid(highlightStart, highlightLen)));

                                QPainterPath path;
                                path.addRoundedRect(highlightRect, 2, 2);
                                painter->fillPath(path, highlightColor);
                                painter->drawPath(path);
                            }

                            if (!isLast) {
                                startPos = matchPositions[i];
                                endPos = matchPositions[i];
                            }
                        }
                    }
                }
            }
        }

        painter->restore();
    }

    painter->save();

#ifdef Q_OS_WINDOWS
    // QWindowsVistaStyle overrides highlight color.
    if (style->objectName() == QLatin1String("windowsvista")) {
        opt.palette.setColor(QPalette::All, QPalette::HighlightedText,
                             opt.palette.color(QPalette::Active, QPalette::Text));
    }
#endif

    const QPalette::ColorGroup cg = (opt.state & QStyle::State_Active)
            ? QPalette::Normal : QPalette::Inactive;

    if (opt.state & QStyle::State_Selected)
        painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
    else
        painter->setPen(opt.palette.color(cg, QPalette::Text));

    // Constant LeftToRight because we don't need to flip it any further.
    // Vertically align the text in the middle to match QCommonStyle behaviour.
    const auto alignedRect = QStyle::alignedRect(Qt::LeftToRight, opt.displayAlignment,
                                                 QSize(textRect.size().width(), fm.height()), textRect);
    const auto textPoint = QPoint(alignedRect.x(), alignedRect.y() + fm.ascent());
    // Force LTR, so that BiDi won't reorder ellipsis to the left.
    painter->drawText(textPoint, elidedText, Qt::TextFlag::TextForceLeftToRight, 0);
    painter->restore();
}

QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);

    QStyle *style = opt.widget->style();

    QSize size = QStyledItemDelegate::sizeHint(opt, index);
    size.setWidth(0);

    const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, opt.widget) + 1;

    // Find decoration roles with data present.
    QList<int> roles;
    for (int role : m_decorationRoles) {
        if (!index.data(role).isNull())
            roles.append(role);
    }

    if (!roles.isEmpty()) {
        const QIcon icon = index.data(roles.first()).value<QIcon>();
        const QSize actualSize = icon.actualSize(opt.decorationSize);
        const int decorationWidth = std::min(opt.decorationSize.width(), actualSize.width());
        size.rwidth() = (decorationWidth + margin) * roles.size() + margin;
    }

    size.rwidth() += opt.fontMetrics.horizontalAdvance(index.data().toString()) + margin * 2;

    return size;
}

void SearchItemDelegate::setHighlight(const QString &text)
{
    m_highlight = text;
}
