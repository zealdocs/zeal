#ifndef DOCSETLISTITEMDELEGATE_H
#define DOCSETLISTITEMDELEGATE_H

#include <QItemDelegate>

class DocsetListItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit DocsetListItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

#endif // DOCSETLISTITEMDELEGATE_H
