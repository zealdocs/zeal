/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
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

#include "webview.h"

#include "webviewtab.h"
#include "../mainwindow.h"

#include <core/application.h>
#include <core/settings.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QWheelEvent>

using namespace Zeal::WidgetUi;

WebView::WebView(QWidget *parent)
    : QWebView(parent)
{
    page()->setNetworkAccessManager(Core::Application::instance()->networkManager());
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

    setZoomFactor(availableZoomLevels().at(level) / 100.0);
    emit zoomLevelChanged();
}

const QVector<int> &WebView::availableZoomLevels()
{
    static const QVector<int> zoomLevels = {30, 40, 50, 67, 80, 90, 100,
                                            110, 120, 133, 150, 170, 200,
                                            220, 233, 250, 270, 285, 300};
    return zoomLevels;
}

const int &WebView::defaultZoomLevel()
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

QWebView *WebView::createWindow(QWebPage::WebWindowType type)
{
    Q_UNUSED(type)
    return Core::Application::instance()->mainWindow()->createTab()->m_webView;
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
#if QT_VERSION >= 0x050600
    const QWebHitTestResult hitTestResult = hitTestContent(event->pos());

    // Return standard menu for input fields.
    if (hitTestResult.isContentEditable()) {
        QWebView::contextMenuEvent(event);
        return;
    }

    event->accept();

    if (m_contextMenu) {
        m_contextMenu->deleteLater();
    }

    QMenu *m_contextMenu = new QMenu(this);

    const QUrl linkUrl = hitTestResult.linkUrl();
    const QString linkText = hitTestResult.linkText();

    if (linkUrl.isValid()) {
        const QString scheme = linkUrl.scheme();

        if (scheme != QLatin1String("javascript")) {
            m_contextMenu->addAction(tr("Open Link in New Tab"), this, [this]() {
                triggerPageAction(QWebPage::WebAction::OpenLinkInNewWindow);
            });
        }

        if (scheme != QLatin1String("qrc")) {
            if (scheme != QLatin1String("javascript")) {
                m_contextMenu->addAction(tr("Open Link in Desktop Browser"), this, [linkUrl]() {
                    QDesktopServices::openUrl(linkUrl);
                });
            }

            m_contextMenu->addAction(pageAction(QWebPage::CopyLinkToClipboard));
        }
    }

    if (hitTestResult.isContentSelected()) {
        if (!m_contextMenu->isEmpty()) {
            m_contextMenu->addSeparator();
        }

        m_contextMenu->addAction(pageAction(QWebPage::Copy));
    }

    if (!linkUrl.isValid() && url().scheme() != QLatin1String("qrc")) {
        if (!m_contextMenu->isEmpty()) {
            m_contextMenu->addSeparator();
        }

        m_contextMenu->addAction(pageAction(QWebPage::Back));
        m_contextMenu->addAction(pageAction(QWebPage::Forward));
        m_contextMenu->addSeparator();

        m_contextMenu->addAction(tr("Open Page in Desktop Browser"), this, [this]() {
            QDesktopServices::openUrl(url());
        });
    }

    if (m_contextMenu->isEmpty()) {
        return;
    }

    m_contextMenu->popup(event->globalPos());
#else
    QWebView::contextMenuEvent(event);
#endif
}

void WebView::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::BackButton:
        back();
        event->accept();
        return;
    case Qt::ForwardButton:
        forward();
        event->accept();
        return;
    case Qt::LeftButton:
    case Qt::MiddleButton:
        m_clickedLink = hitTestContent(event->pos()).linkUrl();
        if (!m_clickedLink.isValid() || m_clickedLink.scheme() == QLatin1String("javascript")) {
            break;
        }

        event->accept();
        return;
    default:
        break;
    }

    QWebView::mousePressEvent(event);
}

void WebView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton && event->button() != Qt::MiddleButton) {
        QWebView::mouseReleaseEvent(event);
        return;
    }

    const QUrl clickedLink = hitTestContent(event->pos()).linkUrl();
    if (!clickedLink.isValid() || clickedLink != m_clickedLink
            || clickedLink.scheme() == QLatin1String("javascript")) {
        QWebView::mouseReleaseEvent(event);
        return;
    }

    if (isUrlExternal(clickedLink)) {
        switch (Core::Application::instance()->settings()->externalLinkPolicy) {
        case Core::Settings::ExternalLinkPolicy::Open:
            break;
        case Core::Settings::ExternalLinkPolicy::Ask: {
            QScopedPointer<QMessageBox> mb(new QMessageBox());
            mb->setIcon(QMessageBox::Question);
            mb->setText(tr("How do you want to open the external link?<br>URL: <b>%1</b>")
                        .arg(clickedLink.toString()));


            QCheckBox *checkBox = new QCheckBox("Do &not ask again");
            mb->setCheckBox(checkBox);

            QPushButton *openInBrowserButton = mb->addButton(tr("Open in &Desktop Browser"),
                                                             QMessageBox::ActionRole);
            QPushButton *openInZealButton = mb->addButton(tr("Open in &Zeal"),
                                                          QMessageBox::ActionRole);
            mb->addButton(QMessageBox::Cancel);

            mb->setDefaultButton(openInBrowserButton);

            if (mb->exec() == QMessageBox::Cancel) {
                event->accept();
                return;
            }

            if (mb->clickedButton() == openInZealButton) {
                if (checkBox->isChecked()) {
                    Core::Application::instance()->settings()->externalLinkPolicy
                            = Core::Settings::ExternalLinkPolicy::Open;
                    Core::Application::instance()->settings()->save();
                }

                break;
            } else if (mb->clickedButton() == openInBrowserButton) {
                if (checkBox->isChecked()) {
                    Core::Application::instance()->settings()->externalLinkPolicy
                            = Core::Settings::ExternalLinkPolicy::OpenInSystemBrowser;
                    Core::Application::instance()->settings()->save();
                }
            }
        }
        case Core::Settings::ExternalLinkPolicy::OpenInSystemBrowser:
            QDesktopServices::openUrl(clickedLink);
            event->accept();
            return;
        }
    }

    switch (event->button()) {
    case Qt::LeftButton:
        if (!(event->modifiers() & Qt::ControlModifier || event->modifiers() & Qt::ShiftModifier)) {
            load(clickedLink);
            event->accept();
            return;
        }
    case Qt::MiddleButton:
        createWindow(QWebPage::WebBrowserWindow)->load(clickedLink);
        event->accept();
        return;
    default:
        break;
    }

    QWebView::mouseReleaseEvent(event);
}

void WebView::wheelEvent(QWheelEvent *event)
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
        return;
    }

    QWebView::wheelEvent(event);
}

QWebHitTestResult WebView::hitTestContent(const QPoint &pos) const
{
    return page()->mainFrame()->hitTestContent(pos);
}

bool WebView::isUrlExternal(const QUrl &url)
{
    static const QStringList localSchemes = {
        QStringLiteral("file"),
        QStringLiteral("qrc"),
    };

    const QString scheme = url.scheme();
    return !scheme.isEmpty() && !localSchemes.contains(scheme);
}
