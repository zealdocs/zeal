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
               const QModelIndex &index) const override;

private:
    QLineEdit *m_lineEdit = nullptr;
    QWidget *m_view = nullptr;
};

#endif // SEARCHITEMDELEGATE_H
