#include "zealsearchedit.h"

#include "registry/searchquery.h"

#include <QKeyEvent>

ZealSearchEdit::ZealSearchEdit(QWidget *parent) :
    QLineEdit(parent)
{
    setClearButtonEnabled(true);

    completionLabel = new QLabel(this);
    completionLabel->setObjectName("completer");
    completionLabel->setStyleSheet("QLabel#completer { color: gray; }");
    completionLabel->setFont(font());

    connect(this, &ZealSearchEdit::textChanged, this, &ZealSearchEdit::showCompletions);
}

void ZealSearchEdit::setTreeView(QTreeView *view)
{
    treeView = view;
    focusing = false;
}

// Makes the line edit use autocompletions.
void ZealSearchEdit::setCompletions(const QStringList &completions)
{
    prefixCompleter = new QCompleter(completions, this);
    prefixCompleter->setCompletionMode(QCompleter::InlineCompletion);
    prefixCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    prefixCompleter->setWidget(this);
}

bool ZealSearchEdit::event(QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return QLineEdit::event(event);

    QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(event);
    if (keyEvent->key() != Qt::Key_Tab)
        return QLineEdit::event(event);

    QString currentText = text();
    QString completed = currentCompletion(currentText);
    if (!completed.isEmpty())
        setText(completed);

    return true;
}

int ZealSearchEdit::queryStart() const
{
    Zeal::SearchQuery currentQuery = Zeal::SearchQuery::fromString(text());
    // Keep the filter for the first esc press
    if (currentQuery.keywordPrefixSize() > 0 && currentQuery.query().size() > 0)
        return currentQuery.keywordPrefixSize() + 1;
    else
        return 0;
}

// Clear input with consideration to docset filters
void ZealSearchEdit::clearQuery()
{
    setText(text().left(queryStart()));
}

void ZealSearchEdit::selectQuery()
{
    setSelection(queryStart(), text().size());
}

void ZealSearchEdit::clear()
{
    clearQuery();
}

void ZealSearchEdit::focusInEvent(QFocusEvent *evt)
{
    // Focus on the widget.
    QLineEdit::focusInEvent(evt);

    // Override the default selection.
    Zeal::SearchQuery currentQuery(text());
    int selectionOffset = currentQuery.keywordPrefixSize();
    if (selectionOffset > 0)
        selectionOffset++; // add the delimeter
    setSelection(selectionOffset, text().size() - selectionOffset);
    focusing = true;
}

void ZealSearchEdit::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        clear();
        event->accept();
        break;
    case Qt::Key_Return:
        emit treeView->activated(treeView->selectionModel()->currentIndex());
        event->accept();
        break;
    case Qt::Key_Down:
    case Qt::Key_Up: {
        QModelIndex index = treeView->currentIndex();
        int nextRow = event->key() == Qt::Key_Down
                ? index.row() + 1
                : index.row() - 1;
        QModelIndex sibling = index.sibling(nextRow, 0);
        if (nextRow >= 0 && nextRow < treeView->model()->rowCount())
            treeView->setCurrentIndex(sibling);
        event->accept();
        break;
    }
    default:
        QLineEdit::keyPressEvent(event);
        break;
    }
}

void ZealSearchEdit::mousePressEvent(QMouseEvent *ev)
{
    // Let the focusInEvent code deal with initial selection on focus.
    if (!focusing)
        QLineEdit::mousePressEvent(ev);
    focusing = false;
}

QString ZealSearchEdit::currentCompletion(const QString &text)
{
    if (text.isEmpty())
        return QString();
    else
        return prefixCompleter->currentCompletion();
}

void ZealSearchEdit::showCompletions(const QString &newValue)
{
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    int textWidth = fontMetrics().width(newValue);

    prefixCompleter->setCompletionPrefix(text());

    QString completed = currentCompletion(newValue).mid(newValue.size());
    QSize labelSize(fontMetrics().width(completed), size().height());

    completionLabel->setMinimumSize(labelSize);
    completionLabel->move(frameWidth + 2 + textWidth, 0);
    completionLabel->setText(completed);
}
