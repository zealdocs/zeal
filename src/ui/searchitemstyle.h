#ifndef SEARCHITEMSTYLE_H
#define SEARCHITEMSTYLE_H

#include <QProxyStyle>

class ZealSearchItemStyle : public QProxyStyle
{
public:
    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget) const;
};

#endif // SEARCHITEMSTYLE_H
