#ifndef ZEALNATIVEEVENTFILTER_H
#define ZEALNATIVEEVENTFILTER_H

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QKeySequence>

namespace Zeal {

class NativeEventFilter : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit NativeEventFilter(QObject *parent = nullptr);

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

    void setEnabled(bool enabled);
    void setHotKey(const QKeySequence &key);

signals:
    void hotKeyPressed();

private:
    bool m_enabled;
    QKeySequence m_hotKey;
};

} // namespace Zeal

#endif // ZEALNATIVEEVENTFILTER_H
