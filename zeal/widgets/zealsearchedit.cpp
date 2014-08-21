#include "zealsearchedit.h"
#include <QKeyEvent>

#include "../zealsearchquery.h"

ZealSearchEdit::ZealSearchEdit(QWidget *parent) :
    LineEdit(parent)
{
    completionLabel = new QLabel(this);
    completionLabel->setObjectName("completer");
    completionLabel->setStyleSheet("QLabel#completer { color: gray; }");
    completionLabel->setFont(this->font());

    connect(this, &ZealSearchEdit::textChanged, this, &ZealSearchEdit::showCompletions);
}

void ZealSearchEdit::setTreeView(QTreeView *view)
{
    treeView = view;
    focusing = false;
    this->installEventFilter(this);
}

// Makes the line edit use autocompletions.
void ZealSearchEdit::setCompletions(QStringList completions)
{
    prefixCompleter = new QCompleter(completions, this);
    prefixCompleter->setCompletionMode(QCompleter::InlineCompletion);
    prefixCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    prefixCompleter->setWidget(this);
}

int ZealSearchEdit::queryStart()
{
    ZealSearchQuery currentQuery(text());
    // Keep the filter for the first esc press
    if(currentQuery.getDocsetFilter().size() > 0 && currentQuery.getCoreQuery().size() > 0) {
        return currentQuery.getDocsetFilter().size() + 1;
    } else {
        return 0;
    }
}

// Clear input with consideration to docset filters
void ZealSearchEdit::clearQuery()
{
    QString query = text();
    query.chop(query.size() - queryStart());
    setText(query);
}

void ZealSearchEdit::selectQuery()
{
    setSelection(queryStart(), text().size());
}

void ZealSearchEdit::clear()
{
    this->clearQuery();
}

bool ZealSearchEdit::eventFilter(QObject *obj, QEvent *ev)
{
    if(obj == this && ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);

        if(keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) {
            QModelIndex index = treeView->currentIndex();
            int nextRow = keyEvent->key() == Qt::Key_Down
                    ? index.row() + 1
                    : index.row() - 1;
            QModelIndex sibling = index.sibling(nextRow, 0);
            if (nextRow >= 0 && nextRow < treeView->model()->rowCount())
            {
                treeView->setCurrentIndex(sibling);
                return true;
            }
        }

        if(keyEvent->key() == Qt::Key_Return) {
            emit treeView->activated(treeView->selectionModel()->currentIndex());
            return true;
        }

        // Autocompletes the prefixes.
        if (keyEvent->key() == Qt::Key_Tab) {
            QString currentText = text();
            QString completed = this->currentCompletion(currentText);
            if (!completed.isEmpty()) {
                this->setText(completed);
                return true;
            }
        }
    }
    return LineEdit::eventFilter(obj, ev);
}

void ZealSearchEdit::resizeEvent(QResizeEvent *ev)
{
    QString text = this->text();
    showCompletions(text);
    LineEdit::resizeEvent(ev);
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

QString ZealSearchEdit::currentCompletion(const QString &text)
{
    if (text.isEmpty()) {
        return QString();
    } else {
        return this->prefixCompleter->currentCompletion();
    }
}

void ZealSearchEdit::showCompletions(const QString &newValue)
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int textWidth = fontMetrics().width(newValue);

    this->prefixCompleter->setCompletionPrefix(this->text());

    QString completed = this->currentCompletion(newValue).mid(newValue.size());
    QSize labelSize(fontMetrics().width(completed), size().height());

    completionLabel->setMinimumSize(labelSize);
    completionLabel->move(frameWidth + 2 + textWidth, frameWidth);
    completionLabel->setText(completed);
}
