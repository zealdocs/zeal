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
        clear();
    }
}

bool ZealSearchEdit::eventFilter(QObject *obj, QEvent *ev)
{
    if(obj == this && ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
        if(keyEvent->key() == Qt::Key_Down) {
            treeView->setFocus();
            return true;
        }

        if(keyEvent->key() == Qt::Key_Escape) {
            clearQuery();
        }
    }
    return QLineEdit::eventFilter(obj, ev);
}

void ZealSearchEdit::focusInEvent(QFocusEvent * evt)
{
    ZealSearchQuery currentQuery(text());
    int selectionOffset = currentQuery.getDocsetFilter().size();
    if(selectionOffset > 0) {
        selectionOffset++; // add the delimeter
    }
    setSelection(selectionOffset, text().size() - selectionOffset);
}
