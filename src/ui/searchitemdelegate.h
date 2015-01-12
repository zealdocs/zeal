#ifndef SEARCHITEMDELEGATE_H
#define SEARCHITEMDELEGATE_H

#include <QLineEdit>
#include <QStyledItemDelegate>

class ZealSearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ZealSearchItemDelegate(QObject *parent = 0, QLineEdit *lineEdit_ = nullptr,
                                    QWidget *view_ = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

private:
    QLineEdit *lineEdit;
    QWidget *view;
};

#endif // SEARCHITEMDELEGATE_H
