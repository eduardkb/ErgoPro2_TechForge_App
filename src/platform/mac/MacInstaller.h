#pragma once

#include "platform/PlatformInterfaces.h"

namespace ergo {

// macOS (2.0.0): ErgoPro2 is installed when ErgoPro2.app is in /Applications
// (dragged there from the .dmg window, or copied by Install) or in
// ~/Applications (Install on an account that cannot write /Applications, and
// all 1.x versions). Install copies the running bundle with ditto, which keeps
// symlinks, permissions and the code signature. The rules which copy counts
// are in core/InstallPolicy.
class MacInstaller final : public IInstaller {
public:
    InstallState state() const override;
    PlatformResult install() override;
    PlatformResult uninstall(bool *deferred) override;
    PlatformResult launchInstalledCopy() override;
    PlatformResult removeOlderCopy() override;
    QList<InstallerImage> installerImages() const override;
    PlatformResult removeInstallerImage(const InstallerImage &image) override;

    static QString systemBundlePath(); // /Applications/ErgoPro2.app
    static QString userBundlePath();   // ~/Applications/ErgoPro2.app
    static QString runningBundlePath(); // empty if not running from a .app bundle
};

} // namespace ergo
