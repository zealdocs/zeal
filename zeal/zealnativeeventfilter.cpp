#include "zealnativeeventfilter.h"

#ifdef WIN32
#include <windows.h>
#endif

ZealNativeEventFilter::ZealNativeEventFilter(QObject *parent) :
    QAbstractNativeEventFilter(), QObject(parent)
{
}

bool ZealNativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
#ifdef WIN32
    MSG* msg = static_cast<MSG*>(message);

    if(WM_HOTKEY == msg->message && msg->wParam == 10) {
        emit gotHotKey();
        return true;
    }
#endif // WIN32
    return false;
}
