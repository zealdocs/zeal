#include "progressitemdelegate.h"

#include <QPainter>
#include <QProgressBar>

ProgressItemDelegate::ProgressItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}

void ProgressItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (!index.model()->data(index, ShowProgressRole).toBool()) {
        QItemDelegate::paint(painter, option, index);
        return;
    }

    bool ok;
    const int value = index.model()->data(index, ValueRole).toInt(&ok);
    const int max = index.model()->data(index, MaximumRole).toInt(&ok);

    if (!ok) {
        QItemDelegate::paint(painter, option, index);
        return;
    }

    // Adjust maximum text width
    QStyleOptionViewItem styleOption = option;
    styleOption.rect.setRight(styleOption.rect.right() - progressBarWidth);

    // Size progress bar
    QScopedPointer<QProgressBar> renderer(new QProgressBar());
    renderer->resize(progressBarWidth, styleOption.rect.height());
    renderer->setMinimum(0);
    renderer->setMaximum(max);
    renderer->setValue(value);

    const QString format = index.model()->data(index, FormatRole).toString();
    if (!format.isEmpty())
        renderer->setFormat(format);

    painter->save();

    // Paint progress bar
    painter->translate(styleOption.rect.topRight());
    renderer->render(painter);

    painter->restore();

    QItemDelegate::paint(painter, styleOption, index);
}
