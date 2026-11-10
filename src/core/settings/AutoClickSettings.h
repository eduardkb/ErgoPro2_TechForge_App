#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "autoclick" section (implementation_stages.md B.2, stage 3).
// The owner's parameter names (AUTOCLICK_TRIGGER_TIMER / _DISTANCE) are kept
// as the JSON key names.
namespace AutoClickSettings {

inline const QString Enabled = QStringLiteral("autoclick.enabled");
inline const QString TriggerTimerMs = QStringLiteral("autoclick.AUTOCLICK_TRIGGER_TIMER");
inline const QString TriggerDistancePx = QStringLiteral("autoclick.AUTOCLICK_TRIGGER_DISTANCE");
inline const QString JitterTolerancePx = QStringLiteral("autoclick.jitterTolerancePx");
inline const QString ShowVisualFeedback = QStringLiteral("autoclick.showVisualFeedback");

inline constexpr int kTriggerTimerMin = 100;
inline constexpr int kTriggerTimerMax = 3000;
inline constexpr int kTriggerTimerDefault = 200;
inline constexpr int kDistanceMin = 5;
inline constexpr int kDistanceMax = 200;
inline constexpr int kDistanceDefault = 15;
inline constexpr int kJitterMin = 1;
inline constexpr int kJitterMax = 10;
inline constexpr int kJitterDefault = 3;
// Fixed (not a setting, A-05): no auto-click within this time after a key press.
inline constexpr int kTypingSuspendMs = 500;

void registerSettings(SettingsSchema &schema);

} // namespace AutoClickSettings
} // namespace ergo
