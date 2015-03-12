#include "docsetlistitemdelegate.h"

#include "registry/listmodel.h"

#include <QPainter>

DocsetListItemDelegate::DocsetListItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}

void DocsetListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QItemDelegate::paint(painter, option, index);

    if (!index.model()->data(index, Zeal::ListModel::UpdateAvailableRole).toBool())
        return;

    const QString text = tr("Update available");

    QFont font(painter->font());
    font.setItalic(true);

    const QFontMetrics fontMetrics(font);
    const int textWidth = fontMetrics.width(text) + 2;

    QRect textRect = option.rect;
    textRect.setLeft(textRect.right() - textWidth);

    painter->save();

    if (option.state & QStyle::State_Selected)
        painter->setPen(QPen(option.palette.highlightedText(), 1));

    painter->setFont(font);
    painter->drawText(textRect, text);

    painter->restore();
}
