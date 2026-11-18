#pragma once

#include <QJsonObject>

namespace ergo {

// Converts an older settings.json (root object) to the current key names.
// schemaVersion 1 -> 2 (1.8.2, owner request): the upper-case keys
// (AUTOCLICK_*, DRAGLOCK_*, BREAKTIMER_*) were renamed to camelCase with the
// unit at the end, like the other keys. Each old key's value is moved to the
// new key unless the new key already exists (the new key wins); the old key is
// removed. Returns the number of old keys found.
int migrateRenamedSettingKeys(QJsonObject &root);

} // namespace ergo
