#pragma once

// Abstract platform interfaces (implementation_stages.md A.3 / B.6).
//
// Every OS-specific behavior lives behind one of these interfaces. Concrete
// implementations are in src/platform/<os>/ and are created by
// PlatformFactory. Interfaces are extended by the stage that implements them:
//   IAutostart, IInstaller, IPermissions, IDesktopIntegration -> stage 1
//   IInputHook, IClickInjector, IPowerEvents                  -> stage 2

#include "core/FeatureAvailability.h"
#include "platform/PlatformResult.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

namespace ergo {

// Global mouse/keyboard monitoring on a dedicated thread (stage 2).
class IInputHook {
public:
    virtual ~IInputHook() = default;
    virtual PlatformResult start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
};

enum class MouseButton { Left, Right, Middle };

// Synthetic mouse clicks at the current pointer position (stage 2).
class IClickInjector {
public:
    virtual ~IClickInjector() = default;
    virtual PlatformResult click(MouseButton button) = 0;
    virtual PlatformResult doubleClick(MouseButton button) = 0;
    virtual PlatformResult press(MouseButton button) = 0;
    virtual PlatformResult release(MouseButton button) = 0;
};

// Start-at-login registration for the current user (no admin rights).
class IAutostart {
public:
    virtual ~IAutostart() = default;
    virtual bool isEnabled() const = 0;
    // Human-readable description of where the registration lives
    // (e.g. "~/Library/LaunchAgents/com.techforge.ergopro2.plist").
    virtual QString location() const = 0;
    virtual PlatformResult enable(const QString &executablePath, const QStringList &arguments) = 0;
    virtual PlatformResult disable() = 0;
};

struct InstallState {
    bool installed = false;               // an installed copy exists at installLocation
    QString installLocation;              // .app bundle (macOS) or folder (Windows/Linux)
    QString installedExecutable;          // executable inside the installed copy
    QString currentLocation;              // location of the running copy (bundle/folder/file)
    bool runningFromInstalledCopy = false;
};

// Copies the application to a permanent per-user folder and removes it again,
// including menu entries / shortcuts (B.1.3). Start-at-login is handled by
// IAutostart.
class IInstaller {
public:
    virtual ~IInstaller() = default;
    virtual InstallState state() const = 0;

    // Copies the running application to installLocation (replacing an older
    // installed copy) and creates the menu entry / shortcut.
    virtual PlatformResult install() = 0;

    // Removes the menu entry / shortcut and the installed copy. When the running
    // process IS the installed copy, deletion is scheduled for after exit and
    // *deferred is set to true (the caller must then exit the application).
    virtual PlatformResult uninstall(bool *deferred) = 0;

    // Starts the installed copy as a new process.
    virtual PlatformResult launchInstalledCopy() = 0;
};

// OS permissions / requirements needed by the input features (stage 1).
class IPermissions {
public:
    virtual ~IPermissions() = default;

    virtual QList<PermissionState> check() const = 0;

    // Desktop session description for diagnostics (e.g. "X11", "Wayland").
    virtual QString sessionDescription() const = 0;

    // macOS: missing permissions are presented in a window at startup.
    virtual bool promptAtStartup() const = 0;

    // FixKind::OpenSettings - asks the OS to list ErgoPro2 for the permission
    // (where supported) and opens the matching settings page.
    virtual PlatformResult openSettingsFor(const QString &permissionId) = 0;

    // FixKind::RunFix - runs the automatic fix asynchronously (may show an OS
    // administrator password dialog; the app never handles passwords).
    // 'done' is called on the calling thread when the fix has finished.
    virtual void runFix(const QString &permissionId, std::function<void(PlatformResult)> done) = 0;
};

// Sleep / resume / lock / unlock notifications (stage 2).
class IPowerEvents {
public:
    virtual ~IPowerEvents() = default;
    virtual PlatformResult start() = 0;
    virtual void stop() = 0;
};

// Desktop shell integration that Qt does not cover.
class IDesktopIntegration {
public:
    virtual ~IDesktopIntegration() = default;

    // macOS: Dock icon visible only while the main window is visible
    // (activation policy Regular <-> Accessory). No-op elsewhere.
    virtual void setDockIconVisible(bool visible) = 0;

    // Brings the application to the foreground after showing a window.
    virtual void activateApplication() = 0;

    // True while the OS is ending the user session (logout, restart, shutdown)
    // and asks the app to quit. The app normally refuses to quit except via
    // its Exit button, but must never block a logout/shutdown.
    virtual bool isSessionEnding() const = 0;

    // macOS menu bar icons should be monochrome "template" images that adapt
    // to light/dark menu bars. Other OSes use the colored tray icon.
    virtual bool prefersMonochromeTrayIcon() const = 0;

    // Starts a new process of the given executable (used to restart the app).
    // macOS starts the enclosing .app bundle through LaunchServices.
    virtual PlatformResult launchApplication(const QString &executablePath, const QStringList &arguments) = 0;
};

} // namespace ergo
