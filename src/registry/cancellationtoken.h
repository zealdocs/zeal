#ifndef CANCELLATIONTOKEN_H
#define CANCELLATIONTOKEN_H

#include <QSharedPointer>

namespace Zeal {

/// Token that stores whether cancel was called on it.
/// In async code can be used to check if another thread called cancel.
struct CancellationToken
{
public:
    CancellationToken();
    bool isCancelled() const;
    void cancel();

private:
    QSharedPointer<bool> m_cancelled;
};

}

Q_DECLARE_METATYPE(Zeal::CancellationToken)

#endif // CANCELLATIONTOKEN_H
