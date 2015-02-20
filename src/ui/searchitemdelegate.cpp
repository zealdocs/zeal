#include "searchitemdelegate.h"

#include "searchitemstyle.h"
#include "registry/searchquery.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>

SearchItemDelegate::SearchItemDelegate(QLineEdit *lineEdit, QWidget *view) :
    QStyledItemDelegate(view),
    m_lineEdit(lineEdit),
    m_view(view)
{
}

void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option_,
                                   const QModelIndex &index) const
{
    painter->save();

    QStyleOptionViewItem option(option_);
    option.text = index.data().toString();
    option.features |= QStyleOptionViewItem::HasDisplay;

    if (!index.data(Qt::DecorationRole).isNull()) {
        option.features |= QStyleOptionViewItem::HasDecoration;
        option.icon = index.data(Qt::DecorationRole).value<QIcon>();
    }

    ZealSearchItemStyle style;
    style.drawControl(QStyle::CE_ItemViewItem, &option, painter, m_view);

    if (option.state & QStyle::State_Selected) {
#ifdef Q_OS_WIN32
        option.palette.setColor(QPalette::All, QPalette::HighlightedText,
                                option.palette.color(QPalette::Active, QPalette::Text));
#endif
        painter->setPen(QPen(option.palette.highlightedText(), 1));
    }

    QRect rect = qApp->style()->subElementRect(QStyle::SE_ItemViewItemText, &option, m_view);
    const int margin = style.pixelMetric(QStyle::PM_FocusFrameHMargin, 0, m_view);
    rect.adjust(margin, 0, 2, 0); // +2px for bold text

    QFont bold(painter->font());
    bold.setBold(true);
    const QFontMetrics metricsBold(bold);

    const QFontMetrics metrics(painter->font());
    QString elided = metrics.elidedText(index.data().toString(), option.textElideMode, rect.width());

    QString highlight;
    if (m_lineEdit)
        highlight = Zeal::SearchQuery::fromString(m_lineEdit->text()).query();

    int from = 0;
    while (from < elided.size()) {
        const int until = highlight.isEmpty() ? -1 : elided.toLower().indexOf(highlight.toLower(), from);

        if (until == -1) {
            painter->drawText(rect, elided.mid(from));
            from = elided.size();
        } else {
            painter->drawText(rect, elided.mid(from, until - from));
            rect.setLeft(rect.left() + metrics.width(elided.mid(from, until - from)));
            QFont old(painter->font());
            painter->setFont(bold);
            painter->drawText(rect, elided.mid(until, highlight.size()));
            painter->setFont(old);
            rect.setLeft(rect.left() + metricsBold.width(elided.mid(until, highlight.size())));
            from = until + highlight.size();
        }
    }

    painter->restore();
}
