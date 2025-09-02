// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

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
    Q_DISABLE_COPY_MOVE(SearchSidebar)
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
