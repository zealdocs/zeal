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

#ifndef ZEAL_SIDEBAR_PROXYVIEW_H
#define ZEAL_SIDEBAR_PROXYVIEW_H

#include "view.h"

namespace Zeal {
namespace Sidebar {

class ViewProvider;

class ProxyView final : public View
{
    Q_OBJECT
    Q_DISABLE_COPY(ProxyView)
public:
    explicit ProxyView(ViewProvider *provider, QString id = QString(), QWidget *parent = nullptr);
    ~ProxyView() override;

private:
    void clearCurrentView();

    ViewProvider *m_viewProvider = nullptr;
    QString m_viewId;

    View *m_view = nullptr;
};

} // namespace Sidebar
} // namespace Zeal

#endif // ZEAL_SIDEBAR_PROXYVIEW_H
