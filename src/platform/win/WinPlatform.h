#pragma once

// Windows implementations of the stage 1 interfaces (pure Qt: registry via
// QSettings, shortcuts via QFile::link).

#include "platform/PlatformInterfaces.h"

namespace ergo {

// Copies the deployed folder (ErgoPro2.exe + Qt DLLs from windeployqt) to
// %LOCALAPPDATA%\Programs\ErgoPro2 and creates a Start Menu shortcut.
class WinInstaller final : public IInstaller {
public:
    InstallState state() const override;
    PlatformResult install() override;
    PlatformResult uninstall(bool *deferred) override;
    PlatformResult launchInstalledCopy() override;

    static QString installFolder();
    static QString startMenuShortcut();
};

// HKCU\Software\Microsoft\Windows\CurrentVersion\Run value "ErgoPro2".
class WinAutostart final : public IAutostart {
public:
    bool isEnabled() const override;
    QString location() const override;
    PlatformResult enable(const QString &executablePath, const QStringList &arguments) override;
    PlatformResult disable() override;
};

// Windows needs no special permission for global hooks.
class WinPermissions final : public IPermissions {
public:
    QList<PermissionState> check() const override;
    QString sessionDescription() const override;
    bool promptAtStartup() const override { return false; }
    PlatformResult openSettingsFor(const QString &permissionId) override;
    void runFix(const QString &permissionId, std::function<void(PlatformResult)> done) override;
};

} // namespace ergo
