#include "core/settings/AutoClickSettings.h"

#include "core/settings/SettingsSchema.h"

namespace ergo::AutoClickSettings {

void registerSettings(SettingsSchema &schema)
{
    schema.add(SettingDefinition::boolean(Enabled, false));
    schema.add(SettingDefinition::integer(TriggerTimerMs, kTriggerTimerDefault, kTriggerTimerMin, kTriggerTimerMax));
    schema.add(SettingDefinition::integer(TriggerDistancePx, kDistanceDefault, kDistanceMin, kDistanceMax));
    schema.add(SettingDefinition::integer(JitterTolerancePx, kJitterDefault, kJitterMin, kJitterMax));
    schema.add(SettingDefinition::boolean(ShowVisualFeedback, false));
}

} // namespace ergo::AutoClickSettings
