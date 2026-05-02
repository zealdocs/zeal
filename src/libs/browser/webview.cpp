// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webview.h"

#include "webcontrol.h"
#include "webpage.h"

#include <ui/browsertab.h>
#include <ui/mainwindow.h>

#include <QApplication>
#include <QCheckBox>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWheelEvent>

namespace Zeal::Browser {

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
    level = qBound(0, level, static_cast<int>(availableZoomLevels().size()) - 1);

    if (level == m_zoomLevel) {
        return;
    }

    m_zoomLevel = level;
    setZoomFactor(availableZoomLevels().at(level) / 100.0);

    emit zoomLevelChanged();
}

const QList<int> &WebView::availableZoomLevels()
{
    // clang-format off
    static const QList<int> zoomLevels = {30, 40, 50, 67, 80, 90, 100,
                                          110, 120, 133, 150, 170, 200,
                                          220, 233, 250, 270, 285, 300};
    // clang-format on

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
    auto *mw = qobject_cast<WidgetUi::MainWindow *>(window());
    if (mw == nullptr) {
        return nullptr;
    }

    const bool activate = (type != QWebEnginePage::WebBrowserBackgroundTab);
    return mw->createTab(QUrl(QStringLiteral("about:blank")), activate)->webControl()->m_webView;
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    QWebEngineContextMenuRequest *contextMenuRequest = lastContextMenuRequest();
    if (contextMenuRequest == nullptr) {
        QWebEngineView::contextMenuEvent(event);
        return;
    }

    event->accept();

    if (m_contextMenu) {
        m_contextMenu->deleteLater();
    }

    m_contextMenu = new QMenu(this);

    const QUrl linkUrl = contextMenuRequest->linkUrl();
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

    const QString selectedText = contextMenuRequest->selectedText();
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

} // namespace Zeal::Browser
