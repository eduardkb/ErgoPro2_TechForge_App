#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "keycontrol" section (implementation_stages.md B.2 / B.5.2, stage 4).
namespace KeyControlSettings {

// JSON array of { "id", "keyCombo", "action", "enabled" } objects.
inline const QString Actions = QStringLiteral("keycontrol.actions");

void registerSettings(SettingsSchema &schema);

} // namespace KeyControlSettings
} // namespace ergo
