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
