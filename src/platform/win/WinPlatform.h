#pragma once

// Windows implementations of the stage 1 interfaces (pure Qt: registry via
// QSettings, shortcuts via QFile::link).

#include "platform/PlatformInterfaces.h"

namespace ergo {

// Copies the deployed folder (ErgoPro2.exe + Qt DLLs from windeployqt) to
// %LOCALAPPDATA%\Programs\ErgoPro2 and creates a Start Menu shortcut.
// An installation made by the ErgoPro2 setup program (packaging/windows/
// ErgoPro2.iss, 1.7.4) is recognized through its uninstall registry entry: its
// folder counts as the installed copy and Uninstall runs the setup's uninstaller.
class WinInstaller final : public IInstaller {
public:
    InstallState state() const override;
    PlatformResult install() override;
    PlatformResult uninstall(bool *deferred) override;
    PlatformResult launchInstalledCopy() override;

    static QString installFolder();
    static QString startMenuShortcut();
    // Registry key the setup program writes (HKCU, per-user install). Must match
    // AppId in packaging/windows/ErgoPro2.iss.
    static QString setupRegistryKey();
};

// HKCU\Software\Microsoft\Windows\CurrentVersion\Run value "ErgoPro2".
class WinAutostart final : public IAutostart {
public:
    bool isEnabled() const override;
    QString location() const override;
    PlatformResult enable(const QString &executablePath, const QStringList &arguments) override;
    PlatformResult disable() override;
};

// Dock/activation are no-ops on Windows; the break window uses HWND_TOPMOST and
// full-screen apps are detected with SHQueryUserNotificationState.
class WinDesktopIntegration final : public IDesktopIntegration {
public:
    void setDockIconVisible(bool) override {}
    void activateApplication() override {}
    bool isSessionEnding() const override { return false; }
    bool prefersMonochromeTrayIcon() const override { return false; }
    PlatformResult launchApplication(const QString &executablePath, const QStringList &arguments) override;
    void raiseOverlayWindow(quintptr nativeWindow) override;
    bool overlayUsesFullScreenState() const override { return true; }
    std::optional<bool> isFullscreenAppActive() const override;
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
