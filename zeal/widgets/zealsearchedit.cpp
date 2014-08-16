#include "zealsearchedit.h"
#include <QCoreApplication>
#include <QKeyEvent>
#include <QDebug>

#include "../zealsearchquery.h"

ZealSearchEdit::ZealSearchEdit(QWidget *parent) :
    LineEdit(parent)
{
}

void ZealSearchEdit::setTreeView(QTreeView *view)
{
    treeView = view;
    focusing = false;
    this->installEventFilter(this);
}

// Clear input with consideration to docset filters
void ZealSearchEdit::clearQuery()
{
    ZealSearchQuery currentQuery(text());
    // Keep the filter for the first esc press
    if(currentQuery.getDocsetFilter().size() > 0 && currentQuery.getCoreQuery().size() > 0) {
        setText(currentQuery.getDocsetFilter() + ZealSearchQuery::DOCSET_FILTER_SEPARATOR);
    } else {
        LineEdit::clear();
    }
}

void ZealSearchEdit::clear()
{
    this->clearQuery();
}

bool ZealSearchEdit::eventFilter(QObject *obj, QEvent *ev)
{
    if(obj == this && ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
        if(keyEvent->key() == Qt::Key_Down) {
            treeView->setFocus();
            return true;
        }

        if(keyEvent->key() == Qt::Key_Return) {
            emit treeView->activated(treeView->selectionModel()->currentIndex());
            return true;
        }
    }
    return LineEdit::eventFilter(obj, ev);
}

void ZealSearchEdit::focusInEvent(QFocusEvent * evt)
{
    // Focus on the widget.
    LineEdit::focusInEvent(evt);

    // Override the default selection.
    ZealSearchQuery currentQuery(text());
    int selectionOffset = currentQuery.getDocsetFilter().size();
    if(selectionOffset > 0) {
        selectionOffset++; // add the delimeter
    }
    setSelection(selectionOffset, text().size() - selectionOffset);
    focusing = true;
}

void ZealSearchEdit::mousePressEvent(QMouseEvent *ev)
{
    // Let the focusInEvent code deal with initial selection on focus.
    if (!focusing) {
        LineEdit::mousePressEvent(ev);
    }
    focusing = false;
}
