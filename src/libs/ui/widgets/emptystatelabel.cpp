// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "emptystatelabel.h"

#include <QAbstractScrollArea>
#include <QEvent>

namespace Zeal::WidgetUi {

namespace {
constexpr int EmptyStateMargin = 24;
} // namespace

EmptyStateLabel::EmptyStateLabel(QAbstractScrollArea *view, const QString &text)
    : QLabel(text, view->viewport())
{
    setAlignment(Qt::AlignCenter);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setEnabled(false);
    setWordWrap(true);
    hide();

    view->viewport()->installEventFilter(this);

    updateBounds();
}

bool EmptyStateLabel::eventFilter(QObject *object, QEvent *event)
{
    if (object == parentWidget() && event->type() == QEvent::Resize) {
        updateBounds();
    }

    return QLabel::eventFilter(object, event);
}

void EmptyStateLabel::setEmpty(bool empty)
{
    setVisible(empty);
    if (empty) {
        updateBounds();
        raise();
    }
}

void EmptyStateLabel::updateBounds()
{
    if (const QWidget *viewport = parentWidget()) {
        setGeometry(
            viewport->rect().adjusted(EmptyStateMargin, EmptyStateMargin, -EmptyStateMargin, -EmptyStateMargin));
    }
}

} // namespace Zeal::WidgetUi
