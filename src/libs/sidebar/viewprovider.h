/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#ifndef ZEAL_SIDEBAR_VIEWPROVIDER_H
#define ZEAL_SIDEBAR_VIEWPROVIDER_H

#include <QObject>

namespace Zeal {
namespace Sidebar {

class View;

class ViewProvider : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ViewProvider)
public:
    explicit ViewProvider(QObject *parent = nullptr);

    virtual View *view(const QString &id = QString()) const = 0;

signals:
    void viewChanged();
};

} // namespace Sidebar
} // namespace Zeal

#endif // ZEAL_SIDEBAR_VIEWPROVIDER_H
