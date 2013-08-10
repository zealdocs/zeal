#include "zealsearchitemdelegate.h"
#include "zealsearchitemstyle.h"
#include <QPainter>
#include <QFontMetrics>
#include <QDebug>
#include <QApplication>

#include "zealsearchquery.h"

ZealSearchItemDelegate::ZealSearchItemDelegate(QObject *parent, QLineEdit* lineEdit_, QWidget* view_) :
    QStyledItemDelegate(parent), lineEdit(lineEdit_), view(view_)
{
}

void ZealSearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option_, const QModelIndex &index) const
{
    painter->save();
    QStyleOptionViewItem option(option_);
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    // overriding subElementRect doesn't work with Qt 5.0.0, but is required to display
    // selected item frame correctly in Windows (for patch see https://codereview.qt-project.org/#change,46559)
    option.text = index.data().toString();
    option.features |= QStyleOptionViewItem::HasDisplay;
#endif
    if(!index.data(Qt::DecorationRole).isNull()) {
        option.features |= QStyleOptionViewItem::HasDecoration;
        option.icon = index.data(Qt::DecorationRole).value<QIcon>();
    }

    ZealSearchItemStyle style;
    style.drawControl(QStyle::CE_ItemViewItem, &option, painter, view);

    if(option.state & QStyle::State_Selected) {
#ifdef WIN32
        option.palette.setColor(QPalette::All, QPalette::HighlightedText, option.palette.color(QPalette::Active, QPalette::Text));
#endif
        painter->setPen(QPen(option.palette.highlightedText(), 1));
    }

    auto rect = static_cast<QApplication*>(QApplication::instance())->style()->subElementRect(QStyle::SE_ItemViewItemText, &option, view);
    const int margin = style.pixelMetric(QStyle::PM_FocusFrameHMargin, 0, view);
    rect.adjust(margin, 0, 2, 0); // +2px for bold text

    QFontMetrics metrics(painter->font());
    QFont bold(painter->font());;
    bold.setBold(true);
    QFontMetrics metricsBold(bold);
    auto elided = metrics.elidedText(index.data().toString(), option.textElideMode, rect.width());
    QString highlight;
    if(lineEdit) {
        highlight = ZealSearchQuery(lineEdit->text()).getCoreQuery();
    }
    int from = 0;
    while(from < elided.size()) {
        int until = elided.toLower().indexOf(highlight.toLower(), from);
        if(!highlight.size()) until = -1;

        if(until == -1) {
            painter->drawText(rect, elided.mid(from));
            from = elided.size();
        } else {
            painter->drawText(rect, elided.mid(from, until-from));
            rect.setLeft(rect.left() + metrics.width(elided.mid(from, until-from)));
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
