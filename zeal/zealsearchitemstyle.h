#ifndef ZEALSEARCHITEMSTYLE_H
#define ZEALSEARCHITEMSTYLE_H

#include <QProxyStyle>

class ZealSearchItemStyle : public QProxyStyle
{
public:
    QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const;
    
};

#endif // ZEALSEARCHITEMSTYLE_H
