# Changelog

All notable changes to ErgoPro2 are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions use `MAJOR.MINOR.PATCH`:
each implementation stage bumps MINOR, each bug-fix run bumps PATCH, MAJOR stays 1
(see `documentation/implementation_stages.md`, A.4).

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
