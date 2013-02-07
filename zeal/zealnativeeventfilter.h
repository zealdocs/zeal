#ifndef ZEALNATIVEEVENTFILTER_H
#define ZEALNATIVEEVENTFILTER_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QKeySequence>

class ZealNativeEventFilter : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit ZealNativeEventFilter(QObject *parent = 0);
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
    void setEnabled(bool enabled_) { enabled = enabled_; }
    void setHotKey(const QKeySequence& hotKey_) { hotKey = hotKey_; }
signals:
    void gotHotKey();
public slots:
private:
    bool enabled = true;
    QKeySequence hotKey;
};

#endif // ZEALNATIVEEVENTFILTER_H
