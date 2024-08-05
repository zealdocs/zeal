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

#include "searchsidebar.h"

#include "searchitemdelegate.h"
#include "widgets/layouthelper.h"
#include "widgets/searchedit.h"
#include "widgets/toolbarframe.h"

#include <core/application.h>
#include <core/settings.h>
#include <registry/docset.h>
#include <registry/docsetregistry.h>
#include <registry/itemdatarole.h>
#include <registry/listmodel.h>
#include <registry/searchmodel.h>
#include <registry/searchquery.h>

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QListView>
#include <QScrollBar>
#include <QShortcut>
#include <QSplitter>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Zeal;
using namespace Zeal::WidgetUi;

SearchSidebar::SearchSidebar(QWidget *parent)
    : SearchSidebar(nullptr, parent)
{
}

SearchSidebar::SearchSidebar(const SearchSidebar *other, QWidget *parent)
    : Sidebar::View(parent)
{
    // Setup search result view.
    m_treeView = new QTreeView();
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setHeaderHidden(true);
    m_treeView->setUniformRowHeights(true);
#ifdef Q_OS_MACOS
    m_treeView->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    // Save expanded items.
    connect(m_treeView, &QTreeView::expanded, this, [this](const QModelIndex &index) {
        if (m_expandedIndexList.indexOf(index) == -1) {
            m_expandedIndexList.append(index);
        }
    });
    connect(m_treeView, &QTreeView::collapsed, this, [this](const QModelIndex &index) {
        m_expandedIndexList.removeOne(index);
    });

    auto delegate = new SearchItemDelegate(m_treeView);
    delegate->setDecorationRoles({Registry::ItemDataRole::DocsetIconRole, Qt::DecorationRole});
    m_treeView->setItemDelegate(delegate);

    connect(m_treeView, &QTreeView::activated, this, &SearchSidebar::navigateToIndexAndActivate);
    connect(m_treeView, &QTreeView::clicked, this, &SearchSidebar::navigateToIndex);

    // Setup Alt+Up, Alt+Down, etc shortcuts.
    const auto keyList = {Qt::Key_Up, Qt::Key_Down,
                          Qt::Key_PageUp, Qt::Key_PageDown,
                          Qt::Key_Home, Qt::Key_End};
    for (const auto key : keyList) {
        auto shortcut = new QShortcut(key | Qt::AltModifier, this);
        connect(shortcut, &QShortcut::activated, this, [this, key]() {
            QKeyEvent event(QKeyEvent::KeyPress, key, Qt::NoModifier);
            QCoreApplication::sendEvent(m_treeView, &event);
        });
    }

    // Setup page TOC view.
    // TODO: Move to a separate Sidebar View.
    m_pageTocView = new QListView();
    m_pageTocView->setFrameShape(QFrame::NoFrame);
    m_pageTocView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pageTocView->setItemDelegate(new SearchItemDelegate(m_pageTocView));
    m_pageTocView->setUniformItemSizes(true);
    m_pageTocView->setVisible(false);
#ifdef Q_OS_MACOS
    m_pageTocView->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif

    m_pageTocModel = new Registry::SearchModel(this);
    connect(m_pageTocModel, &Registry::SearchModel::updated, this, [this]() {
        if (m_pageTocModel->isEmpty()) {
            m_pageTocView->hide();
        } else {
            m_pageTocView->show();
            m_splitter->restoreState(Core::Application::instance()->settings()->tocSplitterState);
        }
    });
    m_pageTocView->setModel(m_pageTocModel);

    connect(m_pageTocView, &QListView::activated, this, &SearchSidebar::navigateToIndexAndActivate);
    connect(m_pageTocView, &QListView::clicked, this, &SearchSidebar::navigateToIndex);

    // Setup search input box.
    m_searchEdit = new SearchEdit();
    m_searchEdit->installEventFilter(this);
    setupSearchBoxCompletions();

    // Connect delegate first to make cloning work.
    connect(m_searchEdit, &QLineEdit::textChanged, this, [delegate](const QString &text) {
        delegate->setHighlight(Registry::SearchQuery::fromString(text).query());
    });

    // Clone state if `other` is provided.
    if (other) {
        if (other->m_searchEdit->text().isEmpty()) {
            m_searchModel = new Registry::SearchModel(this);
            setTreeViewModel(Core::Application::instance()->docsetRegistry()->model(), true);

            for (const QModelIndex &index : other->m_expandedIndexList) {
                m_treeView->expand(index);
            }
        } else {
            m_searchEdit->setText(other->m_searchEdit->text());

            m_searchModel = other->m_searchModel->clone(this);
            setTreeViewModel(m_searchModel, false);
        }

        // Clone current selection. Signals must be disabled to avoid crash in the event handler.
        m_treeView->selectionModel()->blockSignals(true);

        for (const QModelIndex &index : other->m_treeView->selectionModel()->selectedIndexes()) {
            m_treeView->selectionModel()->select(index, QItemSelectionModel::Select);
        }

        m_treeView->selectionModel()->blockSignals(false);

        // Cannot update position until layout geometry is calculated, so set it in showEvent().
        m_pendingVerticalPosition = other->m_treeView->verticalScrollBar()->value();
    } else {
        m_searchModel = new Registry::SearchModel(this);
        setTreeViewModel(Core::Application::instance()->docsetRegistry()->model(), true);
    }

    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        QItemSelectionModel *oldSelectionModel = m_treeView->selectionModel();

        if (text.isEmpty()) {
            setTreeViewModel(Core::Application::instance()->docsetRegistry()->model(), true);
        } else {
            setTreeViewModel(m_searchModel, false);
        }

        QItemSelectionModel *newSelectionModel = m_treeView->selectionModel();
        if (newSelectionModel != oldSelectionModel) {
            // TODO: Remove once QTBUG-49966 is addressed.
            if (oldSelectionModel) {
                oldSelectionModel->deleteLater();
            }

            // Connect to the new selection model.
            connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
                    this, &SearchSidebar::navigateToSelectionWithDelay);
        }

        m_treeView->reset();

        Core::Application::instance()->docsetRegistry()->search(text);
    });

    auto toolBarLayout = new QVBoxLayout();
    toolBarLayout->setContentsMargins(4, 0, 4, 0);
    toolBarLayout->setSpacing(4);

    toolBarLayout->addWidget(m_searchEdit);

    auto toolBarFrame = new ToolBarFrame();
    toolBarFrame->setLayout(toolBarLayout);

    // Setup splitter.
    m_splitter = new QSplitter();
    m_splitter->setOrientation(Qt::Vertical);
    m_splitter->addWidget(m_treeView);
    m_splitter->addWidget(m_pageTocView);
    connect(m_splitter, &QSplitter::splitterMoved, this, [this]() {
        Core::Application::instance()->settings()->tocSplitterState = m_splitter->saveState();
    });

    // Setup main layout.
    auto layout = LayoutHelper::createBorderlessLayout<QVBoxLayout>();
    layout->addWidget(toolBarFrame);
    layout->addWidget(m_splitter);
    setLayout(layout);

    // Setup delayed navigation to a page until user makes a pause in typing a search query.
    m_delayedNavigationTimer = new QTimer(this);
    m_delayedNavigationTimer->setInterval(400);
    m_delayedNavigationTimer->setSingleShot(true);
    connect(m_delayedNavigationTimer, &QTimer::timeout, this, [this]() {
        const QModelIndex index = m_delayedNavigationTimer->property("index").toModelIndex();
        if (!index.isValid()) {
            return;
        }

        navigateToIndex(index);
    });

    // Setup Docset Registry.
    auto registry = Core::Application::instance()->docsetRegistry();
    using Registry::DocsetRegistry;
    connect(registry, &DocsetRegistry::searchCompleted,
            this, [this](const QList<Registry::SearchResult> &results) {
        if (!isVisible()) {
            return;
        }

        m_delayedNavigationTimer->stop();

        m_searchModel->setResults(results);

        const QModelIndex index = m_searchModel->index(0, 0, QModelIndex());
        m_treeView->setCurrentIndex(index);
        m_delayedNavigationTimer->setProperty("index", index);
        m_delayedNavigationTimer->start();
    });

    connect(registry, &DocsetRegistry::docsetAboutToBeUnloaded, this, [this](const QString &name) {
        if (isVisible()) {
            // Disable updates because removeSearchResultWithName can
            // call {begin,end}RemoveRows multiple times, and cause
            // degradation of UI responsiveness.
            m_treeView->setUpdatesEnabled(false);
            m_searchModel->removeSearchResultWithName(name);
            m_treeView->setUpdatesEnabled(true);
        } else {
            m_searchModel->removeSearchResultWithName(name);
        }

        setupSearchBoxCompletions();
    });

    connect(registry, &DocsetRegistry::docsetLoaded, this, [this](const QString &) {
        setupSearchBoxCompletions();
    });
}

void SearchSidebar::setTreeViewModel(QAbstractItemModel *model, bool isRootDecorated)
{
    QItemSelectionModel *oldSelectionModel = m_treeView->selectionModel();

    m_treeView->setModel(model);
    m_treeView->setRootIsDecorated(isRootDecorated);

    // Hide all but the first column.
    for (int i = 1; i < model->columnCount(); ++i) {
        m_treeView->setColumnHidden(i, true);
    }

    QItemSelectionModel *newSelectionModel = m_treeView->selectionModel();
    if (newSelectionModel != oldSelectionModel) {
        // TODO: Remove once QTBUG-49966 is addressed.
        if (oldSelectionModel) {
            oldSelectionModel->deleteLater();
        }

        // Connect to the new selection model.
        connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &SearchSidebar::navigateToSelectionWithDelay);
    }
}

SearchSidebar *SearchSidebar::clone(QWidget *parent) const
{
    return new SearchSidebar(this, parent);
}

Registry::SearchModel *SearchSidebar::pageTocModel() const
{
    return m_pageTocModel;
}

void SearchSidebar::focusSearchEdit(bool clear)
{
    if (!isVisible()) {
        m_pendingSearchEditFocus = true;
        return;
    }

    m_searchEdit->setFocus();

    if (clear) {
        m_searchEdit->clearQuery();
    }
}

void SearchSidebar::search(const Registry::SearchQuery &query)
{
    // TODO: Pass Registry::SearchQuery, converting to QString is dumb at this point.
    m_searchEdit->setText(query.toString());
}

void SearchSidebar::navigateToIndex(const QModelIndex &index)
{
    // When triggered by click, cancel delayed navigation request caused by the selection change.
    if (m_delayedNavigationTimer->isActive()
            && m_delayedNavigationTimer->property("index").toModelIndex() == index) {
        m_delayedNavigationTimer->stop();
    }

    const QVariant url = index.data(Registry::ItemDataRole::UrlRole);
    if (url.isNull()) {
        return;
    }

    emit navigationRequested(url.toUrl());
}

void SearchSidebar::navigateToIndexAndActivate(const QModelIndex &index)
{
    const QVariant url = index.data(Registry::ItemDataRole::UrlRole);
    if (url.isNull()) {
        return;
    }

    emit navigationRequested(url.toUrl());
    emit activated();
}

void SearchSidebar::navigateToSelectionWithDelay(const QItemSelection &selection)
{
    if (selection.isEmpty()) {
        return;
    }

    m_delayedNavigationTimer->setProperty("index", selection.indexes().first());
    m_delayedNavigationTimer->start();
}

void SearchSidebar::setupSearchBoxCompletions()
{
    QStringList completions;
    const auto docsets = Core::Application::instance()->docsetRegistry()->docsets();
    for (const Registry::Docset *docset : docsets) {
        const QStringList keywords = docset->keywords();
        if (keywords.isEmpty())
            continue;

        completions << keywords.constFirst() + QLatin1Char(':');
    }

    if (completions.isEmpty())
        return;

    m_searchEdit->setCompletions(completions);
}

bool SearchSidebar::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_searchEdit && event->type() == QEvent::KeyPress) {
        auto e = static_cast<QKeyEvent *>(event);
        switch (e->key()) {
        case Qt::Key_Return:
            emit activated();
            break;
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Left:
        case Qt::Key_Right:
            if (!m_searchEdit->text().isEmpty()) {
                break;
            }
            [[fallthrough]];
        case Qt::Key_Down:
        case Qt::Key_Up:
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            QCoreApplication::sendEvent(m_treeView, event);
            break;

        }
    }

    return Sidebar::View::eventFilter(object, event);
}

void SearchSidebar::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)

    if (m_pendingVerticalPosition > 0) {
        m_treeView->verticalScrollBar()->setValue(m_pendingVerticalPosition);
        m_pendingVerticalPosition = 0;
    }

    if (m_pendingSearchEditFocus) {
        m_searchEdit->setFocus();
        m_pendingSearchEditFocus = false;
    }
}
