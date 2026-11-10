#pragma once

// Abstract platform interfaces (implementation_stages.md A.3 / B.6).
//
// Every OS-specific behavior lives behind one of these interfaces. Concrete
// implementations are in src/platform/<os>/ and are created by
// PlatformFactory. Interfaces are extended by the stage that implements them:
//   IAutostart, IInstaller, IPermissions, IDesktopIntegration -> stage 1
//   IInputHook, IClickInjector, IPowerEvents                  -> stage 2

#include "core/FeatureAvailability.h"
#include "core/input/InputEvent.h"
#include "core/input/KeyCombo.h"
#include "platform/PlatformResult.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>
#include <optional>

namespace ergo {

// Called ON THE HOOK THREAD for every input event. Must return quickly
// (e.g. InputDispatcher::post): no I/O, no long locks.
using InputEventSink = std::function<void(const InputEvent &)>;

// Global mouse/keyboard monitoring on ONE dedicated thread with its own OS
// event loop (B.6). Key events for which suppression->shouldSwallow() returns
// true are blocked from reaching other applications (they are still delivered
// to the sink with swallowed = true). The same applies to mouse button events
// and suppression->shouldSwallowMouse() where canSuppressMouseClicks() is true.
// While suppression->isDragLockHolding(), real pointer moves must reach
// applications as left-button drags (macOS converts them in the tap).
class IInputHook {
public:
    virtual ~IInputHook() = default;

    virtual PlatformResult start(InputEventSink sink, KeySuppressionRegistry *suppression) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    // False when the hook runs but cannot block keys (e.g. macOS without
    // Accessibility: listen-only event tap).
    virtual bool canSuppressKeys() const = 0;
    // True if the hook can swallow the Drag Lock releasing click
    // (KeySuppressionRegistry::shouldSwallowMouse, B.5.3 / D-03). X11 (XRecord)
    // and evdev only observe the mouse: there the click also reaches the
    // application (a left click merges with the held button).
    virtual bool canSuppressMouseClicks() const = 0;
    virtual QString backendName() const = 0;

    // Health probe for the watchdog (B.5.4). Repairs what can be repaired in
    // place (e.g. re-enables a disabled macOS event tap, counted by
    // selfRepairCount()). Returns false if the hook must be reinstalled.
    virtual bool checkHealth() = 0;
    virtual int selfRepairCount() const = 0;

    // OS-level registration of a key combo that must be exclusive. X11 grabs
    // the key (XGrabKey); other OSes block it inside the hook and only validate.
    // Registrations survive stop()/start() (they are re-applied).
    virtual PlatformResult registerKey(const KeyCombo &combo) = 0;
    virtual PlatformResult unregisterKey(const KeyCombo &combo) = 0;
};

enum class MouseButton { Left, Right, Middle };

// Synthetic mouse clicks at the current pointer position (B.6). Every injected
// event is tagged so the hook reports it with synthetic = true.
class IClickInjector {
public:
    virtual ~IClickInjector() = default;
    virtual PlatformResult click(MouseButton button) = 0;
    virtual PlatformResult doubleClick(MouseButton button) = 0;
    virtual PlatformResult press(MouseButton button) = 0;
    virtual PlatformResult release(MouseButton button) = 0;
    virtual int doubleClickIntervalMs() const = 0; // OS setting (default 400 ms)
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

enum class PowerEvent { Sleep, Resume, ScreenLocked, ScreenUnlocked };

// Sleep / resume / lock / unlock notifications (B.6). The callback may be
// called on any thread.
class IPowerEvents {
public:
    virtual ~IPowerEvents() = default;
    virtual PlatformResult start(std::function<void(PowerEvent)> callback) = 0;
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

    // Break window (B.7): keep the native window (QWidget::winId()) above all
    // other windows, including full-screen applications, on every Space/desktop.
    virtual void raiseOverlayWindow(quintptr nativeWindow) = 0;
    // True if the overlay must use the window system's full-screen state
    // (X11/Windows); macOS uses a normal window at screen-saver level instead,
    // because full-screen windows open a separate Space.
    virtual bool overlayUsesFullScreenState() const = 0;

    // Best-effort detection of a full-screen application in the foreground
    // (B.17); std::nullopt when this OS is not supported.
    virtual std::optional<bool> isFullscreenAppActive() const = 0;
};

} // namespace ergo
