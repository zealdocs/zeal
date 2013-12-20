#include "progressitemdelegate.h"

#include <QPainter>
#include <QProgressBar>

ProgressItemDelegate::ProgressItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}

void ProgressItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant itemProgress = index.model()->data(index, ProgressRole);
    QVariant maxProgress  = index.model()->data(index, ProgressMaxRole);
    QVariant formatProgress = index.model()->data(index, ProgressFormatRole);
    QVariant visible = index.model()->data(index, ProgressVisibleRole);
    QStyleOptionViewItem tempOption = option;
    if( itemProgress.isValid() && maxProgress.isValid() && (visible.isValid() && visible.toBool()) ){
        tempOption.rect.setRight( tempOption.rect.right() - 150 );

        QProgressBar renderer;
        int progressAmnt = itemProgress.toInt();

        renderer.resize( QSize( 150, tempOption.rect.height() ));
        renderer.setMinimum(0);
        renderer.setMaximum( maxProgress.toInt() );
        renderer.setValue( progressAmnt );
        if( formatProgress.isValid() ){
            renderer.setFormat( formatProgress.toString() );
        }

        painter->save();
        QPoint rect = tempOption.rect.topRight();
        painter->translate( rect );
        renderer.render( painter );
        painter->restore();
    }

    QItemDelegate::paint( painter, dynamic_cast<const QStyleOptionViewItem&>(tempOption), index );

    return;
}
