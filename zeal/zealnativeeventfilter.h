#ifndef ZEALNATIVEEVENTFILTER_H
#define ZEALNATIVEEVENTFILTER_H

#include <QObject>
#include <QAbstractNativeEventFilter>

class ZealNativeEventFilter : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit ZealNativeEventFilter(QObject *parent = 0);
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
signals:
    void gotHotKey();
public slots:
    
};

#endif // ZEALNATIVEEVENTFILTER_H
