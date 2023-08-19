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
