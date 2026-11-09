#pragma once

#include "platform/PlatformInterfaces.h"

namespace ergo {

// macOS: Accessibility (synthetic clicks, blocking hotkeys) and Input
// Monitoring (reading the keyboard/mouse globally) permissions.
class MacPermissions final : public IPermissions {
public:
    QList<PermissionState> check() const override;
    QString sessionDescription() const override;
    bool promptAtStartup() const override { return true; }
    PlatformResult openSettingsFor(const QString &permissionId) override;
    void runFix(const QString &permissionId, std::function<void(PlatformResult)> done) override;
};

} // namespace ergo
