// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_REGISTRY_ITEMDATAROLE_H
#define ZEAL_REGISTRY_ITEMDATAROLE_H

#include <Qt>

namespace Zeal::Registry {

// These enums intentionally stay unscoped: their values feed Qt's int-based
// model/view API — data() roles (extending Qt::UserRole) and column/section
// indices — where an enum class would force casts at every use.

// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
enum ItemDataRole {
    DocsetIconRole = Qt::UserRole,
    DocsetNameRole,
    MatchPositionsRole,
    UpdateAvailableRole,
    UrlRole
};

// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
enum SectionIndex {
    Name,
    SearchPrefix,
    Actions
};

} // namespace Zeal::Registry

#endif // ZEAL_REGISTRY_ITEMDATAROLE_H
