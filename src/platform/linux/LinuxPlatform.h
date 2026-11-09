#pragma once

// Linux implementations of the stage 1 interfaces (pure Qt, no X11/evdev yet).

#include "platform/PlatformInterfaces.h"

namespace ergo {

// Copies the executable (or the AppImage) to ~/.local/opt/ErgoPro2 and creates
// a menu entry in ~/.local/share/applications.
class LinuxInstaller final : public IInstaller {
public:
    InstallState state() const override;
    PlatformResult install() override;
    PlatformResult uninstall(bool *deferred) override;
    PlatformResult launchInstalledCopy() override;

    static QString installFolder();
    static QString menuEntryPath();
};

// XDG autostart entry: ~/.config/autostart/ergopro2.desktop
class LinuxAutostart final : public IAutostart {
public:
    bool isEnabled() const override;
    QString location() const override;
    PlatformResult enable(const QString &executablePath, const QStringList &arguments) override;
    PlatformResult disable() override;
};

// X11: full support. Wayland: needs read access to /dev/input/event* and write
// access to /dev/uinput (input group + udev rule, fixed through pkexec).
class LinuxPermissions final : public IPermissions {
public:
    QList<PermissionState> check() const override;
    QString sessionDescription() const override;
    bool promptAtStartup() const override { return false; }
    PlatformResult openSettingsFor(const QString &permissionId) override;
    void runFix(const QString &permissionId, std::function<void(PlatformResult)> done) override;

    enum class Session { X11, Wayland, Unknown };
    static Session detectSession();
};

// Builds the text of a .desktop file (exposed for both installer and autostart).
QString desktopEntry(const QString &executablePath, const QStringList &arguments, const QString &iconPath,
                     bool autostart);

} // namespace ergo
