// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_BROWSERTAB_H
#define ZEAL_WIDGETUI_BROWSERTAB_H

#include <registry/searchmodel.h>

#include <QModelIndexList>
#include <QWidget>

class QToolButton;

namespace Zeal {

namespace Browser {
class WebControl;
} // namespace Browser

namespace Registry {
class SearchQuery;
} //namespace Registry

namespace WidgetUi {

class SearchSidebar;

class BrowserTab : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(BrowserTab)
public:
    explicit BrowserTab(QWidget *parent = nullptr);
    BrowserTab *clone(QWidget *parent = nullptr) const;
    ~BrowserTab() override;

    Browser::WebControl *webControl() const;
    SearchSidebar *searchSidebar(); // TODO: const

public slots:
    void navigateToStartPage();
    void search(const Registry::SearchQuery &query);

signals:
    void iconChanged(const QIcon &icon);
    void titleChanged(const QString &title);

private:
    QIcon docsetIcon(const QUrl &url) const;

    // Widgets.
    SearchSidebar *m_searchSidebar = nullptr;
    Browser::WebControl *m_webControl = nullptr;
    QToolButton *m_backButton = nullptr;
    QToolButton *m_forwardButton = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_BROWSERTAB_H
