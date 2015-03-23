#include "zealsearchedit.h"

#include "registry/searchquery.h"

#include <QCompleter>
#include <QKeyEvent>
#include <QLabel>
#include <QTreeView>

ZealSearchEdit::ZealSearchEdit(QWidget *parent) :
    QLineEdit(parent)
{
    setClearButtonEnabled(true);

    m_completionLabel = new QLabel(this);
    m_completionLabel->setObjectName("completer");
    m_completionLabel->setStyleSheet("QLabel#completer { color: gray; }");
    m_completionLabel->setFont(font());

    connect(this, &ZealSearchEdit::textChanged, this, &ZealSearchEdit::showCompletions);
}

void ZealSearchEdit::setTreeView(QTreeView *view)
{
    m_treeView = view;
    m_focusing = false;
}

// Makes the line edit use autocompletions.
void ZealSearchEdit::setCompletions(const QStringList &completions)
{
    if (m_prefixCompleter)
        delete m_prefixCompleter;

    m_prefixCompleter = new QCompleter(completions, this);
    m_prefixCompleter->setCompletionMode(QCompleter::InlineCompletion);
    m_prefixCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_prefixCompleter->setWidget(this);
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

QString ZealSearchEdit::currentCompletion(const QString &text) const
{
    if (text.isEmpty())
        return QString();
    else
        return m_prefixCompleter->currentCompletion();
}

void ZealSearchEdit::showCompletions(const QString &newValue)
{
    const int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    const int textWidth = fontMetrics().width(newValue);

    m_prefixCompleter->setCompletionPrefix(text());

    const QString completed = currentCompletion(newValue).mid(newValue.size());
    const QSize labelSize(fontMetrics().width(completed), size().height());

    m_completionLabel->setMinimumSize(labelSize);
    m_completionLabel->move(frameWidth + 2 + textWidth, 0);
    m_completionLabel->setText(completed);
}

bool ZealSearchEdit::event(QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return QLineEdit::event(event);

    QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(event);
    if (keyEvent->key() != Qt::Key_Tab)
        return QLineEdit::event(event);

    const QString completed = currentCompletion(text());
    if (!completed.isEmpty())
        setText(completed);

    return true;
}

void ZealSearchEdit::focusInEvent(QFocusEvent *event)
{
    // Focus on the widget.
    QLineEdit::focusInEvent(event);

    // Override the default selection.
    Zeal::SearchQuery currentQuery(text());
    int selectionOffset = currentQuery.keywordPrefixSize();
    if (selectionOffset > 0)
        selectionOffset++; // add the delimeter
    setSelection(selectionOffset, text().size() - selectionOffset);
    m_focusing = true;
}

void ZealSearchEdit::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        clear();
        event->accept();
        break;
    case Qt::Key_Return:
        emit m_treeView->activated(m_treeView->selectionModel()->currentIndex());
        event->accept();
        break;
    case Qt::Key_Down:
    case Qt::Key_Up: {
        const QModelIndex index = m_treeView->currentIndex();
        const int nextRow = index.row() + (event->key() == Qt::Key_Down ? 1 : -1);
        const QModelIndex sibling = index.sibling(nextRow, 0);
        if (nextRow >= 0 && nextRow < m_treeView->model()->rowCount())
            m_treeView->setCurrentIndex(sibling);
        event->accept();
        break;
    }
    default:
        QLineEdit::keyPressEvent(event);
        break;
    }
}

void ZealSearchEdit::mousePressEvent(QMouseEvent *event)
{
    // Let the focusInEvent code deal with initial selection on focus.
    if (m_focusing)
        return;

    QLineEdit::mousePressEvent(event);
    m_focusing = false;
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
