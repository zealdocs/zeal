#ifndef ZEALSEARCHITEMDELEGATE_H
#define ZEALSEARCHITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QLineEdit>

class ZealSearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ZealSearchItemDelegate(QObject *parent = 0, QLineEdit* lineEdit_ = nullptr, QWidget* view_ = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
signals:
    
public slots:
    
private:
    QLineEdit *lineEdit;
    QWidget *view;
};

#endif // ZEALSEARCHITEMDELEGATE_H
