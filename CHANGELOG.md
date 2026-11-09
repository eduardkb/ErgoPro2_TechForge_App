# Changelog

All notable changes to ErgoPro2 are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions use `MAJOR.MINOR.PATCH`:
each implementation stage bumps MINOR, each bug-fix run bumps PATCH, MAJOR stays 1
(see `documentation/implementation_stages.md`, A.4).

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
