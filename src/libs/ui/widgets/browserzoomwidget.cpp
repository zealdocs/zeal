#include "browserzoomwidget.h"
#include <QColor>
#include <QPalette>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QString>

BrowserZoomWidget::BrowserZoomWidget(QWidget *parent)
: QWidget(parent)
{
    const auto highlightedBackgroundColor = palette().highlight().color().name();
    const auto highlightedTextColor = palette().highlightedText().color().name();
    const auto styleSheet
        = QString("QPushButton:hover { background-color: %1; color: %2; border: none; }").arg(highlightedBackgroundColor)
                                                                                          .arg(highlightedTextColor);
    setStyleSheet(styleSheet);
    setMouseTracking(true);
    auto zoomLabel = new QLabel(tr("Zoom"));
    zoomLabel->setMouseTracking(true);
    zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    constexpr int maxButtonWidth = 32;

    m_zoomOutButton = new QPushButton(QStringLiteral("-"));
    m_zoomOutButton->setMouseTracking(true);
    m_zoomOutButton->setMaximumWidth(maxButtonWidth);
    m_zoomOutButton->setToolTip(tr("Zoom out"));

    m_zoomOutButton->setFlat(true);
    m_zoomLevelLabel = new QLabel("100%");
    m_zoomLevelLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_zoomLevelLabel->setMouseTracking(true);
    m_zoomLevelLabel->setToolTip(tr("Current zoom level"));

    m_zoomInButton = new QPushButton(QStringLiteral("+"));
    m_zoomInButton->setFlat(true);
    m_zoomInButton->setMouseTracking(true);
    m_zoomInButton->setMaximumWidth(maxButtonWidth);
    m_zoomInButton->setToolTip(tr("Zoom in"));

    m_resetZoomButton = new QPushButton(QStringLiteral("↻"));
    m_resetZoomButton->setFlat(true);
    m_resetZoomButton->setMouseTracking(true);
    m_resetZoomButton->setMaximumWidth(maxButtonWidth);
    m_resetZoomButton->setToolTip(tr("Reset zoom level"));

    auto layout = new QHBoxLayout(this);
    layout->setSpacing(2);
    layout->addWidget(zoomLabel);
    layout->addWidget(m_zoomOutButton);
    layout->addWidget(m_zoomLevelLabel);
    layout->addWidget(m_zoomInButton);
    layout->addWidget(m_resetZoomButton);
}

QPushButton *BrowserZoomWidget::zoomOutButton()
{
    return m_zoomOutButton;
}

QPushButton *BrowserZoomWidget::zoomInButton()
{
    return m_zoomInButton;
}

QPushButton *BrowserZoomWidget::resetZoomButton()
{
    return m_resetZoomButton;
}

QLabel *BrowserZoomWidget::zoomLevelLabel()
{
    return m_zoomLevelLabel;
}
