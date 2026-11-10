# Changelog

All notable changes to ErgoPro2 are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions use `MAJOR.MINOR.PATCH`:
each implementation stage bumps MINOR, each bug-fix run bumps PATCH, MAJOR stays 1
(see `documentation/implementation_stages.md`, A.4).

## [1.2.0] - 2026-11-10

Stage 2 – shared global input layer.

### Added
- Platform-independent input event model (mouse move, button down/up, wheel, key down/up with Qt key,
  native code, modifiers, auto-repeat, synthetic and blocked flags, monotonic timestamp, logical-pixel
  position and screen).
- Global input hooks on one dedicated thread per OS:
  - macOS: session-level CGEventTap (active, or listen-only without Accessibility); a tap disabled by
    macOS is re-enabled immediately.
  - Windows: WH_KEYBOARD_LL + WH_MOUSE_LL with their own message loop; hooks are re-armed by every
    health check because Windows can drop them silently.
  - Linux X11: XRecord monitoring and XGrabKey exclusive keys (all NumLock/CapsLock variants, re-grabbed
    after keyboard mapping changes).
  - Linux Wayland (best effort, needs libevdev at build time): evdev with device hotplug, relative pointer
    positions, keyboard grab + uinput forwarding to block keys.
- Key suppression registry: register/unregister key combos that must not reach other applications; the
  key-up and auto-repeats of a blocked key are blocked too.
- Click injection (left/right/middle click, double click with the OS double-click interval, press,
  release) at the pointer position; every injected event is tagged and reported back as synthetic.
- Sleep/resume and lock/unlock notifications (macOS NSWorkspace, Windows WM_POWERBROADCAST +
  WTS session notifications, Linux logind and screensaver over D-Bus).
- Health watchdog: checks the hook every 30 s, reinstalls it after failures, resume and unlock, logs every
  reinstall and reports a persistent failure through the feature-availability banners.
- Input dispatcher that delivers events from the hook thread to subscribers in their own threads.
- General tab: collapsible **Input diagnostics** card (hook status, pointer position, last key and mouse
  button, synthetic flag, events per second, watchdog counters, *Test left click in 3 s* and *Test: block
  F9 for 30 s*); the diagnostics card's *Input hook* row shows the real status.
- The input hook starts automatically as soon as the missing permissions are granted.
- Unit tests for the event helpers, key-combo matching and blocking rules, the dispatcher and the
  watchdog.

### Changed
- `SystemClock::monotonicMs()` uses a system-wide steady clock, shared with the input hooks.
- Linux builds need the X11 and XTest development files; libevdev (Wayland) and QtDBus are optional.
- CI installs `libx11-dev`, `libxtst-dev`, `libevdev-dev` and `pkg-config` for the Debian job.

## [1.1.4] - 2026-11-10

Bug fix (permissions window layout).

### Fixed
- **Permissions window:** as soon as one permission was detected as granted, the remaining permission's
  card (e.g. *Input Monitoring*) was squeezed vertically – text, the MISSING badge and the *Open System
  Settings* button were cut off and unreadable. Root cause: after a change the window measured its new
  height before Qt had shown the newly created cards, so they were left out and the window shrank too
  much; the height is now measured once the cards are shown and kept as the window's minimum height.

## [1.1.3] - 2026-11-10

Bug fix (macOS Accessibility permission).

### Fixed
- **macOS:** the Accessibility permission was never recognized as granted, even when it was switched on
  in System Settings and after restarting (Input Monitoring worked). Root cause: the build left
  `ErgoPro2.app` with only the linker's partial signature (identifier `ErgoPro2`, resources not sealed),
  which macOS treats as invalid, so the Accessibility grant could not be matched to the app. The build
  now signs the whole bundle with its bundle identifier after every build (ad-hoc by default, or the
  identity in the new CMake option `ERGOPRO2_CODESIGN_IDENTITY`).
- When Accessibility is missing and the app's own signature is invalid or has the wrong identifier,
  ErgoPro2 now logs a warning and the Accessibility hint explains how to fix it.

## [1.1.2] - 2026-11-10

Bug fix (macOS minimize).

### Fixed
- **macOS:** after minimizing the main window, restoring it (e.g. by clicking its thumbnail in the Dock)
  showed a completely blank window whose red close button did nothing; only the tray's *Show window*
  brought it back. Minimize now hides the window to the tray as specified, and if macOS restores the
  hidden window by itself it is shown normally (content drawn, close button working). Root cause: the app
  cleared the "minimized" state while macOS was still animating the minimize, so macOS kept the window
  minimized in the Dock while Qt considered it hidden; a later restore put a window on screen that Qt
  neither painted nor closed.

## [1.1.1] - 2026-11-10

Bug fixes (CI failures on Windows and macOS).

### Fixed
- **Windows:** the "running from a temporary / Downloads folder" check could treat a whole drive (e.g.
  `C:/`) as such a folder and then warn for every location; the `tst_locationclassifier` unit test failed
  on Windows. Root cause: drive roots were not recognized as file-system roots – trimming the trailing
  `/` turned `C:/` into `C:` (the current folder of drive C).
- **macOS:** building with Qt 6.8 against the macOS 26 SDK (Xcode 26) failed with
  `ld: framework 'AGL' not found`. Root cause: Qt 6.8 links the AGL framework, which Apple removed from
  the macOS 26 SDK; the build now substitutes OpenGL.framework when the SDK has no AGL.

## [1.1.0] - 2026-11-09

Stage 1 – General tab, install / start at login, permissions.

### Added
- **Install / Uninstall** (General tab): copies ErgoPro2 to a permanent per-user folder
  (macOS `~/Applications/ErgoPro2.app`, Windows `%LOCALAPPDATA%\Programs\ErgoPro2`, Linux
  `~/.local/opt/ErgoPro2`), creates the Start Menu shortcut / Linux menu entry and registers the installed
  copy to start at login with `--autostart` (macOS LaunchAgent, Windows `HKCU\…\Run`, Linux XDG
  autostart). No administrator rights. Offers to restart from the installed copy. Uninstall removes
  everything but keeps settings and logs; when run from the installed copy, the copy is deleted right
  after ErgoPro2 exits.
- Status of the installation and a warning when running from a temporary folder, the Downloads folder,
  a disk image or macOS App Translocation.
- **Startup behavior** options: *Started at login* (default: hide in tray) and *Started manually*
  (default: show window), applied at launch using `--autostart` (settings `general.startupAutoMode`,
  `general.startupManualMode`).
- **Open log folder** and **Open settings folder** buttons; **Reset all settings to defaults** (with
  confirmation).
- **Permissions & diagnostics** card: OS, desktop session, application path, each OS permission with
  status and fix button, input hook status, start-at-login registration.
- Permission detection: macOS Accessibility and Input Monitoring (with System Settings shortcuts);
  Linux X11/Wayland detection and, on Wayland, input device / `/dev/uinput` access with an optional fix
  through `pkexec` (after confirmation; the app never handles passwords); Windows reports that no
  permission is needed (with the note about programs running as Administrator).
- `FeatureAvailability` rules and service: per feature (Auto Click, Key Control, Break Timer activity)
  available / reason / fix; re-checked every 3 s while something is missing.
- "Unavailable" banner at the top of the Break Timer, Auto Click and Key Control tabs, updated live.
- macOS startup window "ErgoPro2 needs your permission" with *Open System Settings*, *Check again*,
  *Restart ErgoPro2* and *Continue without*; closes itself when everything is granted.
- Unit tests for the startup decision, the location check, the availability rules and the new settings.

### Changed
- Platform interfaces `IAutostart`, `IInstaller`, `IPermissions` and `IDesktopIntegration` extended; the
  stage 0 stubs for them are replaced by real implementations on all three OSes.
- Long file paths in the UI wrap instead of widening the window.

## [1.0.0] - 2026-11-09

Stage 0 – project skeleton.

### Added
- CMake project (C++20, Qt 6.8+ Widgets) with the version defined once in the root `CMakeLists.txt`
  and exposed through a generated header; macOS app bundle (`com.techforge.ergopro2`, Info.plist,
  `.icns`), Windows executable with icon and version resource, Linux `.desktop` file and install rules.
- Layered source layout: `src/core` (platform-independent logic), `src/platform` (OS interfaces,
  factory, stub implementations, macOS Dock integration), `src/ui` (Qt Widgets).
- Logging module: DEBUG/INFO/WARNING/ERROR/CRITICAL, cumulative user-selectable level, one file per day
  with 5 MB roll-over, retention cleanup (1–180 days), Qt message redirection, thread-safe, console
  output in debug builds, CRITICAL logging of uncaught exceptions (`std::terminate` handler and Qt event
  loop guard).
- Settings module: `settings.json` in the per-user config folder, `schemaVersion`, typed and validated
  values with defaults, corrupt-file backup with user notification, atomic debounced (300 ms) saving,
  change signals; per-stage registration of settings (stage 0: `general.*`).
- Injectable clock (`SystemClock`, `FakeClock`) for time-dependent logic and tests.
- Platform interfaces (`IInputHook`, `IClickInjector`, `IAutostart`, `IInstaller`, `IPermissions`,
  `IPowerEvents`, `IDesktopIntegration`) with logging stub implementations.
- Main window with the tabs General, Break Timer, Auto Click, Key Control and Statistics; General tab
  with version, log level, log retention and Exit button; placeholders for later features.
- Modern look & feel: Fusion style, stylesheet with light/dark themes that follow the OS, reusable card,
  toggle switch, status badge and slider + spin box parameter widgets; SVG app and tray icons.
- System tray / menu bar icon with menu (Show window, Exit) and an extension point for feature actions;
  closing or minimizing hides to the tray (one-time notification); on macOS the Dock icon is shown only
  while the window is visible. Quit requests other than Exit are refused, except when the OS session is
  ending (macOS logout/restart/shutdown is never blocked).
- Single-instance guard: a second launch brings the running window to the front.
- `--autostart` command-line flag (used from stage 1).
- Unit tests (Qt Test + CTest) for clock, logging and settings.
- GitHub Actions CI for macOS, Windows and Debian 13 with build artifacts.
- Documentation: `README.md`, `documentation/BUILD.md`, `documentation/MANUAL_TESTS.md`.
- Developer tool and script to regenerate the platform icons (`scripts/generate-icons.sh`).
