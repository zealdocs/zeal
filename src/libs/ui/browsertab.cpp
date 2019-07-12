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

#include "browsertab.h"

#include "searchsidebar.h"
#include "widgets/layouthelper.h"
#include "widgets/toolbarframe.h"

#include <browser/webcontrol.h>
#include <core/application.h>
#include <core/settings.h>
#include <registry/docset.h>
#include <registry/docsetregistry.h>
#include <registry/searchquery.h>

#include <QApplication>
#include <QLabel>
#include <QMenu>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineHistory>

using namespace Zeal;
using namespace Zeal::WidgetUi;

namespace {
constexpr char WelcomePageUrl[] = "qrc:///browser/welcome.html";
} // namespace

BrowserTab::BrowserTab(QWidget *parent)
    : QWidget(parent)
{
    // Setup WebControl.
    m_webControl = new Browser::WebControl(this);
    connect(m_webControl, &Browser::WebControl::titleChanged, this, &BrowserTab::titleChanged);
    connect(m_webControl, &Browser::WebControl::urlChanged, this, [this](const QUrl &url) {
        // TODO: Check if changed.
        emit iconChanged(docsetIcon(url));

        Registry::Docset *docset = Core::Application::instance()->docsetRegistry()->docsetForUrl(url);
        if (docset) {
            searchSidebar()->pageTocModel()->setResults(docset->relatedLinks(url));
            m_webControl->setJavaScriptEnabled(docset->isJavaScriptEnabled());
        } else {
            searchSidebar()->pageTocModel()->setResults();
            // Always enable JS outside of docsets.
            m_webControl->setJavaScriptEnabled(true);
        }

        m_backButton->setEnabled(m_webControl->canGoBack());
        m_forwardButton->setEnabled(m_webControl->canGoForward());
    });

    // Setup navigation toolbar.
    m_backButton = new QToolButton();
    m_backButton->setAutoRaise(true);
    m_backButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowBack));
    m_backButton->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image: none; }"));
    m_backButton->setText(QStringLiteral("←"));
    m_backButton->setToolTip(tr("Go back one page"));

    auto backMenu = new QMenu(m_backButton);
    connect(backMenu, &QMenu::aboutToShow, this, [this, backMenu]() {
        backMenu->clear();
        QWebEngineHistory *history = m_webControl->history();
        QList<QWebEngineHistoryItem> items = history->backItems(10);
        for (auto it = items.crbegin(); it != items.crend(); ++it) {
            const QIcon icon = docsetIcon(it->url());
            const QWebEngineHistoryItem item = *it;
            backMenu->addAction(icon, it->title(), this, [=](bool) { history->goToItem(item); });
        }
    });
    m_backButton->setMenu(backMenu);

    connect(m_backButton, &QToolButton::clicked, m_webControl, &Browser::WebControl::back);

    m_forwardButton = new QToolButton();
    m_forwardButton->setAutoRaise(true);
    m_forwardButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowForward));
    m_forwardButton->setStyleSheet(QStringLiteral("QToolButton::menu-indicator { image: none; }"));
    m_forwardButton->setText(QStringLiteral("→"));
    m_forwardButton->setToolTip(tr("Go forward one page"));

    auto forwardMenu = new QMenu(m_forwardButton);
    connect(forwardMenu, &QMenu::aboutToShow, this, [this, forwardMenu]() {
        forwardMenu->clear();
        QWebEngineHistory *history = m_webControl->history();
        const auto forwardItems = history->forwardItems(10);
        for (const QWebEngineHistoryItem &item : forwardItems) {
            const QIcon icon = docsetIcon(item.url());
            forwardMenu->addAction(icon, item.title(), this, [=](bool) { history->goToItem(item); });
        }
    });
    m_forwardButton->setMenu(forwardMenu);

    connect(m_forwardButton, &QToolButton::clicked, m_webControl, &Browser::WebControl::forward);

    auto label = new QLabel();
    label->setAlignment(Qt::AlignCenter);
    connect(m_webControl, &Browser::WebControl::titleChanged, this, [label](const QString &title) {
        if (title.isEmpty())
            return;

        label->setText(title);
    });

    auto toolBarLayout = new QHBoxLayout();
    toolBarLayout->setContentsMargins(4, 0, 4, 0);
    toolBarLayout->setSpacing(4);

    toolBarLayout->addWidget(m_backButton);
    toolBarLayout->addWidget(m_forwardButton);
    toolBarLayout->addWidget(label, 1);

    auto toolBarFrame = new ToolBarFrame();
    toolBarFrame->setLayout(toolBarLayout);

    // Setup main layout.
    auto layout = LayoutHelper::createBorderlessLayout<QVBoxLayout>();
    layout->addWidget(toolBarFrame);
    layout->addWidget(m_webControl);
    setLayout(layout);

    auto registry = Core::Application::instance()->docsetRegistry();
    using Registry::DocsetRegistry;
    connect(registry, &DocsetRegistry::docsetAboutToBeUnloaded,
            this, [this, registry](const QString &name) {
        Registry::Docset *docset = registry->docsetForUrl(m_webControl->url());
        if (docset == nullptr ||  docset->name() != name) {
            return;
        }


        // TODO: Add custom 'Page has been removed' page.
        navigateToStartPage();

        // TODO: Cleanup history.
    });
}

BrowserTab *BrowserTab::clone(QWidget *parent) const
{
    auto tab = new BrowserTab(parent);

    if (m_searchSidebar) {
        tab->m_searchSidebar = m_searchSidebar->clone();
        connect(tab->m_searchSidebar, &SearchSidebar::activated,
                tab->m_webControl, &Browser::WebControl::focus);
        connect(tab->m_searchSidebar, &SearchSidebar::navigationRequested,
                tab->m_webControl, &Browser::WebControl::load);
    }

    tab->m_webControl->restoreHistory(m_webControl->saveHistory());
    tab->m_webControl->setZoomLevel(m_webControl->zoomLevel());

    return tab;
}

BrowserTab::~BrowserTab()
{
    if (m_searchSidebar) {
        // The sidebar is not in this widget's hierarchy, so direct delete is not safe.
        m_searchSidebar->deleteLater();
    }
}

Browser::WebControl *BrowserTab::webControl() const
{
    return m_webControl;
}

SearchSidebar *BrowserTab::searchSidebar()
{
    if (m_searchSidebar == nullptr) {
        // Create SearchSidebar managed by this tab.
        m_searchSidebar = new SearchSidebar();
        connect(m_searchSidebar, &SearchSidebar::activated,
                m_webControl, &Browser::WebControl::focus);
        connect(m_searchSidebar, &SearchSidebar::navigationRequested,
                m_webControl, &Browser::WebControl::load);
    }

    return m_searchSidebar;
}

void BrowserTab::navigateToStartPage()
{
    m_webControl->load(QUrl(WelcomePageUrl));
}

void BrowserTab::search(const Registry::SearchQuery &query)
{
    if (query.isEmpty())
        return;

    m_searchSidebar->search(query);
}

QIcon BrowserTab::docsetIcon(const QUrl &url) const
{
    Registry::Docset *docset = Core::Application::instance()->docsetRegistry()->docsetForUrl(url);
    return docset ? docset->icon() : QIcon(QStringLiteral(":/icons/logo/icon.png"));
}
