/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/
// from http://git.forwardbias.in/?p=lineeditclearbutton.git;a=blob_plain;f=lineedit.cpp;hb=HEAD
#include "lineedit.h"
#include <QToolButton>
#include <QStyle>
#include <QApplication>

LineEdit::LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    clearButton = new QToolButton(this);
    clearButton->setIcon(static_cast<QApplication*>(QApplication::instance())->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    clearButton->setToolTip("Clear");
    clearButton->hide();
    hideOnClear = false;
    connect(clearButton, &QToolButton::clicked, this, &LineEdit::clear);
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(clearButton->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void LineEdit::clear() {
    QLineEdit::clear();
    if(hideOnClear) hide();
}

void LineEdit::resizeEvent(QResizeEvent *)
{
    QSize sz = clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    clearButton->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height())/2);
}

void LineEdit::updateCloseButton(const QString& text)
{
    clearButton->setVisible(!text.isEmpty());
}

