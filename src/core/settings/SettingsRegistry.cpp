#include "core/settings/SettingsRegistry.h"

#include "core/settings/AutoClickSettings.h"
#include "core/settings/BreakTimerSettings.h"
#include "core/settings/GeneralSettings.h"
#include "core/settings/KeyControlSettings.h"
#include "core/settings/SettingsSchema.h"

namespace ergo {

void registerAllSettings(SettingsSchema &schema)
{
    GeneralSettings::registerSettings(schema);
    AutoClickSettings::registerSettings(schema);        // stage 3
    KeyControlSettings::registerSettings(schema);       // stage 4
    BreakTimerSettings::registerSettings(schema);       // stage 5
    // Later stages register their sections here:
    //   DragLockSettings::registerSettings(schema);    // stage 6
}

} // namespace ergo
