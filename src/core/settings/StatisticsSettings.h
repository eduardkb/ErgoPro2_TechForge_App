#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "statistics" section (implementation_stages.md B.2 / B.8, stage 7).
namespace StatisticsSettings {

// Days of statistics files kept (today included); older files are deleted.
inline const QString RetentionDays = QStringLiteral("statistics.retentionDays");
inline constexpr int kRetentionMin = 1, kRetentionMax = 360, kRetentionDefault = 90;

void registerSettings(SettingsSchema &schema);

} // namespace StatisticsSettings
} // namespace ergo
