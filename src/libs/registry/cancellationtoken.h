/****************************************************************************
**
** Copyright (C) 2015 Artur Spychaj
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef CANCELLATIONTOKEN_H
#define CANCELLATIONTOKEN_H

#include <QSharedPointer>

namespace Zeal {
namespace Registry {

/// Token that stores whether cancel was called on it.
/// In async code can be used to check if another thread called cancel.
struct CancellationToken
{
public:
    CancellationToken();
    bool isCanceled() const;
    void cancel();

private:
    QSharedPointer<bool> m_cancelled;
};

} // namespace Registry
} // namespace Zeal

Q_DECLARE_METATYPE(Zeal::Registry::CancellationToken)

#endif // CANCELLATIONTOKEN_H
