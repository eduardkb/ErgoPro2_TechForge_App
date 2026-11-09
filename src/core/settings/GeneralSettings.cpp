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
    // Stage 1: what to do with the main window at startup (W-02).
    schema.add(SettingDefinition::enumeration(StartupAutoMode, StartupTray, {StartupTray, StartupWindow}));
    schema.add(SettingDefinition::enumeration(StartupManualMode, StartupWindow, {StartupWindow, StartupTray}));
}

} // namespace ergo::GeneralSettings
