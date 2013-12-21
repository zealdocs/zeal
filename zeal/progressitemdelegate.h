#ifndef PROGRESSITEMDELEGATE_H
#define PROGRESSITEMDELEGATE_H

#include <QItemDelegate>

class ProgressItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit ProgressItemDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;


    enum ProgressRoles {
        ProgressRole = Qt::UserRole + 10,
        ProgressMaxRole = Qt::UserRole + 11,
        ProgressFormatRole = Qt::UserRole + 12,
        ProgressVisibleRole = Qt::UserRole + 13
    };
private:
    static const int progressBarWidth = 150;

signals:

public slots:

};

#endif // PROGRESSITEMDELEGATE_H
