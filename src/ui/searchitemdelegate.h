#ifndef SEARCHITEMDELEGATE_H
#define SEARCHITEMDELEGATE_H

#include <QLineEdit>
#include <QStyledItemDelegate>

class SearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SearchItemDelegate(QLineEdit *lineEdit_ = nullptr, QWidget *view = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const;

private:
    QLineEdit *m_lineEdit;
    QWidget *m_view;
};

#endif // SEARCHITEMDELEGATE_H
