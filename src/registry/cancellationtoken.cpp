#include "cancellationtoken.h"

using namespace Zeal;

CancellationToken::CancellationToken()
{
    m_cancelled = QSharedPointer<bool>(new bool(false));
}

void CancellationToken::cancel()
{
    *m_cancelled = true;
}

bool CancellationToken::isCancelled() const
{
    return *m_cancelled;
}
