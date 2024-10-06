// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "container.h"

#include "view.h"

#include <core/application.h>
#include <core/settings.h>
#include <ui/widgets/layouthelper.h>

#include <QSplitter>
#include <QVBoxLayout>

using namespace Zeal;
using namespace Zeal::Sidebar;

Container::Container(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(150);

    // Setup splitter.
    m_splitter = new QSplitter();
    m_splitter->setOrientation(Qt::Vertical);
    connect(m_splitter, &QSplitter::splitterMoved, this, [this]() {
        Core::Application::instance()->settings()->tocSplitterState = m_splitter->saveState();
    });

    // Setup main layout.
    auto layout = WidgetUi::LayoutHelper::createBorderlessLayout<QVBoxLayout>();
    layout->addWidget(m_splitter);
    setLayout(layout);
}

Container::~Container() = default;

void Container::addView(View *view)
{
    if (m_views.contains(view))
        return;

    m_views.append(view);
    m_splitter->addWidget(view);
}
