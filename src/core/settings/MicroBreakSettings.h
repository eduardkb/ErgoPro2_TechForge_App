#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "microBreaks" section (2.5.0, owner request; spec B.7a). All
// three are shown on the Micro-breaks tab and can also be edited in
// settings.json; out-of-range numbers are set to the nearest limit.
namespace MicroBreakSettings {

inline const QString Enabled = QStringLiteral("microBreaks.enabled");
inline const QString IntervalMinutes = QStringLiteral("microBreaks.intervalMinutes");
inline const QString ShowSeconds = QStringLiteral("microBreaks.showSeconds");

inline constexpr int kIntervalMin = 1, kIntervalMax = 60, kIntervalDefault = 10; // minutes of active use
inline constexpr int kShowMin = 10, kShowMax = 120, kShowDefault = 20;           // seconds

void registerSettings(SettingsSchema &schema);

} // namespace MicroBreakSettings
} // namespace ergo
