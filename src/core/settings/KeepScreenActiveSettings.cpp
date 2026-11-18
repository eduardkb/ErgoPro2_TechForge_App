#include "core/settings/KeepScreenActiveSettings.h"

#include "core/settings/SettingsSchema.h"

namespace ergo::KeepScreenActiveSettings {

void registerSettings(SettingsSchema &schema)
{
    schema.add(SettingDefinition::boolean(Enabled, false));
    schema.add(SettingDefinition::integer(StartAfterIdleMinutes, kStartAfterDefault, kStartAfterMin, kStartAfterMax));
    schema.add(SettingDefinition::integer(IntervalMinutes, kIntervalDefault, kIntervalMin, kIntervalMax));
}

} // namespace ergo::KeepScreenActiveSettings
