#include "core/StartupMode.h"

#include "core/settings/GeneralSettings.h"

namespace ergo {

StartupWindowMode decideStartupWindowMode(bool autostart, const QString &autoModeSetting,
                                          const QString &manualModeSetting, bool trayAvailable)
{
    if (!trayAvailable)
        return StartupWindowMode::ShowWindow;
    const QString &mode = autostart ? autoModeSetting : manualModeSetting;
    return mode == GeneralSettings::StartupTray ? StartupWindowMode::HideInTray : StartupWindowMode::ShowWindow;
}

} // namespace ergo
