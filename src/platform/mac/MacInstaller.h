#pragma once

#include "platform/PlatformInterfaces.h"

namespace ergo {

// macOS: copies the running ErgoPro2.app bundle to ~/Applications/ErgoPro2.app
// (no admin rights needed). ditto preserves symlinks, permissions and the
// code signature of the bundle.
class MacInstaller final : public IInstaller {
public:
    InstallState state() const override;
    PlatformResult install() override;
    PlatformResult uninstall(bool *deferred) override;
    PlatformResult launchInstalledCopy() override;

    static QString installedBundlePath();
    static QString runningBundlePath(); // empty if not running from a .app bundle
};

} // namespace ergo
