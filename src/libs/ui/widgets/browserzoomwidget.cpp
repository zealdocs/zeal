// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "browserzoomwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

namespace Zeal::WidgetUi {

namespace {
constexpr int ButtonWidth = 32;
} // namespace

BrowserZoomWidget::BrowserZoomWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *captionLabel = new QLabel(tr("Zoom"));
    captionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *zoomOutButton = new QToolButton();
    zoomOutButton->setAutoRaise(true);
    zoomOutButton->setFixedWidth(ButtonWidth);
    zoomOutButton->setText(QStringLiteral("−"));
    zoomOutButton->setToolTip(tr("Zoom out"));
    connect(zoomOutButton, &QToolButton::clicked, this, &BrowserZoomWidget::zoomOutRequested);

    m_levelLabel = new QLabel(QStringLiteral("100%"));
    m_levelLabel->setAlignment(Qt::AlignCenter);
    m_levelLabel->setMinimumWidth(ButtonWidth);
    m_levelLabel->setToolTip(tr("Current zoom level"));

    auto *zoomInButton = new QToolButton();
    zoomInButton->setAutoRaise(true);
    zoomInButton->setFixedWidth(ButtonWidth);
    zoomInButton->setText(QStringLiteral("+"));
    zoomInButton->setToolTip(tr("Zoom in"));
    connect(zoomInButton, &QToolButton::clicked, this, &BrowserZoomWidget::zoomInRequested);

    auto *resetButton = new QToolButton();
    resetButton->setAutoRaise(true);
    resetButton->setFixedWidth(ButtonWidth);
    resetButton->setText(QStringLiteral("↻"));
    resetButton->setToolTip(tr("Actual size"));
    connect(resetButton, &QToolButton::clicked, this, &BrowserZoomWidget::resetZoomRequested);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(2);
    layout->addWidget(captionLabel, 1);
    layout->addWidget(zoomOutButton);
    layout->addWidget(m_levelLabel);
    layout->addWidget(zoomInButton);
    layout->addWidget(resetButton);
}

void BrowserZoomWidget::setZoomPercentage(int percent)
{
    m_levelLabel->setText(QStringLiteral("%1%").arg(percent));
}

} // namespace Zeal::WidgetUi
