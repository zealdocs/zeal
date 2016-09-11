#include "toolbarframe.h"

#include <QPainter>

ToolBarFrame::ToolBarFrame(QWidget *parent) : QWidget(parent)
{
}

void ToolBarFrame::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    // Draw a line at the bottom.
    QPainter painter(this);
    painter.save();
    painter.setPen(palette().dark().color());
    painter.drawLine(0, height() - 1, width(), height() - 1);
    painter.restore();
}
