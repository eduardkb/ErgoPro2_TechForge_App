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
    schema.add(SettingDefinition::boolean(OlderCopyKept, false)); // 2.0.0, macOS
    schema.add(SettingDefinition::integer(InstallConfirmedVersion, 0, 0, 999999)); // 2.2.1, macOS
    // 2.2.0: log problems card (system-only; out of range -> nearest limit).
    schema.add(SettingDefinition::clampedInteger(LogViewerDays, 7, kLogViewerDaysMin, kLogViewerDaysMax));
    schema.add(SettingDefinition::clampedInteger(LogViewerRefreshMinutes, 5, kLogViewerRefreshMin,
                                                 kLogViewerRefreshMax));
    // Stage 1: what to do with the main window at startup (W-02).
    schema.add(SettingDefinition::enumeration(StartupAutoMode, StartupTray, {StartupTray, StartupWindow}));
    schema.add(SettingDefinition::enumeration(StartupManualMode, StartupWindow, {StartupWindow, StartupTray}));
}

} // namespace ergo::GeneralSettings
