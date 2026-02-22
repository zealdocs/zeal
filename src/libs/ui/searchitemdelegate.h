// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

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

    int textHighlightRole() const;
    void setTextHighlightRole(int role);

    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option,
                   const QModelIndex &index) override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QList<int> m_decorationRoles = {Qt::DecorationRole};
    int m_textHighlightRole = -1;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_SEARCHITEMDELEGATE_H
