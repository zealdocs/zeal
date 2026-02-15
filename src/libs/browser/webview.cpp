// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webview.h"

#include "webcontrol.h"
#include "webpage.h"

#include <core/application.h>
#include <core/settings.h>
#include <ui/browsertab.h>
#include <ui/mainwindow.h>

#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVector>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWheelEvent>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QWebEngineContextMenuData>
#else
#include <QWebEngineContextMenuRequest>
#endif

#include <algorithm>

using namespace Zeal::Browser;

WebView::WebView(QWidget *parent)
    : QWebEngineView(parent)
{
    setPage(new WebPage(this));
    setZoomLevel(defaultZoomLevel());

    // Enable plugins for PDF support.
    settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    settings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);

    QApplication::instance()->installEventFilter(this);
}

int WebView::zoomLevel() const
{
    return m_zoomLevel;
}

void WebView::setZoomLevel(int level)
{
    if (level == m_zoomLevel) {
        return;
    }

    level = qMax(0, level);
    level = qMin(level, availableZoomLevels().size() - 1);

    m_zoomLevel = level;

    // Scale the webview relative to the DPI of the screen.
    // Only scale up for HiDPI displays (>96 DPI), never scale down.
    const double dpiZoomFactor = std::max(1.0, logicalDpiY() / 96.0);

    setZoomFactor(availableZoomLevels().at(level) / 100.0 * dpiZoomFactor);
    emit zoomLevelChanged();
}

const QVector<int> &WebView::availableZoomLevels()
{
    static const QVector<int> zoomLevels = {30, 40, 50, 67, 80, 90, 100,
                                            110, 120, 133, 150, 170, 200,
                                            220, 233, 250, 270, 285, 300};
    return zoomLevels;
}

int WebView::defaultZoomLevel()
{
    static const int level = availableZoomLevels().indexOf(100);
    return level;
}

void WebView::zoomIn()
{
    setZoomLevel(m_zoomLevel + 1);
}

void WebView::zoomOut()
{
    setZoomLevel(m_zoomLevel - 1);
}

void WebView::resetZoom()
{
    setZoomLevel(defaultZoomLevel());
}

QWebEngineView *WebView::createWindow(QWebEnginePage::WebWindowType type)
{
    Q_UNUSED(type)
    return Core::Application::instance()->mainWindow()->createTab()->webControl()->m_webView;
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QWebEngineContextMenuData& contextData = page()->contextMenuData();

    if (!contextData.isValid()) {
        QWebEngineView::contextMenuEvent(event);
        return;
    }
#else
    QWebEngineContextMenuRequest *contextMenuRequest = lastContextMenuRequest();
    if (contextMenuRequest == nullptr) {
        QWebEngineView::contextMenuEvent(event);
        return;
    }
#endif

    event->accept();

    if (m_contextMenu) {
        m_contextMenu->deleteLater();
    }

    m_contextMenu = new QMenu(this);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QUrl linkUrl = contextData.linkUrl();
#else
    QUrl linkUrl = contextMenuRequest->linkUrl();
#endif

    if (linkUrl.isValid()) {
        const QString scheme = linkUrl.scheme();

        if (scheme != QLatin1String("javascript")) {
            m_contextMenu->addAction(tr("Open Link in New Tab"), this, [this]() {
                triggerPageAction(QWebEnginePage::WebAction::OpenLinkInNewWindow);
            });
        }

        if (scheme != QLatin1String("qrc")) {
            if (scheme != QLatin1String("javascript")) {
                m_contextMenu->addAction(tr("Open Link in Desktop Browser"), this, [linkUrl]() {
                    QDesktopServices::openUrl(linkUrl);
                });
            }

            m_contextMenu->addAction(pageAction(QWebEnginePage::CopyLinkToClipboard));
        }
    }


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QString selectedText = contextData.selectedText();
#else
    const QString selectedText = contextMenuRequest->selectedText();
#endif

    if (!selectedText.isEmpty()) {
        if (!m_contextMenu->isEmpty()) {
            m_contextMenu->addSeparator();
        }

        m_contextMenu->addAction(pageAction(QWebEnginePage::Copy));
    }

    if (!linkUrl.isValid() && url().scheme() != QLatin1String("qrc")) {
        if (!m_contextMenu->isEmpty()) {
            m_contextMenu->addSeparator();
        }

        m_contextMenu->addAction(pageAction(QWebEnginePage::Back));
        m_contextMenu->addAction(pageAction(QWebEnginePage::Forward));
        m_contextMenu->addSeparator();

        m_contextMenu->addAction(tr("Open Page in Desktop Browser"), this, [this]() {
            QDesktopServices::openUrl(url());
        });
    }

    if (m_contextMenu->isEmpty()) {
        return;
    }

    m_contextMenu->popup(event->globalPos());
}

bool WebView::handleMouseReleaseEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::BackButton:
        // Check if cursor is still inside webview.
        if (rect().contains(event->pos())) {
            back();
        }

        event->accept();
        return true;

    case Qt::ForwardButton:
        if (rect().contains(event->pos())) {
            forward();
        }

        event->accept();
        return true;

    default:
        break;
    }

    return false;
}

bool WebView::handleWheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const QPoint angleDelta = event->angleDelta();
        int delta = qAbs(angleDelta.x()) > qAbs(angleDelta.y()) ? angleDelta.x() : angleDelta.y();
        const int direction = delta > 0 ? 1 : -1;

        int levelDelta = 0;
        while (delta * direction >= 120) {
            levelDelta += direction;
            delta -= 120 * direction;
        }

        setZoomLevel(m_zoomLevel + levelDelta);
        event->accept();
        return true;
    }

    return false;
}

bool WebView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched->parent() == this) {
        switch (event->type()) {
        case QEvent::MouseButtonRelease:
            if (handleMouseReleaseEvent(static_cast<QMouseEvent *>(event))) {
                return true;
            }

            break;

        case QEvent::Wheel:
            if (handleWheelEvent(static_cast<QWheelEvent *>(event))) {
                return true;
            }

            break;

        default:
            break;
        }
    }

    return QWebEngineView::eventFilter(watched, event);
}
