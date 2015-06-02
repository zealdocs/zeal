#ifndef SEARCHITEMDELEGATE_H
#define SEARCHITEMDELEGATE_H

#include <QStyledItemDelegate>

class SearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SearchItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

public slots:
    void setHighlight(const QString &text);

private:
    QString m_highlight;
};

#endif // SEARCHITEMDELEGATE_H
