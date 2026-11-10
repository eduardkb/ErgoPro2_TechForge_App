#include "core/settings/StatisticsSettings.h"

#include "core/settings/SettingsSchema.h"

namespace ergo::StatisticsSettings {

void registerSettings(SettingsSchema &schema)
{
    schema.add(SettingDefinition::integer(RetentionDays, kRetentionDefault, kRetentionMin, kRetentionMax));
}

} // namespace ergo::StatisticsSettings
