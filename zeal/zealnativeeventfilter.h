#ifndef ZEALNATIVEEVENTFILTER_H
#define ZEALNATIVEEVENTFILTER_H

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QKeySequence>

class ZealNativeEventFilter : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit ZealNativeEventFilter(QObject *parent = nullptr);

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

    void setEnabled(bool enabled);
    void setHotKey(const QKeySequence &key);

signals:
    void hotKeyPressed();

private:
    bool m_enabled;
    QKeySequence m_hotKey;
};

#endif // ZEALNATIVEEVENTFILTER_H
