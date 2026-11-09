#pragma once

#include "platform/PlatformInterfaces.h"

namespace ergo {

// macOS: per-user LaunchAgent (~/Library/LaunchAgents/com.techforge.ergopro2.plist)
// that starts ErgoPro2 at login with --autostart. No admin rights needed.
//
// SMAppService (macOS 13+) is not used: it can only register the *running*
// bundle and cannot pass arguments, while ErgoPro2 must register the installed
// copy (~/Applications) with --autostart. macOS lists the agent under
// System Settings > General > Login Items & Extensions > "Allow in the Background".
class MacAutostart final : public IAutostart {
public:
    bool isEnabled() const override;
    QString location() const override;
    PlatformResult enable(const QString &executablePath, const QStringList &arguments) override;
    PlatformResult disable() override;

    static QString plistPath();
};

} // namespace ergo
