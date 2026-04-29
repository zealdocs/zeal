// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_BROWSERZOOMWIDGET_H
#define ZEAL_WIDGETUI_BROWSERZOOMWIDGET_H

#include <QWidget>

class QLabel;

namespace Zeal::WidgetUi {

class BrowserZoomWidget final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(BrowserZoomWidget)
public:
    explicit BrowserZoomWidget(QWidget *parent = nullptr);
    ~BrowserZoomWidget() override = default;

    void setZoomPercentage(int percent);

signals:
    void zoomInRequested();
    void zoomOutRequested();
    void resetZoomRequested();

private:
    QLabel *m_levelLabel = nullptr;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_BROWSERZOOMWIDGET_H
