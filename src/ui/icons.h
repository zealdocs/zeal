#ifndef ICONS_H
#define ICONS_H

class QIcon;
class QString;

class Icons
{
public:
    static const QString Back;
    static const QString Forward;
    static const QString OpenLink;
    static const QString Plus;
    static const QString CloseCircled;
    static const QString CloseRound;
    static const QString Close;
    static const QString Search;
    static const QString DocumentText;
    static const QString WorldOutline;

    static QIcon getIcon(const QString text);
};

#endif // ICONS_H
