/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Copyright (C) 2015 Artur Spychaj
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#ifndef ZEAL_REGISTRY_CANCELLATIONTOKEN_H
#define ZEAL_REGISTRY_CANCELLATIONTOKEN_H

#include <atomic>

namespace Zeal {
namespace Registry {

/// Token that stores whether cancel was called on it.
/// In async code can be used to check if another thread called cancel.
class CancellationToken
{
public:
    inline bool isCanceled() const { return m_canceled; }

    inline void cancel() { m_canceled = true; }
    inline void reset() { m_canceled = false; }

private:
    std::atomic_bool m_canceled;
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_CANCELLATIONTOKEN_H
