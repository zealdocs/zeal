#include "progressitemdelegate.h"

#include <QPainter>
#include <QProgressBar>

ProgressItemDelegate::ProgressItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}

void ProgressItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem tempOption = option;
    QVariant itemProgress = index.model()->data(index, ProgressRole);
    QVariant maxProgress  = index.model()->data(index, ProgressMaxRole);
    QVariant visible = index.model()->data(index, ProgressVisibleRole);

    if( itemProgress.isValid() && maxProgress.isValid() && (visible.isValid() && visible.toBool()) ){
        QProgressBar renderer;
        QVariant formatProgress = index.model()->data(index, ProgressFormatRole);
        int progressAmnt = itemProgress.toInt();

        // Adjust maximum text width
        tempOption.rect.setRight( tempOption.rect.right() - progressBarWidth);

        // Size progress bar
        renderer.resize( QSize( progressBarWidth, tempOption.rect.height() ));
        renderer.setMinimum(0);
        renderer.setMaximum( maxProgress.toInt() );
        renderer.setValue( progressAmnt );
        if( formatProgress.isValid() ){
            renderer.setFormat( formatProgress.toString() );
        }

        // Paint progress bar
        painter->save();
        QPoint rect = tempOption.rect.topRight();
        painter->translate( rect );
        renderer.render( painter );
        painter->restore();
    }

    // Paint text
    QItemDelegate::paint( painter, dynamic_cast<const QStyleOptionViewItem&>(tempOption), index );

    return;
}
