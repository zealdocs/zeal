#include "zealsearchedit.h"
#include <QCoreApplication>
#include <QKeyEvent>
#include <QDebug>

#include "zealsearchquery.h"

ZealSearchEdit::ZealSearchEdit(QWidget *parent) :
    LineEdit(parent)
{
}

void ZealSearchEdit::setTreeView(QTreeView *view)
{
    treeView = view;
    this->installEventFilter(this);
}

bool ZealSearchEdit::eventFilter(QObject *obj, QEvent *ev)
{
    if(obj == this && ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
        if(keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up ||
                keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return ||
                keyEvent->key() == Qt::Key_PageUp || keyEvent->key() == Qt::Key_PageDown) {
            QCoreApplication::instance()->sendEvent(treeView, keyEvent);
            return true;
        }

        // Clear input with consideration to docset filters when the user presses escape.
        if(keyEvent->key() == Qt::Key_Escape) {
            ZealSearchQuery currentQuery(text());
            // Keep the filter for the first esc press
            if(currentQuery.getDocsetFilter().size() > 0 && currentQuery.getCoreQuery().size() > 0) {
                setText(currentQuery.getDocsetFilter() + ZealSearchQuery::DOCSET_FILTER_SEPARATOR);
            } else {
                clear();
            }
        }
    }
    return QLineEdit::eventFilter(obj, ev);
}
