#ifndef PROGRESSITEMDELEGATE_H
#define PROGRESSITEMDELEGATE_H

#include <QItemDelegate>

class ProgressItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    enum ProgressRoles {
        ProgressRole = Qt::UserRole + 10,
        ProgressMaxRole = Qt::UserRole + 11,
        ProgressFormatRole = Qt::UserRole + 12,
        ProgressVisibleRole = Qt::UserRole + 13
    };

    explicit ProgressItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    static const int progressBarWidth = 150;
};

#endif // PROGRESSITEMDELEGATE_H
