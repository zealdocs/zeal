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
