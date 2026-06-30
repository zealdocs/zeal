// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webcontrol.h"

#include "searchtoolbar.h"
#include "webview.h"

#include <core/networkaccessmanager.h>

#include <QDataStream>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QMetaEnum>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineSettings>

namespace Zeal::Browser {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.browser.webcontrol")

// Bound recovery attempts so a deterministically crashing page does not thrash forever.
constexpr int MaxRenderProcessReloadAttempts = 3;
} // namespace

WebControl::WebControl(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_webView = new WebView();
    setFocusProxy(m_webView);

    connect(m_webView->page(), &QWebEnginePage::linkHovered, this, [this](const QString &link) {
        if (Core::NetworkAccessManager::isLocalUrl(QUrl(link))) {
            return;
        }

        m_webView->setToolTip(link);
    });
    connect(m_webView, &QWebEngineView::titleChanged, this, &WebControl::titleChanged);
    connect(m_webView, &QWebEngineView::urlChanged, this, [this](const QUrl &url) {
        updateWebBridge(url);
        emit urlChanged(url);
    });
    connect(m_webView, &WebView::zoomLevelChanged, this, &WebControl::zoomLevelChanged);

    // On a failed load that set no title, relabel the tab to signal the failure.
    connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            m_renderProcessReloadAttempts = 0;
            return;
        }

        qCWarning(log, "Failed to load '%s'.", qPrintable(m_webView->url().toString()));
        if (m_webView->title().isEmpty()) {
            emit titleChanged(tr("Couldn't load page"));
        }
    });

    // A crashed render process leaves a blank page and never updates the title. Reload a
    // bounded number of times to recover, then relabel the tab if it keeps dying.
    connect(m_webView,
            &QWebEngineView::renderProcessTerminated,
            this,
            [this](QWebEnginePage::RenderProcessTerminationStatus status, int exitCode) {
        if (status == QWebEnginePage::NormalTerminationStatus) {
            return;
        }

        const char *statusName = QMetaEnum::fromType<QWebEnginePage::RenderProcessTerminationStatus>().valueToKey(
            status);
        qCWarning(log,
                  "Render process for '%s' terminated (%s, exit code %d).",
                  qPrintable(m_webView->url().toString()),
                  statusName,
                  exitCode);

        if (m_renderProcessReloadAttempts < MaxRenderProcessReloadAttempts) {
            ++m_renderProcessReloadAttempts;
            // Reloading directly from the termination handler is not supported; defer it.
            // Skip if the user has navigated away in the meantime.
            const QUrl crashedUrl = m_webView->url();
            QTimer::singleShot(0, m_webView, [this, crashedUrl]() {
                if (m_webView->url() == crashedUrl) {
                    m_webView->reload();
                }
            });
            return;
        }

        qCWarning(log,
                  "Giving up reloading '%s' after %d attempts.",
                  qPrintable(m_webView->url().toString()),
                  MaxRenderProcessReloadAttempts);
        emit titleChanged(tr("Couldn't load page"));
    });

    layout->addWidget(m_webView);

    setLayout(layout);
}

void WebControl::focus()
{
    m_webView->setFocus();
}

int WebControl::zoomLevel() const
{
    return m_webView->zoomLevel();
}

void WebControl::setZoomLevel(int level)
{
    m_webView->setZoomLevel(level);
}

int WebControl::zoomLevelPercentage() const
{
    return WebView::availableZoomLevels().value(m_webView->zoomLevel(), 100);
}

void WebControl::zoomIn()
{
    m_webView->zoomIn();
}

void WebControl::zoomOut()
{
    m_webView->zoomOut();
}

void WebControl::resetZoom()
{
    m_webView->resetZoom();
}

void WebControl::setJavaScriptEnabled(bool enabled)
{
    m_webView->page()->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, enabled);
}

void WebControl::setWebBridgeObject(const QString &name, QObject *object)
{
    m_webBridgeName = name;
    m_webBridgeObject = object;

    // Drop any existing channel so updateWebBridge() re-registers with the new object; it
    // returns early when a channel is already present on an open qrc page.
    if (QWebChannel *oldChannel = m_webView->page()->webChannel()) {
        m_webView->page()->setWebChannel(nullptr);
        oldChannel->deleteLater();
    }

    updateWebBridge(m_webView->url());
}

void WebControl::updateWebBridge(const QUrl &url)
{
    QWebEnginePage *page = m_webView->page();
    if (url.scheme() != QLatin1String("qrc") || m_webBridgeName.isEmpty() || m_webBridgeObject == nullptr) {
        if (QWebChannel *oldChannel = page->webChannel()) {
            page->setWebChannel(nullptr);
            oldChannel->deleteLater();
        }
        return;
    }

    if (page->webChannel() != nullptr) {
        return;
    }

    auto *channel = new QWebChannel(page);
    channel->registerObject(m_webBridgeName, m_webBridgeObject);
    page->setWebChannel(channel);
}

void WebControl::load(const QUrl &url)
{
    m_renderProcessReloadAttempts = 0;
    updateWebBridge(url);
    m_webView->load(url);
}

void WebControl::activateSearchBar()
{
    if (m_searchToolBar == nullptr) {
        m_searchToolBar = new SearchToolBar(m_webView);
        layout()->addWidget(m_searchToolBar);
    }

    if (m_webView->hasSelection()) {
        const QString selectedText = m_webView->selectedText().simplified();
        if (!selectedText.isEmpty()) {
            m_searchToolBar->setText(selectedText);
        }
    }

    m_searchToolBar->activate();
}

void WebControl::back()
{
    m_webView->back();
}

void WebControl::forward()
{
    m_webView->forward();
}

bool WebControl::canGoBack() const
{
    return m_webView->history()->canGoBack();
}

bool WebControl::canGoForward() const
{
    return m_webView->history()->canGoForward();
}

QString WebControl::title() const
{
    return m_webView->title();
}

QUrl WebControl::url() const
{
    return m_webView->url();
}

QWebEngineHistory *WebControl::history() const
{
    return m_webView->history();
}

void WebControl::restoreHistory(const QByteArray &array)
{
    QDataStream stream(array);
    stream >> *m_webView->history();
}

QByteArray WebControl::saveHistory() const
{
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream << *m_webView->history();
    return array;
}

void WebControl::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Slash) {
        activateSearchBar();
    } else {
        event->ignore();
    }
}

} // namespace Zeal::Browser
