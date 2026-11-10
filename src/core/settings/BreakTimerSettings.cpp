#include "core/settings/BreakTimerSettings.h"

#include "core/settings/SettingsSchema.h"

namespace ergo::BreakTimerSettings {

void registerSettings(SettingsSchema &schema)
{
    schema.add(SettingDefinition::boolean(Enabled, true));
    schema.add(SettingDefinition::integer(IdleResetMin, kIdleResetDefault, kIdleResetMin, kIdleResetMax));
    schema.add(SettingDefinition::integer(MaxWorkMin, kMaxWorkDefault, kMaxWorkMin, kMaxWorkMax));
    schema.add(SettingDefinition::integer(BreakMin, kBreakDefault, kBreakMin, kBreakMax));
    schema.add(SettingDefinition::integer(PostponeMin, kPostponeDefault, kPostponeMin, kPostponeMax));
    schema.add(SettingDefinition::boolean(SuppressWhenFullscreen, false));
}

} // namespace ergo::BreakTimerSettings
