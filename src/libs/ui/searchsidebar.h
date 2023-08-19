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

#ifndef ZEAL_WIDGETUI_SEARCHSIDEBAR_H
#define ZEAL_WIDGETUI_SEARCHSIDEBAR_H

#include <sidebar/view.h>

#include <QModelIndexList>
#include <QWidget>

class QItemSelection;
class QSplitter;
class QListView;
class QTimer;
class QTreeView;

namespace Zeal {

namespace Registry {
class SearchModel;
class SearchQuery;
} // namespace Registry

namespace WidgetUi {

class SearchEdit;

class SearchSidebar final : public Sidebar::View
{
    Q_OBJECT
    Q_DISABLE_COPY(SearchSidebar)
public:
    explicit SearchSidebar(QWidget *parent = nullptr);
    SearchSidebar *clone(QWidget *parent = nullptr) const;

    Registry::SearchModel *pageTocModel() const;

signals:
    void activated();
    void navigationRequested(const QUrl &url);

public slots:
    void focusSearchEdit(bool clear = false);
    void search(const Registry::SearchQuery &query);

private slots:
    void navigateToIndex(const QModelIndex &index);
    void navigateToIndexAndActivate(const QModelIndex &index);
    void navigateToSelectionWithDelay(const QItemSelection &selection);
    void setupSearchBoxCompletions();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    explicit SearchSidebar(const SearchSidebar *other, QWidget *parent = nullptr);
    void setTreeViewModel(QAbstractItemModel *model, bool isRootDecorated);

    SearchEdit *m_searchEdit = nullptr;
    bool m_pendingSearchEditFocus = false;

    // Index and search results tree view state.
    QTreeView *m_treeView = nullptr;
    QModelIndexList m_expandedIndexList;
    int m_pendingVerticalPosition = 0;
    Registry::SearchModel *m_searchModel = nullptr;

    // TOC list view state.
    QListView *m_pageTocView = nullptr;
    Registry::SearchModel *m_pageTocModel = nullptr;

    QSplitter *m_splitter = nullptr;
    QTimer *m_delayedNavigationTimer = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_SEARCHSIDEBAR_H
