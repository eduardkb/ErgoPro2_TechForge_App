#pragma once

#include "platform/PlatformInterfaces.h"

#include <memory>

namespace ergo {

// All platform services of the current OS, created once at startup.
struct Platform {
    QString osName; // "macOS", "Windows", "Linux"
    std::unique_ptr<IInputHook> inputHook;
    std::unique_ptr<IClickInjector> clickInjector;
    std::unique_ptr<IAutostart> autostart;
    std::unique_ptr<IInstaller> installer;
    std::unique_ptr<IPermissions> permissions;
    std::unique_ptr<IPowerEvents> powerEvents;
    std::unique_ptr<IDesktopIntegration> desktop;
};

// Creates the implementations for the OS this binary was compiled for.
// This (and src/platform/<os>/) is the only place where OS #ifdefs are allowed.
Platform createPlatform();

} // namespace ergo
