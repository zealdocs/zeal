// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "browserzoomwidget.h"

#include "iconhelper.h"

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

    m_zoomOutButton = new QToolButton();
    m_zoomOutButton->setAutoRaise(true);
    m_zoomOutButton->setFixedWidth(ButtonWidth);
    // Light, symmetric fallback glyphs (−, +, ↻) suit this inline stepper better
    // than the bulkier magnifier icons used for the equivalent View → Zoom menu
    // actions; a themed desktop still substitutes its own zoom icons.
    m_zoomOutButton->setIcon(
        IconHelper::fromTheme(QStringLiteral("zoom-out"), QStringLiteral(":/icons/tabler/minus.svg")));
    m_zoomOutButton->setText(QStringLiteral("−"));
    m_zoomOutButton->setToolTip(tr("Zoom out"));
    connect(m_zoomOutButton, &QToolButton::clicked, this, &BrowserZoomWidget::zoomOutRequested);

    m_levelLabel = new QLabel(QStringLiteral("100%"));
    m_levelLabel->setAlignment(Qt::AlignCenter);
    m_levelLabel->setMinimumWidth(ButtonWidth);
    m_levelLabel->setToolTip(tr("Current zoom level"));

    m_zoomInButton = new QToolButton();
    m_zoomInButton->setAutoRaise(true);
    m_zoomInButton->setFixedWidth(ButtonWidth);
    m_zoomInButton->setIcon(
        IconHelper::fromTheme(QStringLiteral("zoom-in"), QStringLiteral(":/icons/tabler/plus.svg")));
    m_zoomInButton->setText(QStringLiteral("+"));
    m_zoomInButton->setToolTip(tr("Zoom in"));
    connect(m_zoomInButton, &QToolButton::clicked, this, &BrowserZoomWidget::zoomInRequested);

    auto *resetButton = new QToolButton();
    resetButton->setAutoRaise(true);
    resetButton->setFixedWidth(ButtonWidth);
    resetButton->setIcon(
        IconHelper::fromTheme(QStringLiteral("zoom-original"), QStringLiteral(":/icons/tabler/rotate-clockwise.svg")));
    resetButton->setText(QStringLiteral("↻"));
    resetButton->setToolTip(tr("Actual size"));
    connect(resetButton, &QToolButton::clicked, this, &BrowserZoomWidget::resetZoomRequested);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(2);
    layout->addWidget(captionLabel, 1);
    layout->addWidget(m_zoomOutButton);
    layout->addWidget(m_levelLabel);
    layout->addWidget(m_zoomInButton);
    layout->addWidget(resetButton);
}

void BrowserZoomWidget::setZoomState(int percent, bool canZoomOut, bool canZoomIn)
{
    m_levelLabel->setText(QStringLiteral("%1%").arg(percent));
    m_zoomOutButton->setEnabled(canZoomOut);
    m_zoomInButton->setEnabled(canZoomIn);
}

} // namespace Zeal::WidgetUi
