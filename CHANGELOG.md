# Changelog

All notable changes to ErgoPro2 are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). Versions use `MAJOR.MINOR.PATCH`:
each implementation stage bumps MINOR, each bug-fix run bumps PATCH, MAJOR stays 1
(see `documentation/implementation_stages.md`, A.4).

## [2.2.0] - 2026-11-20

### Added
- **General tab › "Warnings and errors in the log"** (owner request): a card below the Logging card lists
  every WARNING, ERROR and CRITICAL entry of the last days' log files, newest first (at most the newest 500),
  in a fixed-height box with a scroll bar. It searches in the background only while the General tab is
  shown – at once when the tab is opened and the last search is old enough, then every few minutes – shows
  "Refreshing..." above the previous result while searching, and disappears when nothing is found. Not shown
  with the log level *Info only*.
- **settings.json** (not in the app): `general.logViewerDays` (3–30, default 7) and
  `general.logViewerRefreshMinutes` (3–120, default 5). Values outside the range become the nearest limit
  (WARNING in the log) instead of the default.
- Tests: new `tst_logproblems` (7 tests: which files, entry parsing incl. continuation lines, newest-first
  limit, when to search, which log levels); `tst_settings` checks the ranges and the nearest-limit rule.

## [2.1.0] - 2026-11-20

### Fixed
- **macOS `.dmg` window showed only two icons** (no arrow, no instructions) on macOS Tahoe. Root cause:
  dmgbuild 1.6.5 stored the background as a bookmark that Finder on macOS 26.2+ no longer follows
  (dmgbuild issue #273); the package script now uses dmgbuild **1.6.7** (needs Python 3.10+).
- The background explains both steps more clearly: "1. Drag the ErgoPro2 icon onto the Applications folder",
  "2. Open ErgoPro2 from Applications".

### Added
- **Installation confirmation (macOS):** on the first start of a copy dragged into Applications, ErgoPro2
  shows "ErgoPro2 is installed at … and starts automatically when you log in." (owner request).
- **Remove the installer `.dmg`:** together with that confirmation (and whenever the ErgoPro2 disk image is
  still mounted) ErgoPro2 offers to eject the disk image and move the `.dmg` to the Trash. When it is no
  longer mounted, downloaded `ErgoPro2-<version>-macos.dmg` files in Downloads / Desktop are offered.
- Tests: `tst_installpolicy` 13 tests (confirmation and `.dmg` offer rules, installer file names, reading
  `hdiutil info`). CI warns if the `.dmg` layout contains the old background bookmark again.

## [2.0.0] - 2026-11-20

Major version (owner decision): macOS installs like other Mac apps.

### Added
- **macOS drag-and-drop install:** opening the `.dmg` shows a window with ErgoPro2, an arrow and the
  **Applications** folder ("Drag ErgoPro2 to the Applications folder"). Dragging ErgoPro2 there installs it.
- When ErgoPro2 starts from Applications, the General tab shows it as **installed** and it sets itself to
  **start at login** (or moves the start at login from an older copy) – no Install click needed.
- Updating from 1.x: ErgoPro2 asks once whether to remove the older copy in `~/Applications` (your settings,
  statistics and logs are kept). New system-only setting `general.olderCopyKept`.

### Changed
- macOS **Install** button copies to `/Applications` (to `~/Applications` only when your account cannot
  write `/Applications`). A copy in either folder counts as installed; **Uninstall** removes both.
- The macOS `.dmg` is built with dmgbuild (`packaging/macos/dmg-settings.py`, background
  `packaging/macos/dmg-background.svg`); CI checks the Applications link and the window layout. The release
  notes describe the drag-and-drop install.
- Versioning: MAJOR is 2 from now on (spec A.4).
- Tests: new `tst_installpolicy` (9 tests: which copy counts as installed, Install target, start at login
  and older-copy question at start, reading the LaunchAgent); `tst_settings` checks the new setting.

## [1.9.1] - 2026-11-18

### Fixed
- **Keep screen active – clicks hit Teams buttons (macOS):** the click on the Teams / Zoom window now goes
  exactly to the window's **top-left corner** (where the pointer shows the resize arrows), once, with the left
  button, and the pointer is put back. Root cause: the 1.9.0 spot "a quarter of the window width in the title
  bar" lands on Teams controls (e.g. the search box) in narrow or rearranged windows. When neither Teams nor
  Zoom runs, the 1-pixel pointer movement stays (no click).
- **Keep screen active – not on after adding its hotkey:** adding a "Keep screen active on/off" hotkey in Key
  Control (or turning a disabled one back on in the hotkey list) now switches the feature on at once. Root
  cause: 1.9.0 only switched it on when the hotkey was pressed (owner change request).
- Tests: `tst_keepactive` now checks the corner click spot and when a hotkey change switches the feature on.

## [1.9.0] - 2026-11-18

Stage 9 – **Keep Screen Active** (Function 5, owner request; specification B.10,
`documentation/stage9_keep_screen_active.md`).

### Added
- **Keep screen active:** while you are away, ErgoPro2 keeps the screen awake and Microsoft Teams / Zoom
  showing you as active. After **3 minutes** without mouse or keyboard input it acts at once, then every
  **3 minutes**: if Teams runs, its window is brought back (also from minimized / tray / menu bar), its title
  bar is clicked once and the pointer is put back; otherwise the same with Zoom; otherwise the pointer moves
  1 pixel and back. Any real input stops it immediately. Nothing happens while the screen is locked, the break
  window is shown or Drag lock holds the button. Its moves and clicks are never counted by Statistics, Break
  Timer or Auto Click.
- **Key Control:** new action **Keep screen active on/off** (one hotkey). Disabling or deleting the hotkey
  switches the feature off. On/off is remembered across restarts.
- **Auto Click tab:** card **Keep screen active** (only while its hotkey exists): Enabled/Disabled, the
  hotkey and the status *Idle / Working / Disabled*.
- **settings.json:** `keepScreenActive.enabled`, `keepScreenActive.startAfterIdleMinutes` (1–60, default 3)
  and `keepScreenActive.intervalMinutes` (1–60, default 3); the two minute values are not shown in the app.
- Linux: X11 fully supported (a Teams that is only in the tray counts as not running); on Wayland only the
  pointer movement is used.
- Tests: new `tst_keepactive` (11 tests), Key Control tests extended (20 test programs on macOS).

## [1.8.2] - 2026-11-18

Owner-requested change (specification B.2 updated).

### Changed
- **settings.json: all keys now use the same style** (camelCase with the unit at the end, like
  `general.logRetentionDays`). The eight upper-case keys were renamed:

  | Old key (≤ 1.8.1) | New key |
  |---|---|
  | `autoclick.AUTOCLICK_TRIGGER_TIMER` | `autoclick.triggerTimeMs` |
  | `autoclick.AUTOCLICK_TRIGGER_DISTANCE` | `autoclick.triggerDistancePx` |
  | `draglock.DRAGLOCK_TIMEOUT` | `draglock.timeoutSeconds` |
  | `breaktimer.BREAKTIMER_IDLE_RESET` | `breaktimer.idleResetMinutes` |
  | `breaktimer.BREAKTIMER_MAX_WORK_TIME` | `breaktimer.maxWorkMinutes` |
  | `breaktimer.BREAKTIMER_BREAK_TIME` | `breaktimer.breakMinutes` |
  | `breaktimer.BREAKTIMER_POSTPONE_TIME` | `breaktimer.postponeMinutes` |
  | `breaktimer.BREAKTIMER_ACTIVITY_PAUSE` | `breaktimer.activityPauseSeconds` |

  **Your settings are kept:** at the first start an existing file is converted automatically (`schemaVersion`
  1 → 2; each value moves to its new name, the old name is removed; logged as INFO). If both names exist, the
  new one wins.
- The documentation (including the original description, the questions, older changelog entries and manual
  tests) uses the new names.
- 2 new settings unit tests (conversion, no upper-case key in the schema).

## [1.8.1] - 2026-11-18

Owner-requested change (specification B.8.3 updated).

### Changed
- **Statistics tab date header:** other days are shown as "<weekday> – <date>", e.g. "Tuesday – 10 November
  2026" (before: "Tuesday, 10 November 2026"); today stays "Today – <date>" and no longer repeats the weekday
  ("Today – 18 November 2026"). The date uses the system's long date format without the weekday.
- New unit test `tst_datetext` (19 test programs on macOS).

## [1.8.0] - 2026-11-16

Stage 8 – hardening, packaging and final documentation. No new features.

### Added
- **Documentation:** `documentation/USER_GUIDE.md` (every tab and setting, permissions, limitations,
  troubleshooting), `documentation/ARCHITECTURE.md` (modules, threads, event flow, how to add a Key Control
  action, statistics pipeline) and `documentation/REQUIREMENTS_TRACE.md` (every requirement → code, unit test,
  manual test; open gaps listed). `BUILD.md` links them and has new troubleshooting entries.
- **Debian package:** after installation a note explains the `input` group needed on Wayland
  (`packaging/linux/postinst`).
- `MANUAL_TESTS.md`: full Stage 8 regression checklist including a long-run / sleep-resume test.

### Fixed
- **Exit on OS logout / shutdown (all OSes):** statistics of the last minute, the settings and the release of
  hooks and keys now also happen when the OS ends the session (Windows did not return from the event loop);
  shutdown messages are written to the log file again; no error dialog can block the exit.
- **Statistics:** minutes that could not be written (e.g. disk full) are kept and written later instead of
  being lost; repairing a damaged last line of a CSV file is logged.
- **Windows:**
  - pointer positions were wrong on a second monitor with a different scaling (Auto Click overlay and trigger
    distance);
  - a hotkey with **Alt** (e.g. Alt+F9) opened the menu bar of the active application;
  - the 30-second health check could freeze ErgoPro2 when the hook thread hung; the routine re-arm is no
    longer counted (and logged) as a self-repair every 30 s;
  - Ctrl/Alt could stay "held" after Ctrl+Alt+Del or a UAC prompt, so hotkeys stopped working;
  - resume from sleep was reported twice; failures of the lock/unlock notification are logged.
- **Linux Wayland:** a key could stay pressed for the whole session when key blocking started or ended while
  a key was held; touchpad touches were counted as mouse clicks (and blocked Auto Click); a rare race at hook
  stop.
- **Linux X11:** a failing hook start leaked three X server connections per retry (every 30 s).
- **macOS:** a rare race between the health check and the removal of the event tap; a small leak when the hook
  restarted after a lost permission; a missing error check when reading the pointer position. The permission
  hint no longer points end users to a developer document.
- Installer file copies: a failure to set the executable flag is now reported instead of ignored.

## [1.7.9] - 2026-11-16

Owner-requested change (specification B.8.3 updated).

### Changed
- **Statistics tab redesign:**
  - The date and the **‹ Previous day / Today / Next day ›** buttons are now at the top and stay visible while
    scrolling. The buttons are slimmer.
  - *Computer usage*, *Activity* and *Day totals* are now one card (they always show the same day), separated by
    thin lines.
  - The long descriptions were removed: only a title per section (plus the chart legend). Also removed: the
    "Live – updated every minute" line, the privacy note under the totals and the hint under *Keep statistics
    for*.
  - The *Options* card stays at the bottom.

## [1.7.8] - 2026-11-12

Bug fix and owner-requested changes (specification updated: B.1.1, B.5.4, B.8.2, B.8.3).

### Fixed
- **macOS: removing the Accessibility permission while ErgoPro2 was running froze the whole computer** (no mouse
  or keyboard input anywhere until the screen was locked and unlocked). ErgoPro2 now notices within a second that
  the permission is gone, removes its event tap at once and shows the "permission missing" banners; it starts
  again by itself when the permission is granted again. Cause: the active event tap stayed installed (and was
  even re-enabled) after the permission was removed; only the reinstall at unlock removed it.

### Changed
- **Computer usage % now follows the Break Timer's active work time:** 0 % = no work time, 100 % = the active
  work time reached *Break after working* (short pauses count the same way as on the Break Timer tab; nothing
  counts while the break window is shown). Before, only seconds with input counted, so the break window
  appeared at e.g. 70 %. Resets (pause as long as the break, completed break) and Dismiss/Postpone (keeps
  growing beyond 100 %) work as before.
- **A changed *Break after working* becomes the new 100 % when a new work period starts** (active work time
  reset: idle reset, break completed, Dismiss) or immediately if the active work time is 0.
- **Maximum 500 %:** if breaks are ignored until the usage goes above 500 %, it is set back to 0 % – breaks then
  come after the full *Break after working* time again. The usage chart's axis stops at 500 %.
- **Main window:** can no longer be maximized or made full screen (all OSes); it can still be resized, up to
  80 % of the screen's usable width/height (minimum 720 × 560, smaller on small screens).
- Tests: 3 new / 2 adapted Statistics tests, new `tst_windowlimits` (18 test programs).

## [1.7.7] - 2026-11-11

Owner-requested change (specification B.2 updated).

### Changed
- **Break Timer ranges:**

  | Setting | New range | Default | Before |
  |---|---|---|---|
  | *Break after working* | 20–120 min | 50 | 5–180 |
  | *Reset after a pause of* | 2–15 min | 5 | 1–30 |
  | *Postpone by* | 3–20 min | 10 | 1–60 |

  A value stored in `settings.json` outside the new range is replaced by the default when ErgoPro2 starts
  (logged as WARNING).

## [1.7.6] - 2026-11-11

Owner-requested changes (specification updated: B.2, B.7, B.8.2).

### Changed
- **Computer usage % no longer jumps when "Break after working" is changed.** Before, changing 50 → 35 min
  at 75 % made it jump to 120 %, and 35 → 45 made it drop to 91 %. Now the percent continues on the old scale;
  the new minutes become the 100 % reference when the usage restarts at 0. The Break Timer still applies the
  new value at once, so a break can come before 100 % (and immediately if the active work time is already
  past the new value). Cause: the percent was always calculated against the current setting.
- **A completed break resets the computer usage to 0 %**, also when the mouse or keyboard was used while the
  break window was shown. Before, only a pause without any input reset it.
- **Breaks come after half the work time while the usage is above 100 %** (e.g. after Dismiss): with *Break
  after working* = 50 min the next break comes after 25 min of active work, until the usage is reset to 0.
- **Break length:** 3–15 min (was 1–30), default 4. A stored value outside the new range is replaced by 4.
- **Tabs:** the parameter descriptions no longer start with the variable names (`BREAKTIMER_…`,
  `AUTOCLICK_…`).
- 4 new / 2 adapted Statistics tests, 2 new Break Timer tests.

## [1.7.5] - 2026-11-11

Owner-requested changes (specification updated: B.1.1, B.7).

### Changed
- **Break Timer › Status:** new field **Work started at** between *Active work time* and *Next break in*:
  the time (24 h, HH:mm:ss) when the current work period started – the first mouse/keyboard input after the
  last reset. It shows "–" after a reset until you use the computer again (and while the timer is off).
- **Sliders no longer react to the mouse wheel.** Scrolling over a slider or its number box scrolls the page;
  values change only by clicking/dragging the slider or typing in (or clicking the arrows of) the number box.
  Applies to every slider (Break Timer, Auto Click, General, Statistics).
- **Drop-down boxes no longer react to the mouse wheel** either (General: startup options, log level; Add/
  Modify Hotkey: action): scrolling over them scrolls the page; choose a value by clicking the box.
- **General › Input diagnostics:** the "Test left click in 3 s" and "Test: block F9 for 30 s" buttons were
  removed; the card shows only the live hook information.
- 1 new Break Timer unit test (`tst_breaktimer`: 27).

## [1.7.4] - 2026-11-11

Owner-requested features (specification updated: B.1.3, new B.9).

### Added
- **GitHub releases:** after a push to `main` with a new version number, the CI creates the release
  **ErgoPro2_v<version>** with this version's CHANGELOG text and the downloads for all three systems: macOS
  `.dmg`, Windows setup `.exe`, Debian `.deb`. Pushing again with the same version creates nothing.
- **Windows setup program** `ErgoPro2-<version>-windows-x64-setup.exe` (Inno Setup):
  - installs for the current user without administrator rights, by default into
    `%LOCALAPPDATA%\Programs\ErgoPro2`; another folder can be chosen on the first installation;
  - an update goes into the folder of the existing installation without asking;
  - a running ErgoPro2 is closed first (it exits cleanly, like the Exit button); a copy that does not react
    is ended;
  - Start Menu entry, optional desktop icon, option "Start ErgoPro2 automatically when I log in" (on by
    default);
  - settings, logs and statistics stay where they are;
  - Windows "Apps" entry with an uninstaller.
- The **General tab** recognizes an installation made by the setup program: "Installed at <folder> …
  Installed with the ErgoPro2 setup program", and **Uninstall** runs the setup's uninstaller (ErgoPro2 exits
  first; settings, statistics and logs are kept).
- Command-line option **`--quit`**: asks the running ErgoPro2 to exit and waits for it (used by the setup and
  the uninstaller).
- CI checks the setup program: silent install, `--quit`, silent uninstall.

## [1.7.3] - 2026-11-10

### Fixed
- **macOS: ErgoPro2 crashed when the screen locked** (or the Mac went to sleep / woke up) – the log showed
  `CRITICAL … Uncaught exception in std::terminate: std::bad_function_call`. Cause: the sleep/lock
  notification handlers kept a reference to a temporary that no longer existed after start-up, so the first
  lock or sleep notification called an empty function (undefined behavior: it could also work by chance,
  which is why earlier lock tests passed). Each handler now owns its copy of the callback. Windows and Linux
  were not affected (they store the callback).
- New macOS regression test `tst_macpowerevents` (posts lock/unlock/sleep/wake notifications inside the test
  process after start-up; it crashed before the fix).

## [1.7.2] - 2026-11-10

Build and packaging fixes (CI artifacts).

### Fixed
- **macOS download could not be opened** ("Unable to expand … unsupported format"). The CI now uploads a
  **`.dmg`** directly. Cause (most likely – it could not be reproduced here): the CI put its own `.zip`
  into GitHub's artifact zip (a zip inside a zip), which macOS's Archive Utility does not open reliably;
  GitHub's zip also drops file permissions, which an app bundle needs.

### Changed
- **One ready-to-use package per operating system**, each uploaded as the file itself (no extra zip):
  - macOS: `ErgoPro2-<version>-macos.dmg` – universal (Apple silicon + Intel), Qt bundled, only the Qt
    plugins/frameworks ErgoPro2 uses, ad-hoc signed, verified.
  - Windows: `ErgoPro2-<version>-windows-x64.zip` – one `ErgoPro2` folder with only the needed files:
    `ErgoPro2.exe`, only the Qt DLLs it loads, 3 Qt plugins and the Microsoft C++ runtime DLLs (no
    Redistributable install needed). Translations, software OpenGL, D3D compiler, styles, TLS and unused
    image formats are gone. The build fails if a needed DLL is missing.
  - Debian 13: `ergopro2_<version>_amd64.deb` – installs with `sudo apt install ./…deb` (dependencies
    computed automatically); the CI installs and starts it as a check.
- Packaging scripts `scripts/package-macos.sh`, `scripts/package-windows.ps1`, `scripts/package-linux.sh`
  (also for local builds; documentation/BUILD.md 1.5, 2.5, 3.5). No change to the application itself.

## [1.7.1] - 2026-11-10

Owner-requested changes (specification Part B updated: B.3, B.7, B.8.4).

### Changed
- **No file is deleted when a retention setting is changed.** Lowering *Keep statistics for* (Statistics tab) or
  *Keep log files for* (General tab) no longer deletes files right away – a slider moved by mistake cannot
  delete anything. Old files are deleted only when ErgoPro2 starts and once a day. The hints on both settings
  say so; the text "also right away when you lower this" is gone. Cause: both settings triggered a cleanup on
  every change (and the log cleanup also ran every hour).
- **Break Timer – State "Paused":** while the active work time stands still because there was no mouse or
  keyboard input for 20 s, the Status box shows **Paused** instead of *Working*. During a postpone it keeps
  showing *Postponed*.
- 1 new Break Timer unit test (`tst_breaktimer`: 26).

## [1.7.0] - 2026-11-10

Stage 7 – Function 4: Statistics.

### Added
- **Statistics tab** (replaces "Coming soon"):
  - **Computer usage** chart: one point per minute.
    - 100 % = mouse/keyboard use for the whole *Break after working* time. Only seconds with real input count.
    - The value keeps growing beyond 100 % when a break is dismissed or postponed, and goes back to 0 % only
      after a pause of *Break length* without input (sleep, a locked screen and ErgoPro2 not running count as a
      pause).
    - It also works with the Break Timer switched off.
    - Dashed 100 % line, markers for break shown / dismissed / postponed / completed, tooltip on hover; gaps are
      not connected.
  - **Activity** chart: mouse clicks, key presses and mouse distance in 5-minute bars on the same time axis,
    each with its own scale, with a tooltip.
  - **Day totals:** clicks (left / right / other), *clicks done for you* (Auto Click, Key Control and Drag Lock
    clicks), key presses, wheel, approximate mouse distance (from the screen's DPI), mouse / keyboard / total
    active time (h:mm:ss), highest usage.
  - **‹ Previous day / Today / Next day ›:** jumps only between days that have a file. Today updates live once
    per minute; past days are read-only.
  - **Options:** *Open statistics folder* and **Keep statistics for** 1–360 days (default 90).
- **CSV files:** one file per day, `ergopro2-stats-YYYY-MM-DD.csv`, in the `statistics` folder next to the logs
  (also shown on the General tab with an *Open statistics folder* button).
  - One row per minute with data. Each finished minute is appended and the current minute is written on exit.
  - Today's file is continued after a restart, and the usage percent continues if the restart was within the
    break length.
  - A partial last line after a crash is repaired; unreadable lines are skipped and logged.
  - Old files are deleted at startup, at midnight and immediately when the retention is lowered.
- **Privacy:** only counts and durations are stored – never which keys were pressed, pointer positions or
  application names.
- New signals for the statistics: Auto Click and Key Control clicks, and Break Timer events (shown, dismissed,
  postponed, completed).
- New availability entry "Statistics recording" (macOS: needs Input Monitoring) with the usual banner on the
  tab.
- Setting `statistics.retentionDays`.
- 31 unit tests: `tst_statistics` (collector, usage percent, totals, chart axes) and `tst_statisticsstore` (CSV
  format, continuing a file, repair, invalid lines, navigation, retention).

## [1.6.1] - 2026-11-10

Bug fixes and owner-requested changes (specification Part B updated).

### Fixed
- **Key Control tab – hotkeys missing from the list:** with more than two hotkeys, entries were cut off and the
  "Every action already has a hotkey" text overlapped the list. The page now scrolls like the other tabs and the
  list is always as tall as all its entries (no scroll bar of its own). Cause: the Key Control page was not in a
  scroll area, so its card was squeezed to the window height.
- **Break Timer tab – everything greyed out:** the settings and the on/off switch stayed disabled even with all
  permissions granted. Cause: the tab is created before the input hook starts and never re-checked whether
  activity monitoring had become available.

### Changed
- Key Control: "Every action already has a hotkey…" is shown **above** the list, in the warning color.
- Key Control: the **Drag lock** card is removed from the tab. `draglock.timeoutSeconds` stays in
  `settings.json` as a system-only setting.
- Break Timer: the **Status** panel is now right below the on/off switch, above the settings.
- Break Timer: **Break length** default is now **4 min** (was 5). Only new installations and "Reset all
  settings" get the new default; a value already stored is kept.
- Break Timer: **Active work time** now counts only while you use the computer: after 20 s without any mouse or
  keyboard input it stops (those 20 s still count) and continues with the next input. The break appears when
  this active time reaches *Break after working*. The 20 s are the system-only setting
  `breaktimer.activityPauseSeconds` (5–60 s) in `settings.json`.
- 5 new / 3 adapted Break Timer unit tests (`tst_breaktimer`: 25).

## [1.6.0] - 2026-11-10

Stage 6 – Drag Lock (Key Control action) and its interaction with Auto Click and the Break Timer.

### Added
- **Drag lock (left button)** is now selectable in the Add/Modify Hotkey dialog. Pressing its hotkey holds the
  left mouse button down at the pointer, so you can drag files, select text or move windows without holding
  the button. The button is let go (at the current pointer position) when:
  - the drag-lock hotkey is pressed again (the key is not passed on),
  - `draglock.timeoutSeconds` seconds pass **without mouse movement** (D-02),
  - any non-modifier key is pressed (the key reaches the application normally; Shift/Ctrl/Option/Cmd alone
    do not let go, D-01); another hotkey lets go first and then does its action,
  - any mouse button is pressed – that click only lets go and is **not passed on** (D-03),
  - the break window appears (D-06),
  - the hotkey is disabled, deleted, modified or reset, Key Control is paused (hotkey dialog open) or
    becomes unavailable, Auto Click is switched off, the input hook stops, or the application exits.
- **Auto Click while drag-locked:** no auto-click while the button is held and none on release; the release
  position becomes the reference point, so the pointer must move `autoclick.triggerDistancePx` before the next
  auto-click (B.4 rule 7, D-04).
- **Indicators:** the tray icon shows the drag-lock variant while the button is held (D-05; takes priority
  over the Auto Click variant), and the hotkey's row in the Key Control list shows **HOLDING**.
- **Drag lock timeout** parameter (1–60 s, default 10) on the Key Control tab; setting
  `draglock.timeoutSeconds`.
- Input layer: the hooks can now swallow the click that ends a drag lock (macOS event tap, Windows
  `WH_MOUSE_LL`). On macOS, pointer moves while the button is held are turned into left-mouse-dragged events,
  because a button pressed by ErgoPro2 does not make macOS report drags by itself.
- **Safety:** if ErgoPro2 crashes or is killed (SIGSEGV, SIGABRT, SIGTERM, …, Windows unhandled exception,
  `std::terminate`) while the button is held, the button is released before the process ends. Every press and
  release is logged with its reason; a failed release is logged as CRITICAL and shown on the Key Control tab.
- Drag Lock state machine (pure, fake clock) with 21 unit tests (`tst_draglock`, incl. the Auto Click
  reference point and the click consumption); 3 new Key Control tests.

### Known limitations
- Linux (X11 and Wayland/evdev): the click that ends a drag lock cannot be swallowed – XRecord and evdev only
  observe the mouse. The drag lock still ends; a left click merges with the held button, other buttons also
  reach the application.

## [1.5.0] - 2026-11-10

Stage 5 – Function 3: Break Timer.

### Added
- **Break reminders based on real activity:** the work time (INTERACTION_TIMER) starts with the first mouse or
  keyboard use and counts wall-clock time; short pauses keep counting, a pause of `breaktimer.idleResetMinutes`
  (also sleep or a locked screen, including the time asleep) resets it. After `breaktimer.maxWorkMinutes` the
  break window appears. Only real input counts – clicks from Auto Click and Key Control do not.
- **Break window** on every screen (one per screen, also screens connected during the break): semi-transparent
  overlay, always on top (macOS: screen-saver level on all Spaces and over full-screen apps; Windows:
  topmost; Linux: stay-on-top + full-screen state). The main screen shows "Time to rest your wrists", an mm:ss
  countdown, five illustrated stretching tips and **Dismiss** / **Postpone N min**. It cannot be closed with
  Esc, Alt+F4 or Cmd+W; it closes when the countdown reaches zero. Auto Click and hotkeys work on it.
- Rules: auto-close after `breaktimer.breakMinutes` and reset; Dismiss resets immediately; Postpone brings the
  window back after `breaktimer.postponeMinutes` while the work time keeps counting (unlimited); resting for the
  idle-reset time during a postpone cancels it; activity during a break changes nothing; no break starts
  while the screen is locked; parameter changes apply to the running work period; disabling ends a break.
- Optional **"No breaks during full-screen apps"** (off by default): macOS and Windows detect full-screen
  applications; on Linux the option is disabled with an explanation.
- **Break Timer tab:** enable switch with status badge, the four durations (minutes), the full-screen option
  and a live status panel (active work time, next break in, state) updated every second. No "break now" or
  "reset" buttons (B-04).
- Tray menu: **Break Timer on/off** and **Start break now**.
- Signal `breakWindowShown()` for stage 6 (Drag Lock release).
- Settings `breaktimer.enabled`, `breaktimer.idleResetMinutes`, `…_MAX_WORK_TIME`, `…_BREAK_TIME`,
  `…_POSTPONE_TIME`, `breaktimer.suppressWhenFullscreen`.
- Break Timer engine (pure state machine) with 21 unit tests driven by the fake clock.

## [1.4.2] - 2026-11-10

Owner-requested change (specification B.4 updated).

### Changed
- **Auto Click countdown cross:** the orange X no longer flashes while the mouse is moved slowly. It now
  appears only after the pointer has made no movement at all (not even tiny tremor movements) for half the
  click time, and then grows until the click. Any movement hides it again. The click itself is unchanged.
  Cause of the flashing: movements smaller than the tremor tolerance do not restart the click countdown, so
  a slowly creeping pointer kept starting short countdowns that each showed the X.

## [1.4.1] - 2026-11-10

Owner-requested changes (specification updated in `implementation_stages.md` Part B).

### Changed
- **Key Control – one hotkey per action:** an action can be assigned to only one hotkey. The Add/Modify
  dialog offers only actions without a hotkey (plus the hotkey's own action when modifying); an action
  becomes available again when its hotkey is deleted. Disabled hotkeys keep their action. When every action
  has a hotkey, *Add Hotkey* is disabled with an explanation. Stored duplicates are shown with an Error status.
- **Tab icons:** every tab shows an icon next to its name (new icon for General); icons use the accent color
  and turn black on the selected tab.
- **Auto Click countdown:** the countdown ring is replaced by an orange X that grows outward from the pointer
  tip to about the size of the mouse cursor when the click happens (option *Show countdown cross*).

## [1.4.0] - 2026-11-10

Stage 4 – Function 2: Key Control (hotkeys; Drag Lock follows in stage 6).

### Added
- **Key Control tab**: *Add Hotkey* and a list of hotkeys with key, action and status (Active / Disabled /
  Paused / Unavailable / Error + reason) and the buttons **Enable/Disable**, **Modify…**, **Reset binding**
  and **Delete…** (with confirmation). Controls are disabled while Key Control is unavailable.
- **Add / Modify Hotkey dialog** (K-15): "Press the key or key combination…" → the captured combination with
  *Confirm* / *Retry* (or *Use anyway* for a plain typing key, or the reason why it cannot be used) → choose
  the action → *Save*; if the key cannot be registered the reason is shown and another key can be chosen.
  Keys are captured from the global input hook, so they match exactly what the hook sees at runtime. All
  hotkeys are paused while the dialog is open (K-14).
- Actions (pluggable registry): **Left mouse click**, **Right mouse click**, **Double left click** (OS
  double-click time), **Turn Auto Click on/off**. Drag lock is registered but hidden until stage 6.
- Hotkeys are exclusive: while enabled, other applications do not receive the key (also its auto-repeat and
  release). Each press fires once on key-down; clicks happen at the pointer and count as manual clicks for
  Auto Click. Hotkeys also work on the ErgoPro2 window.
- Key combination rules: F1–F24, keypad keys and any key with a modifier are allowed; a plain typing key
  needs confirmation; modifier-only keys, unknown keys (Fn/Globe, media keys) and combinations reserved by
  the OS (e.g. ⌘Tab, ⌘Space, ⌥⌘Esc on macOS; Ctrl+Alt+Del, Alt+Tab, Win+… on Windows; Ctrl+Alt+F1…F12 on
  Linux) are rejected with an explanation; a combination can be assigned only once.
- Display of combinations per OS (⌃⌥⇧⌘F9 on macOS, Ctrl+Alt+Shift+Win+F9 on Windows, …+Super+… on Linux).
- Setting `keycontrol.actions` (list of `{id, keyCombo, action, enabled}`); invalid stored entries are
  logged, kept unchanged and shown with an Error status. *Reset all settings* clears the list and releases
  the keys.
- Robustness: hotkeys are re-registered after hook reinstalls (watchdog, resume, unlock); statuses update
  live; all keys are released on exit.
- Unit tests for combination text, validation and reserved combinations, the stored list, the action
  registry and the Key Control engine with a fake input layer (32 cases).

### Changed
- The settings schema supports list values (validated by the feature that owns them).

## [1.3.0] - 2026-11-10

Stage 3 – Function 1: Auto Click.

### Added
- **Auto Click** (dwell click): when the pointer has moved at least `autoclick.triggerDistancePx` from the last
  click and then rests for `autoclick.triggerTimeMs`, one left click happens at the pointer. Resting the mouse
  never produces repeated clicks.
  - Movements up to the tremor tolerance (1–10 px, default 3) do not restart the countdown.
  - A manual click (any button) cancels a pending auto-click and becomes the new reference point (on press
    and release, so a manual drag never leaves Auto Click armed at the drop position).
  - No auto-click while a mouse button is held or within 0.5 s after a key press (modifier keys alone do
    not count); a pending click is delayed, not cancelled.
  - Ready for later stages: Key Control clicks count as manual clicks, Drag Lock suppression and reference
    point reset.
  - Works on every window, including ErgoPro2's own window.
- AutoClick engine as a pure state machine in `src/core` (Disabled, Waiting for movement, Armed, Countdown,
  Suppressed) with 22 unit tests driven by the fake clock.
- AutoClick worker running in its own thread, fed by the input dispatcher, injecting the click and
  recognizing its own click when the hook reports it back as synthetic.
- **Auto Click tab**: availability banner, large "AutoClick is ACTIVE / INACTIVE" badge with
  Activate/Deactivate, live state line, trigger time (100–3000 ms, default 200), distance (5–200 px, default
  15), tremor tolerance (1–10 px, default 3) and the optional countdown ring. Controls are disabled while the
  feature is unavailable.
- Optional visual feedback: a click-through ring next to the pointer that fills during the countdown (off by
  default; not available on Wayland, with an explanation).
- Tray menu entry **AutoClick on/off** and a tray icon variant while Auto Click is active.
- Settings `autoclick.enabled`, `autoclick.triggerTimeMs`, `autoclick.triggerDistancePx`,
  `autoclick.jitterTolerancePx`, `autoclick.showVisualFeedback`; changes apply immediately and persist.

### Changed
- Disabled buttons, sliders and inputs now look disabled in both themes.
- Number boxes of slider rows have a fixed width, so the sliders line up.

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
