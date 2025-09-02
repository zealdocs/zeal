// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "proxyview.h"

#include "viewprovider.h"

#include <ui/widgets/layouthelper.h>

#include <QLoggingCategory>
#include <QVBoxLayout>

#include <utility>

using namespace Zeal;
using namespace Zeal::Sidebar;

static Q_LOGGING_CATEGORY(log, "zeal.sidebar.proxyview")

ProxyView::ProxyView(ViewProvider *provider, QString id, QWidget *parent)
    : View(parent)
    , m_viewProvider(provider)
    , m_viewId(std::move(id))
{
    setLayout(WidgetUi::LayoutHelper::createBorderlessLayout<QVBoxLayout>());

    connect(m_viewProvider, &ViewProvider::viewChanged, this, [this]() {
        auto view = m_viewProvider->view(m_viewId);
        if (view == nullptr) {
            qCWarning(log, "ViewProvider returned invalid view for id '%s'.", qPrintable(m_viewId));
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
