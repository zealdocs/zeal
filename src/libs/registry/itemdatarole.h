/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#ifndef ZEAL_REGISTRY_ITEMDATAROLE_H
#define ZEAL_REGISTRY_ITEMDATAROLE_H

#include <Qt>

namespace Zeal {
namespace Registry {

enum ItemDataRole {
    DocsetIconRole = Qt::UserRole,
    DocsetNameRole,
    UpdateAvailableRole,
    UrlRole
};

enum SectionIndex {
    Name,
    SearchPrefix,
    Actions
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_ITEMDATAROLE_H
