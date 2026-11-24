#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "keepScreenActive" section (B.10 Keep Screen Active, stage 9).
// Only "enabled" is changed by the app (hotkey); the two minute values and
// maxActiveHours (2.4.0) are system-only (settings.json, never shown in the
// interface).
namespace KeepScreenActiveSettings {

inline const QString Enabled = QStringLiteral("keepScreenActive.enabled");
inline const QString StartAfterIdleMinutes = QStringLiteral("keepScreenActive.startAfterIdleMinutes");
inline const QString IntervalMinutes = QStringLiteral("keepScreenActive.intervalMinutes");
// 2.4.0 (owner request): hours without real input after which the feature is
// switched off so the screen can sleep; 25 = never. Out of range: nearest limit.
inline const QString MaxActiveHours = QStringLiteral("keepScreenActive.maxActiveHours");

inline constexpr int kStartAfterMin = 1, kStartAfterMax = 60, kStartAfterDefault = 3;
inline constexpr int kIntervalMin = 1, kIntervalMax = 60, kIntervalDefault = 3;
inline constexpr int kMaxActiveHoursMin = 4, kMaxActiveHoursMax = 25, kMaxActiveHoursDefault = 4;

void registerSettings(SettingsSchema &schema);

} // namespace KeepScreenActiveSettings
} // namespace ergo
