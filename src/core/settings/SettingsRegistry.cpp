#include "core/settings/SettingsRegistry.h"

#include "core/settings/GeneralSettings.h"
#include "core/settings/SettingsSchema.h"

namespace ergo {

void registerAllSettings(SettingsSchema &schema)
{
    GeneralSettings::registerSettings(schema);
    // Later stages register their sections here:
    //   AutoClickSettings::registerSettings(schema);   // stage 3
    //   KeyControlSettings::registerSettings(schema);  // stage 4
    //   BreakTimerSettings::registerSettings(schema);  // stage 5
    //   DragLockSettings::registerSettings(schema);    // stage 6
}

} // namespace ergo
