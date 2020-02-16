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

#include "proxyview.h"

#include "viewprovider.h"

#include <ui/widgets/layouthelper.h>

#include <QVBoxLayout>

#include <utility>

using namespace Zeal;
using namespace Zeal::Sidebar;

ProxyView::ProxyView(ViewProvider *provider, QString id, QWidget *parent)
    : View(parent)
    , m_viewProvider(provider)
    , m_viewId(std::move(id))
{
    setLayout(WidgetUi::LayoutHelper::createBorderlessLayout<QVBoxLayout>());

    connect(m_viewProvider, &ViewProvider::viewChanged, this, [this]() {
        auto view = m_viewProvider->view(m_viewId);
        if (view == nullptr) {
            qWarning("ViewProvider returned invalid view!");
            return;
        }

        if (m_view == view)
            return;

        clearCurrentView();
        layout()->addWidget(view);
        view->show();
        m_view = view;
    });
}

ProxyView::~ProxyView()
{
    clearCurrentView();
}

void ProxyView::clearCurrentView()
{
    // Unparent the view, because we don't own it.
    QLayout *l = layout();
    if (l->isEmpty())
        return;

    m_view->hide();
    l->removeWidget(m_view);
    m_view->setParent(nullptr);
}
