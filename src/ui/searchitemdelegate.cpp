#include "searchitemdelegate.h"

#include "searchitemstyle.h"
#include "registry/searchmodel.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>

SearchItemDelegate::SearchItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option_,
                               const QModelIndex &index) const
{
    if (m_highlight.isEmpty()) {
        QStyledItemDelegate::paint(painter, option_, index);
        return;
    }

    painter->save();

    QStyleOptionViewItem option(option_);
    option.text = index.data().toString();
    option.features |= QStyleOptionViewItem::HasDisplay;

    if (!index.data(Qt::DecorationRole).isNull()) {
        option.features |= QStyleOptionViewItem::HasDecoration;
        option.icon = index.data(Zeal::SearchModel::DocsetIconRole).value<QIcon>();
    }

    ZealSearchItemStyle style;
    style.drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

    if (option.state & QStyle::State_Selected) {
#ifdef Q_OS_WIN32
        option.palette.setColor(QPalette::All, QPalette::HighlightedText,
                                option.palette.color(QPalette::Active, QPalette::Text));
#endif
        painter->setPen(QPen(option.palette.highlightedText(), 1));
    }

    QRect rect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &option,
                                                       option.widget);
    const int margin = style.pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget);
    rect.adjust(margin, 0, 2, 0); // +2px for bold text

    const QFont defaultFont(painter->font());
    QFont boldFont(defaultFont);
    boldFont.setBold(true);

    const QFontMetrics metrics(defaultFont);
    const QFontMetrics metricsBold(boldFont);

    const QString elided = metrics.elidedText(option.text, option.textElideMode, rect.width());

    int from = 0;
    while (from < elided.size()) {
        const int to = elided.indexOf(m_highlight, from, Qt::CaseInsensitive);

        if (to == -1) {
            painter->drawText(rect, elided.mid(from));
            break;
        }

        QString text = elided.mid(from, to - from);
        painter->drawText(rect, text);
        rect.setLeft(rect.left() + metrics.width(text));

        text = elided.mid(to, m_highlight.size());
        painter->setFont(boldFont);
        painter->drawText(rect, text);
        rect.setLeft(rect.left() + metricsBold.width(text));

        painter->setFont(defaultFont);

        from = to + m_highlight.size();
    }

    painter->restore();
}

void SearchItemDelegate::setHighlight(const QString &text)
{
    m_highlight = text;
}
