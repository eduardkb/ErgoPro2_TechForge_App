#pragma once

namespace ergo {

class SettingsSchema;

// Registers every setting of the application. Each stage adds exactly one
// call here (e.g. AutoClickSettings::registerSettings in stage 3).
void registerAllSettings(SettingsSchema &schema);

} // namespace ergo
