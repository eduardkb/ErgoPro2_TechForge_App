#pragma once

// Abstract platform interfaces (implementation_stages.md A.3 / B.6).
//
// Every OS-specific behavior lives behind one of these interfaces. Concrete
// implementations are in src/platform/<os>/ and are created by
// PlatformFactory. The interfaces are intentionally minimal in stage 0 and
// are extended by the stage that implements them:
//   IAutostart, IInstaller, IPermissions  -> stage 1
//   IInputHook, IClickInjector, IPowerEvents -> stage 2

#include "platform/PlatformResult.h"

#include <QList>
#include <QString>
#include <QStringList>

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

// Start-at-login registration (stage 1).
class IAutostart {
public:
    virtual ~IAutostart() = default;
    virtual bool isEnabled() const = 0;
    virtual PlatformResult enable(const QString &executablePath, const QStringList &arguments) = 0;
    virtual PlatformResult disable() = 0;
};

// Copy to a permanent per-user location / removal (stage 1).
class IInstaller {
public:
    virtual ~IInstaller() = default;
    virtual bool isInstalled() const = 0;
    virtual QString installLocation() const = 0;
    virtual PlatformResult install() = 0;
    virtual PlatformResult uninstall() = 0;
};

struct PermissionStatus {
    QString id;          // e.g. "accessibility"
    QString displayName; // e.g. "Accessibility"
    bool granted = false;
    QString explanation;
};

// OS permissions needed by the input features (stage 1).
class IPermissions {
public:
    virtual ~IPermissions() = default;
    virtual QList<PermissionStatus> check() const = 0;
    virtual PlatformResult openSettingsFor(const QString &permissionId) = 0;
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
};

} // namespace ergo
