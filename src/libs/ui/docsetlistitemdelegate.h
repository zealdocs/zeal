// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_DOCSETLISTITEMDELEGATE_H
#define ZEAL_WIDGETUI_DOCSETLISTITEMDELEGATE_H

#include <QStyledItemDelegate>

namespace Zeal::WidgetUi {

class DocsetListItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    // Unscoped: these values feed Qt's int-based model/view API — data()/setData()
    // roles extending Qt::UserRole — where an enum class would force casts at every use.
    // NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
    enum ProgressRoles {
        ValueRole = Qt::UserRole + 10,
        FormatRole,
        ShowProgressRole
    };

    explicit DocsetListItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintProgressBar(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_DOCSETLISTITEMDELEGATE_H
