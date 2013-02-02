#include "zealsearchitemstyle.h"
#include <QDebug>

QRect ZealSearchItemStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
    if(element == QStyle::SE_ItemViewItemText) {
        // do not draw text - delegate does it
        return QRect();
    } else {
        return QProxyStyle::subElementRect(element, option, widget);
    }
}
