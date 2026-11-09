#include "core/settings/GeneralSettings.h"

#include "core/LogLevel.h"
#include "core/Logger.h"
#include "core/settings/SettingsSchema.h"

namespace ergo::GeneralSettings {

void registerSettings(SettingsSchema &schema)
{
    schema.add(SettingDefinition::enumeration(LogLevel, logLevelSelectionId(kDefaultLogLevelSelection),
                                              allLogLevelSelectionIds()));
    schema.add(SettingDefinition::integer(LogRetentionDays, 30, Logger::kMinRetentionDays,
                                          Logger::kMaxRetentionDays));
    schema.add(SettingDefinition::boolean(TrayHintShown, false));
}

} // namespace ergo::GeneralSettings
