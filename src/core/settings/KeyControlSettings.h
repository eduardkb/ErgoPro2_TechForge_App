#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "keycontrol" section (implementation_stages.md B.2 / B.5.2, stage 4).
namespace KeyControlSettings {

// JSON array of { "id", "keyCombo", "action", "enabled" } objects.
inline const QString Actions = QStringLiteral("keycontrol.actions");

// Drag Lock (stage 6, B.5.3): seconds without mouse movement before the held
// left button is released. Stored in its own "draglock" section (B.2).
inline const QString DragLockTimeoutS = QStringLiteral("draglock.DRAGLOCK_TIMEOUT");
inline constexpr int kDragLockTimeoutMin = 1, kDragLockTimeoutMax = 60, kDragLockTimeoutDefault = 10;

void registerSettings(SettingsSchema &schema);

} // namespace KeyControlSettings
} // namespace ergo
