/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#ifndef ZEAL_WIDGETUI_SEARCHITEMDELEGATE_H
#define ZEAL_WIDGETUI_SEARCHITEMDELEGATE_H

#include <QStyledItemDelegate>

namespace Zeal {
namespace WidgetUi {

class SearchItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SearchItemDelegate(QObject *parent = nullptr);

    QList<int> decorationRoles() const;
    void setDecorationRoles(const QList<int> &roles);

    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option,
                   const QModelIndex &index) override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

public slots:
    void setHighlight(const QString &text);

private:
    QList<int> m_decorationRoles = {Qt::DecorationRole};
    QString m_highlight;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_SEARCHITEMDELEGATE_H
