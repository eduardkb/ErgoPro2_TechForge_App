#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "breaktimer" section (implementation_stages.md B.2 / B.7, stage 5).
// The owner's parameter names are kept as JSON key names. Durations in minutes.
namespace BreakTimerSettings {

inline const QString Enabled = QStringLiteral("breaktimer.enabled");
inline const QString IdleResetMin = QStringLiteral("breaktimer.BREAKTIMER_IDLE_RESET");
inline const QString MaxWorkMin = QStringLiteral("breaktimer.BREAKTIMER_MAX_WORK_TIME");
inline const QString BreakMin = QStringLiteral("breaktimer.BREAKTIMER_BREAK_TIME");
inline const QString PostponeMin = QStringLiteral("breaktimer.BREAKTIMER_POSTPONE_TIME");
inline const QString SuppressWhenFullscreen = QStringLiteral("breaktimer.suppressWhenFullscreen");

inline constexpr int kIdleResetMin = 1, kIdleResetMax = 30, kIdleResetDefault = 5;
inline constexpr int kMaxWorkMin = 5, kMaxWorkMax = 180, kMaxWorkDefault = 50;
inline constexpr int kBreakMin = 1, kBreakMax = 30, kBreakDefault = 5;
inline constexpr int kPostponeMin = 1, kPostponeMax = 60, kPostponeDefault = 10;

void registerSettings(SettingsSchema &schema);

} // namespace BreakTimerSettings
} // namespace ergo
