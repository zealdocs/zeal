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

#ifndef ZEAL_WIDGETUI_DOCSETLISTITEMDELEGATE_H
#define ZEAL_WIDGETUI_DOCSETLISTITEMDELEGATE_H

#include <QStyledItemDelegate>

namespace Zeal {
namespace WidgetUi {

class DocsetListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    enum ProgressRoles {
        ValueRole = Qt::UserRole + 10,
        FormatRole,
        ShowProgressRole
    };

    explicit DocsetListItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    void paintProgressBar(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_DOCSETLISTITEMDELEGATE_H
