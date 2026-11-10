#include "core/settings/KeyControlSettings.h"

#include "core/settings/SettingsSchema.h"

namespace ergo::KeyControlSettings {

void registerSettings(SettingsSchema &schema)
{
    // Entries are validated by KeyBindingStore: invalid ones are kept and shown
    // with an Error status instead of being dropped.
    schema.add(SettingDefinition::list(Actions));
    schema.add(SettingDefinition::integer(DragLockTimeoutS, kDragLockTimeoutDefault, kDragLockTimeoutMin,
                                          kDragLockTimeoutMax));
}

} // namespace ergo::KeyControlSettings
