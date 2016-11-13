#ifndef ZEAL_REGISTRY_ITEMDATAROLE_H
#define ZEAL_REGISTRY_ITEMDATAROLE_H

#include <Qt>

namespace Zeal {
namespace Registry {

enum ItemDataRole {
    DocsetIconRole = Qt::UserRole,
    DocsetNameRole,
    UpdateAvailableRole,
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_ITEMDATAROLE_H
