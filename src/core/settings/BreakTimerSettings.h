#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "breaktimer" section (implementation_stages.md B.2 / B.7, stage 5).
// camelCase key names with the unit at the end (1.8.2; were BREAKTIMER_* – old
// files are converted by SettingsMigration).
namespace BreakTimerSettings {

inline const QString Enabled = QStringLiteral("breaktimer.enabled");
inline const QString IdleResetMin = QStringLiteral("breaktimer.idleResetMinutes");
inline const QString MaxWorkMin = QStringLiteral("breaktimer.maxWorkMinutes");
inline const QString BreakMin = QStringLiteral("breaktimer.breakMinutes");
inline const QString PostponeMin = QStringLiteral("breaktimer.postponeMinutes");
inline const QString SuppressWhenFullscreen = QStringLiteral("breaktimer.suppressWhenFullscreen");
// System-only (settings.json, not on the tab; 1.6.1): the work time stops this
// many seconds after the last mouse/keyboard input and resumes with the next one.
inline const QString ActivityPauseS = QStringLiteral("breaktimer.activityPauseSeconds");

inline constexpr int kIdleResetMin = 2, kIdleResetMax = 15, kIdleResetDefault = 5;  // 2-15 since 1.7.7
inline constexpr int kMaxWorkMin = 20, kMaxWorkMax = 120, kMaxWorkDefault = 50; // 20-120 since 1.7.7
inline constexpr int kBreakMin = 3, kBreakMax = 15, kBreakDefault = 4; // 4 since 1.6.1, 3-15 since 1.7.6
inline constexpr int kPostponeMin = 3, kPostponeMax = 20, kPostponeDefault = 10; // 3-20 since 1.7.7
inline constexpr int kActivityPauseMin = 5, kActivityPauseMax = 60, kActivityPauseDefault = 20; // seconds

void registerSettings(SettingsSchema &schema);

} // namespace BreakTimerSettings
} // namespace ergo
