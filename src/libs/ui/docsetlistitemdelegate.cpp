// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docsetlistitemdelegate.h"

#include <registry/itemdatarole.h>

#include <QPainter>
#include <QProgressBar>

using namespace Zeal::WidgetUi;

namespace {
constexpr int ProgressBarWidth = 150;
} // namespace

DocsetListItemDelegate::DocsetListItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void DocsetListItemDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (index.model()->data(index, ShowProgressRole).toBool()) {
        paintProgressBar(painter, option, index);
        return;
    }

    if (index.column() != Registry::SectionIndex::Actions) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);

    if (!index.model()->data(index, Registry::ItemDataRole::UpdateAvailableRole).toBool()) {
        return;
    }

    const QString text = tr("Update available");

    QFont font(painter->font());
    font.setItalic(true);

    const QFontMetrics fontMetrics(font);
    const int margin = 4; // Random small number

    QRect textRect = option.rect.adjusted(-margin, 0, -margin, 0);
    textRect.setLeft(textRect.right() - fontMetrics.horizontalAdvance(text) - 2);
    textRect = QStyle::visualRect(option.direction, option.rect, textRect);
    // Constant LeftToRight because we don't need to flip it any further.
    // Vertically align the text in the middle to match QCommonStyle behaviour.
    const auto alignedRect = QStyle::alignedRect(Qt::LeftToRight, option.displayAlignment,
                                                 QSize(textRect.size().width(), fontMetrics.height()), textRect);

    painter->save();

    QPalette palette = option.palette;

#ifdef Q_OS_WINDOWS
    // QWindowsVistaStyle overrides highlight colour.
    if (option.widget->style()->objectName() == QLatin1String("windowsvista")) {
        palette.setColor(QPalette::All, QPalette::HighlightedText,
                         palette.color(QPalette::Active, QPalette::Text));
    }
#endif

    const QPalette::ColorGroup cg = (option.state & QStyle::State_Active)
            ? QPalette::Normal : QPalette::Inactive;

    if (option.state & QStyle::State_Selected) {
        painter->setPen(palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(palette.color(cg, QPalette::Text));
    }

    painter->setFont(font);
    painter->drawText(alignedRect, text);

    painter->restore();
}

void DocsetListItemDelegate::paintProgressBar(QPainter *painter,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    bool ok;
    const int value = index.model()->data(index, ValueRole).toInt(&ok);

    if (!ok) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // Adjust maximum text width
    QStyleOptionViewItem styleOption = option;
    styleOption.rect.setRight(styleOption.rect.right() - ProgressBarWidth);

    // Size progress bar
    QScopedPointer<QProgressBar> renderer(new QProgressBar());
    renderer->resize(ProgressBarWidth, styleOption.rect.height());
    renderer->setRange(0, 100);
    renderer->setValue(value);

    const QString format = index.model()->data(index, FormatRole).toString();
    if (!format.isEmpty()) {
        renderer->setFormat(format);
    }

    painter->save();

    // Paint progress bar
    painter->translate(styleOption.rect.topRight());
    renderer->render(painter);

    painter->restore();

    QStyledItemDelegate::paint(painter, styleOption, index);
}
