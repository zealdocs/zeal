// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_BROWSERZOOMWIDGET_H
#define ZEAL_WIDGETUI_BROWSERZOOMWIDGET_H

#include <QWidget>

class QLabel;
class QToolButton;

namespace Zeal::WidgetUi {

class BrowserZoomWidget final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(BrowserZoomWidget)
public:
    explicit BrowserZoomWidget(QWidget *parent = nullptr);
    ~BrowserZoomWidget() override = default;

    void setZoomState(int percent, bool canZoomOut, bool canZoomIn);

signals:
    void zoomInRequested();
    void zoomOutRequested();
    void resetZoomRequested();

private:
    QLabel *m_levelLabel = nullptr;
    QToolButton *m_zoomInButton = nullptr;
    QToolButton *m_zoomOutButton = nullptr;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_BROWSERZOOMWIDGET_H
