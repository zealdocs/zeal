// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_REGISTRY_ITEMDATAROLE_H
#define ZEAL_REGISTRY_ITEMDATAROLE_H

#include <Qt>

namespace Zeal::Registry {

enum ItemDataRole {
    DocsetIconRole = Qt::UserRole,
    DocsetNameRole,
    MatchPositionsRole,
    UpdateAvailableRole,
    UrlRole
};

enum SectionIndex {
    Name,
    SearchPrefix,
    Actions
};

} // namespace Zeal::Registry

#endif // ZEAL_REGISTRY_ITEMDATAROLE_H
