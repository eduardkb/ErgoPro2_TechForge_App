#pragma once

#include <QString>

namespace ergo {

enum class StartupWindowMode { ShowWindow, HideInTray };

// Decides whether the main window is shown at startup (W-02, B.1.3).
//
// autostart          - the app was started by the OS login item (--autostart)
// autoModeSetting    - value of general.startupAutoMode   ("tray" / "window")
// manualModeSetting  - value of general.startupManualMode ("window" / "tray")
// trayAvailable      - without a system tray the window is always shown,
//                      otherwise the user could not reach the application.
StartupWindowMode decideStartupWindowMode(bool autostart, const QString &autoModeSetting,
                                          const QString &manualModeSetting, bool trayAvailable);

} // namespace ergo
