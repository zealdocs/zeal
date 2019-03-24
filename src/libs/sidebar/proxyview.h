/****************************************************************************
**
** Copyright (C) 2019 Oleg Shparber
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef ZEAL_SIDEBAR_PROXYVIEW_H
#define ZEAL_SIDEBAR_PROXYVIEW_H

#include "view.h"

namespace Zeal {
namespace Sidebar {

class ViewProvider;

class ProxyView final : public View
{
    Q_OBJECT
public:
    explicit ProxyView(ViewProvider *provider, const QString &id = QString(), QWidget *parent = nullptr);
    ~ProxyView() override;

private:
    Q_DISABLE_COPY(ProxyView)

    void clearCurrentView();

    ViewProvider *m_viewProvider = nullptr;
    QString m_viewId;

    View *m_view = nullptr;
};

} // namespace Sidebar
} // namespace Zeal

#endif // ZEAL_SIDEBAR_PROXYVIEW_H
