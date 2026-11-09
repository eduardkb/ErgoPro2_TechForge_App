# ErgoPro2 – Manual Test Checklists

Each stage (and bug fix) adds a section here. Build and start the app as described in
[`BUILD.md`](BUILD.md). Paths of the settings file and log folder for each OS are listed in
`BUILD.md` section 4 (they are also printed in the log at every start).

Mark each step ✅ / ❌ and note the OS and version you tested on.

---

## Stage 0 – v1.0.0

Goal: application shell, settings, logging, tray, single instance, look & feel.

**Preparation:** exit any running ErgoPro2. To test the "first start" behavior, delete (or rename)
the settings file and the log folder first.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | Build and run `ctest --test-dir build --output-on-failure`. | All tests pass (`tst_clock`, `tst_logger`, `tst_settings`). |
| 2 | Start ErgoPro2. | The main window opens. Title: **ErgoPro2 v1.0.0**. On macOS the Dock icon is visible. |
| 3 | Look at the tabs. | Five tabs in this order: **General, Break Timer, Auto Click, Key Control, Statistics**. |
| 4 | Open each tab except General. | Break Timer, Auto Click and Key Control show an icon, a one-line description and "Coming in a later version." Statistics shows "Coming soon." |
| 5 | General tab. | Card "ErgoPro2" with a green **RUNNING** badge and **Version 1.0.0**; card "Logging" with *Log level* (default "Info + Warning + Error + Critical (recommended)"), *Keep log files for* slider + number box (default **30** days) and the log folder path; red **Exit ErgoPro2** button at the bottom. |
| 6 | Check the tray / menu bar. | An ErgoPro2 icon (mouse) is shown. On macOS it is monochrome and adapts to a light/dark menu bar. Hovering shows **ErgoPro2 v1.0.0** (Windows/Linux). |
| 7 | Open the tray icon's menu. | Entries: **Show window**, separator, **Exit**. |
| 8 | Check the settings file (see BUILD.md §4). | `settings.json` exists, contains `"schemaVersion": 1` and a `general` section with `logLevel`, `logRetentionDays` (30) and `trayHintShown`. |
| 9 | Check the log folder. | `ergopro2-<today>.log` exists; it contains the start lines (`ErgoPro2 1.0.0 starting on …`, `Settings file: …`, `Log folder: …`) in the format `YYYY-MM-DD HH:MM:SS.mmm [LEVEL] [thread] [module] message`. |
| 10 | Close the window with the window's close button (red ● on macOS / X). | The window disappears, the app keeps running (tray icon remains). **The first time only**, a notification says "ErgoPro2 is still running in the tray…". On macOS the Dock icon disappears. |
| 11 | Close the window a second time (after reopening it, step 12). | No notification this time. |
| 12 | Click/double-click the tray icon (Windows/Linux), or tray menu → **Show window** (all OSes). | The window reappears in front. On macOS the Dock icon reappears. |
| 13 | Minimize the window (yellow ● on macOS / _ button). | The window hides to the tray (same as closing). |
| 14 | macOS: press **Cmd+Q** while the window is focused. | The app does **not** exit (only the Exit button / tray Exit may quit it); the window hides to the menu bar. |
| 15 | Start ErgoPro2 a **second time** while it is running (hidden in the tray). | No second instance starts. The running instance's window comes to the front. The log shows "Activation requested by a second launch". |
| 16 | General tab: change *Log level* to "Info + Warning + Error + Critical + Debug". | Within a second `settings.json` shows `"logLevel": "info_warning_error_critical_debug"`. The log contains `Log level set to …`. New `[DEBUG]` lines appear in the log (e.g. when hiding the window: `Main window hidden to tray`). |
| 17 | Change *Log level* to "Info only", then hide and show the window. | `settings.json` shows `"logLevel": "info"`; no new `[DEBUG]` or `[WARNING]` lines are written. Set it back to the recommended level afterwards. |
| 18 | Drag the *Keep log files for* slider, then type a value in the number box (e.g. 7). | Slider and number box always show the same value. Values are limited to 1–180. `settings.json` shows the final value shortly after you stop. The log contains `Log retention set to 7 day(s)`. |
| 19 | Retention cleanup: exit the app, create a fake old log file in the log folder, e.g. `ergopro2-2020-01-01.log`, start the app. | The fake file is deleted at startup; the log contains `Removed 1 log file(s) older than … day(s)`. |
| 20 | Exit with **Exit ErgoPro2** on the General tab. | The app quits immediately without confirmation; the tray icon disappears. The log ends with `Exit requested by the user` and `ErgoPro2 exited with code 0`. |
| 21 | Start again. | The log level and retention days you set are restored (persisted). |
| 22 | Start, then quit via tray menu → **Exit**. | The app quits immediately. |
| 23 | Corrupt settings: exit, replace the content of `settings.json` with `{ broken`, start. | A warning dialog says the settings file was damaged and names the backup file `settings.corrupt-<date-time>.json`. The backup exists with the broken content; a new valid `settings.json` with defaults was written. |
| 24 | Invalid value: exit, edit `settings.json` and set `"logRetentionDays": 999`, start. | The app starts with 30 days; a tray notification says that 1 setting value was invalid and was reset. The log contains a WARNING `Invalid value for general.logRetentionDays`. |
| 25 | Light/dark mode: switch the OS appearance (macOS: System Settings → Appearance; Windows: Settings → Personalization → Colors) while the window is open. | The window switches between the light and dark theme immediately; all text stays readable. |
| 26 | Resize the window to its minimum size. | Nothing overlaps or is cut off; the General tab scrolls if needed. |
| 27 | `--autostart`: start from a terminal with `--autostart` (macOS: `./build/bin/ErgoPro2.app/Contents/MacOS/ErgoPro2 --autostart`). | The app starts (in stage 0 the window is still shown); the log contains `Started automatically at login (--autostart)`. |
| 28 | `--version` and `--help` from a terminal. | `--version` prints `ErgoPro2 1.0.0`; `--help` lists the `--autostart` option. |
| 29 | Log out of the OS (macOS: Apple menu → Log Out) while ErgoPro2 runs with its window **open**. | ErgoPro2 does **not** block or cancel the logout. After logging back in, the log contains `macOS session is ending` (macOS) and `ErgoPro2 exited with code 0`. |
