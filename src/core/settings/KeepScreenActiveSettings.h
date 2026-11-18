#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "keepScreenActive" section (B.10 Keep Screen Active, stage 9).
// Only "enabled" is changed by the app (hotkey); the two minute values are
// system-only (settings.json, never shown in the interface).
namespace KeepScreenActiveSettings {

inline const QString Enabled = QStringLiteral("keepScreenActive.enabled");
inline const QString StartAfterIdleMinutes = QStringLiteral("keepScreenActive.startAfterIdleMinutes");
inline const QString IntervalMinutes = QStringLiteral("keepScreenActive.intervalMinutes");

inline constexpr int kStartAfterMin = 1, kStartAfterMax = 60, kStartAfterDefault = 3;
inline constexpr int kIntervalMin = 1, kIntervalMax = 60, kIntervalDefault = 3;

void registerSettings(SettingsSchema &schema);

} // namespace KeepScreenActiveSettings
} // namespace ergo
