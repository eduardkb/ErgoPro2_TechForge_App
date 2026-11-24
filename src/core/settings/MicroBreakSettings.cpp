#include "core/settings/MicroBreakSettings.h"

#include "core/settings/SettingsSchema.h"

namespace ergo::MicroBreakSettings {

void registerSettings(SettingsSchema &schema)
{
    schema.add(SettingDefinition::boolean(Enabled, false));
    schema.add(SettingDefinition::clampedInteger(IntervalMinutes, kIntervalDefault, kIntervalMin, kIntervalMax));
    schema.add(SettingDefinition::clampedInteger(ShowSeconds, kShowDefault, kShowMin, kShowMax));
}

} // namespace ergo::MicroBreakSettings
