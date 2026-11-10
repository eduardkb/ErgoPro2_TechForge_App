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

---

## Stage 1 – v1.1.0

Goal: complete General tab, Install / Uninstall with start at login, startup behavior, permission
detection, the macOS permissions window and the "unavailable" banners.

**Preparation (macOS):**
1. Exit any running ErgoPro2 (General tab → Exit, or tray → Exit).
2. Build, then **sign** the app (BUILD.md 1.6 – ideally with your self-signed certificate, 1.6.3).
3. To see the permissions window, the permissions must be missing. If you granted them before, reset
   them: `tccutil reset Accessibility com.techforge.ergopro2` and
   `tccutil reset ListenEvent com.techforge.ergopro2`.
4. Start the app with `open build/bin/ErgoPro2.app` (**not** from Terminal – see BUILD.md 1.4).

### A. Version, layout and folders

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 6 test programs pass (adds `tst_startupmode`, `tst_locationclassifier`, `tst_featureavailability`). |
| 2 | Look at the title and the General tab. | Title **ErgoPro2 v1.1.0**; General tab cards in this order: ErgoPro2 (version **1.1.0**), Installation, Startup, Logging, Permissions & diagnostics, Reset, then the Exit button. Hovering the tray icon shows **ErgoPro2 v1.1.0** (Windows/Linux). |
| 3 | Make the window as narrow as possible and scroll the General tab. | No horizontal scrollbar; long paths wrap; nothing is cut off. |
| 4 | Logging card → **Open log folder**. | Finder/Explorer opens the log folder (BUILD.md §4). |
| 5 | Logging card → **Open settings folder**. | The folder containing `settings.json` opens. |

### B. macOS permissions window and banners (permissions missing)

| # | Steps | Expected result |
|---|-------|-----------------|
| 6 | Start ErgoPro2 with the permissions missing. | The window **"ErgoPro2 needs your permission"** appears in front, listing **Accessibility** and **Input Monitoring**, both **MISSING**, each with an explanation and **Open System Settings**. The log contains `Permission Accessibility : MISSING`. |
| 7 | Click **Continue without**. | The window closes; ErgoPro2 keeps running. |
| 8 | Open the **Auto Click**, **Key Control** and **Break Timer** tabs. | Each shows a yellow banner "<feature> is unavailable" with the reason, **Open System Settings** and **Check again**. Break Timer's banner mentions only **Input Monitoring**; Auto Click and Key Control mention both. |
| 9 | General tab → Permissions & diagnostics. | OS (e.g. macOS Tahoe 26.x, arm64), Desktop session *macOS desktop (Aqua)*, the application path, both permissions **MISSING** with buttons, Input hook **NOT RUNNING** ("added in a later version"), Start at login *Not registered*. |
| 10 | On the Auto Click banner click **Open System Settings**. | macOS shows its own hint and System Settings opens at **Privacy & Security → Accessibility**, with ErgoPro2 in the list (switched off). |
| 11 | Switch **ErgoPro2** on (confirm with password/Touch ID) and go back to ErgoPro2 without clicking anything. | Within about 3 seconds the diagnostics show Accessibility **GRANTED**; the Break Timer banner is unchanged (still needs Input Monitoring). |
| 12 | Grant **Input Monitoring** the same way (diagnostics card button). | Within ~3 s all three banners disappear and both permissions show **GRANTED**. If a permission stays MISSING although switched on, restart ErgoPro2 (step 15). |
| 13 | Exit and start again (`open build/bin/ErgoPro2.app`). | No permissions window appears (everything granted). |
| 14 | Reset the permissions (Preparation step 3) and start again. In the permissions window click **Open System Settings** for one permission and grant it. | The row changes to **GRANTED** by itself. A **Restart ErgoPro2** button has appeared. After granting the second permission the window says "All permissions are granted" and closes itself after ~1.5 s. |
| 15 | Reset one permission again, start ErgoPro2, click **Open System Settings** in the window, then **Restart ErgoPro2**. | ErgoPro2 quits and a new instance starts (window or tray according to the Startup options); no "already running" message. |

### C. Startup behavior

| # | Steps | Expected result |
|---|-------|-----------------|
| 16 | Startup card defaults. | *Started at login*: **Hide in tray**; *Started manually*: **Show window**. |
| 17 | Exit, then start with `open build/bin/ErgoPro2.app --args --autostart`. | No main window appears; only the tray/menu bar icon (no Dock icon on macOS). Log: `Started automatically at login (--autostart) - hidden in the tray`. Tray → Show window works. |
| 18 | Set *Started at login* to **Show window**, exit, start again with `--autostart`. | The window is shown. |
| 19 | Set *Started manually* to **Hide in tray**, exit, start normally. | Only the tray icon appears. Set both back to the defaults afterwards. |
| 20 | Check `settings.json`. | Contains `"startupAutoMode"` and `"startupManualMode"` in the `general` section with the chosen values. |

### D. Install / Uninstall (macOS)

| # | Steps | Expected result |
|---|-------|-----------------|
| 21 | Installation card before installing. | "Not installed – running from …/build/bin/ErgoPro2.app"; **Install** enabled, **Uninstall** disabled. |
| 22 | Click **Install**. | A message says ErgoPro2 was installed at `~/Applications/ErgoPro2.app` and will start at login, and asks to restart from there. `~/Applications/ErgoPro2.app` exists; `~/Library/LaunchAgents/com.techforge.ergopro2.plist` exists and contains that path plus `--autostart` (BUILD.md 5.0). macOS may show "Background Items Added". |
| 23 | Answer **Yes** to the restart question. | This instance exits and the installed copy starts. Its diagnostics card shows the application path inside `~/Applications`. Installation card: "✅ Installed at …, starts at login."; Install disabled (nothing to do), Uninstall enabled. |
| 24 | Log out of macOS and log in again. | ErgoPro2 starts automatically, hidden in the menu bar (default). The log contains `Started automatically at login (--autostart)`. |
| 25 | Start the **build** copy (`open build/bin/ErgoPro2.app`) while the installed copy is NOT running. | Installation card: "Installed at … starts at login" plus "This window runs from …/build/…"; the button reads **Reinstall**. |
| 26 | In the installed copy click **Uninstall** and confirm. | A message says ErgoPro2 no longer starts at login, the installed copy will be deleted after exit, and settings/logs are kept; ErgoPro2 exits. A few seconds later `~/Applications/ErgoPro2.app` and the LaunchAgent plist are gone. `settings.json` and the logs still exist. |
| 27 | Install again from the build copy, answer **No** to the restart, then click **Uninstall** in the same (build) window. | The installed copy and the plist are removed immediately; the build copy keeps running; the card shows "Not installed". |
| 28 | Location warning: copy `ErgoPro2.app` into `~/Downloads` and start it from there. | The Installation card shows "⚠ ErgoPro2 is running from your Downloads folder…". (A copy inside a mounted `.dmg` shows the disk-image warning.) |

### E. Reset settings

| # | Steps | Expected result |
|---|-------|-----------------|
| 29 | Change the log retention and a startup option, then click **Reset all settings to defaults…** → **No**. | Nothing changes. |
| 30 | Click it again → **Yes**. | All controls jump back to the defaults (30 days, recommended log level, Hide in tray / Show window); `settings.json` contains the defaults. Installation / start at login are unchanged. |

### F. Windows 11 and Debian (when a machine is available)

| # | Steps | Expected result |
|---|-------|-----------------|
| 31 | Windows: run **Install** from a `windeployqt` folder (BUILD.md 2.5). | Copied to `%LOCALAPPDATA%\Programs\ErgoPro2`; Start Menu shortcut "ErgoPro2"; Task Manager → Startup apps lists ErgoPro2; `reg query` shows the command with `--autostart`. Diagnostics: "No special permission needed" with the Administrator note; no banners. |
| 32 | Windows: run Install from `build\bin` (not deployed). | Error message asking to prepare the folder with windeployqt; nothing installed. |
| 33 | Windows: Uninstall from the installed copy. | Shortcut and Run value removed; the folder is deleted after ErgoPro2 exits. |
| 34 | Debian X11: Install. | `~/.local/opt/ErgoPro2/ErgoPro2` + `ergopro2.svg`, menu entry in the application menu, `~/.config/autostart/ergopro2.desktop` with `--autostart`. Diagnostics: Desktop session **X11**, "X11 session" **GRANTED**, no banners. |
| 35 | Debian Wayland: start ErgoPro2 as a user that is not in the `input` group. | Diagnostics: session **Wayland**; "Read access to input devices" and "Write access to /dev/uinput" **MISSING** with **Grant access…**; banners on the three tabs. |
| 36 | Click **Grant access…**. | A confirmation explains the changes (input group + udev rule); after **Yes** the system password dialog appears; on success a message asks to log out and in. After re-login both requirements are **GRANTED** and the banners are gone. Cancelling the password dialog shows "authorization was cancelled". |

---

## Bug fix – v1.1.1

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | Push the commit and open the GitHub Actions run. | All three jobs (macOS, Windows, Debian 13) are green. In the Windows job, `tst_locationclassifier` passes. In the macOS job, the *Configure* step prints `macOS SDK without AGL.framework (…): Qt's AGL link replaced by OpenGL.framework` and the *Build* step links `ErgoPro2` without `framework 'AGL' not found`. |
| 2 | macOS: `rm -rf build`, configure and build as in BUILD.md 1.2, run `ctest`. | Build succeeds, all tests pass (`tst_locationclassifier`: 16 passed). With Homebrew Qt 6.9+ the AGL message may appear but has no effect. |
| 3 | macOS: start the app from `build/bin` and look at General → Installation. | No location warning (the build folder is a normal folder). |
| 4 | Windows (when available): start a `windeployqt` folder from e.g. `C:\Tools\ErgoPro2` and look at General → Installation. | No "temporary folder" / "Downloads" warning. Copy the folder to `%USERPROFILE%\Downloads` and start it from there: the Downloads warning appears. |

---

## Bug fix – v1.1.2

Start ErgoPro2 with the main window visible (macOS: `open build/bin/ErgoPro2.app`; if *Started manually*
is set to *Hide in tray*, use tray → *Show window*).

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | Click the yellow **minimize** button. | The window disappears (minimize animation) and ErgoPro2 stays in the menu bar; the Dock icon disappears. |
| 2 | Menu bar icon → **Show window**. | The window comes back with all its content drawn (not blank). |
| 3 | Minimize again. If a thumbnail of the ErgoPro2 window is shown on the right side of the Dock, click it. | The window comes back **with its content drawn**. The log contains `Hidden main window was restored by the OS (e.g. Dock thumbnail): showing it`. |
| 4 | In that restored window, click the red **close** button. | The window hides to the menu bar (ErgoPro2 keeps running). Menu bar → Show window shows it again, drawn normally. |
| 5 | Minimize with **Cmd+M**, then menu bar → Show window. | Same as steps 1–2. |
| 6 | Repeat steps 1–4 three times in a row. | Same result every time; never a blank window. |
| 7 | Windows / Linux (when available): minimize, then restore from the tray. Also try restoring via the taskbar if a button remains. | The window hides to the tray on minimize and comes back drawn and closable. |

---

## Bug fix – v1.1.3

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | Build (BUILD.md 1.2). | The build output contains `Signing ErgoPro2.app (identity: -)` (or your certificate name). |
| 2 | `codesign -dv build/bin/ErgoPro2.app 2>&1 \| grep Identifier` and `codesign --verify --strict build/bin/ErgoPro2.app && echo valid` | `Identifier=com.techforge.ergopro2` and `valid`. |
| 3 | Exit ErgoPro2. In System Settings → Privacy & Security → **Accessibility** and → **Input Monitoring**, remove every **ErgoPro2** entry (select → **−**). Then run `tccutil reset Accessibility com.techforge.ergopro2` and `tccutil reset ListenEvent com.techforge.ergopro2`. | No ErgoPro2 entry is left in either list. |
| 4 | Start the new build with `open build/bin/ErgoPro2.app`, click **Install**, answer **Yes** to restart from the installed copy. | `codesign -dv ~/Applications/ErgoPro2.app` also shows `Identifier=com.techforge.ergopro2`. |
| 5 | In the permissions window click **Open System Settings** for Accessibility, switch ErgoPro2 on. Repeat for Input Monitoring. | Both become **GRANTED** within ~3 s (or after *Restart ErgoPro2*). The banners disappear. |
| 6 | Exit and start again with `open ~/Applications/ErgoPro2.app`. | No permissions window; diagnostics show both **GRANTED**. |
| 7 | Negative check: remove the signature of a *copy* (`ditto build/bin/ErgoPro2.app /tmp/E.app && codesign --remove-signature /tmp/E.app`) and start it with `open /tmp/E.app` (Accessibility not granted). | The Accessibility hint ends with "macOS cannot match it to this copy of ErgoPro2 because …", and the log contains `Accessibility cannot be matched to this copy of ErgoPro2`. Delete `/tmp/E.app` afterwards. |

> With ad-hoc signing, every rebuild + reinstall changes the app's identity: repeat steps 3–5 after
> each new install, or use the self-signed certificate (BUILD.md 1.6.3) to keep the permissions.

---

## Bug fix – v1.1.4

Preparation: exit ErgoPro2 and remove both permissions (System Settings → Privacy & Security →
Accessibility / Input Monitoring → select every ErgoPro2 entry → **−**), then start it with
`open ~/Applications/ErgoPro2.app` (or the build copy).

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | The window "ErgoPro2 needs your permission" opens. | Both cards fully readable: title, explanation, **MISSING** badge, hint text and **Open System Settings** button. |
| 2 | Grant **Accessibility** only (Open System Settings → switch on) and come back. | Within ~3 s Accessibility shows **GRANTED** and its card becomes smaller. The **Input Monitoring** card keeps its full height: all text, the **MISSING** badge and the button are completely readable. The window gets shorter but nothing is cut off. |
| 3 | Grant **Input Monitoring**. | Both cards show **GRANTED**, all text readable; "All permissions are granted" appears and the window closes after ~1.5 s. |
| 4 | Repeat with the opposite order (Input Monitoring first, then Accessibility). | Same: the remaining card is never squeezed. |
| 5 | While the window is open with one permission missing, try to drag the bottom edge of the window upwards. | The window cannot be made smaller than its content. |

---

## Stage 2 – v1.2.0

Goal: the shared global input layer (hook, click injection, key blocking, sleep/lock handling,
watchdog) and its diagnostics view. No end-user feature yet.

**Preparation (macOS):** build (`cmake --build build`), exit ErgoPro2, then **Reinstall** from the build
copy and restart from the installed copy (BUILD.md 5.0). Grant **Accessibility** and **Input
Monitoring** to the installed copy (BUILD.md 6.1). With ad-hoc signing, remove the old ErgoPro2 entries
first (BUILD.md 1.6.4). Start ErgoPro2 with `open ~/Applications/ErgoPro2.app`.

### A. Hook status and live events

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 10 test programs pass (adds `tst_inputevent`, `tst_keycombo`, `tst_dispatcher`, `tst_watchdog`). |
| 2 | Title / General tab. | **ErgoPro2 v1.2.0**. Permissions & diagnostics → *Input hook* **RUNNING**, "CGEventTap (active)". No banners on the function tabs. |
| 3 | General tab → **Input diagnostics** → **Show**. | The card expands: Hook **RUNNING** "CGEventTap (active) – can block keys". |
| 4 | Move the mouse over the whole screen (and to a second monitor if available). | *Pointer position* follows the mouse in real time (logical pixels, top-left of the main screen = 0,0); *Events per second* rises while moving; screen number changes on a second monitor. |
| 5 | Type in another app (e.g. TextEdit): `a`, `Shift+B`, `Ctrl+Option+Cmd+F9`; hold `x` for 2 s. | *Last key event* shows e.g. `KeyDown Shift+B (native 11)`, `KeyDown Ctrl+Alt+Meta+F9 …`; holding `x` shows `repeat`. Typing in the other app works normally. |
| 6 | Click left, right and middle buttons and scroll. | *Last mouse button* shows `MouseDown/MouseUp Left/Right/Middle at x,y`; *Synthetic* says "Last event: no". |
| 7 | **Test left click in 3 s**; within 3 s move the pointer over an empty desktop area. | After the countdown a real left click happens at the pointer, and the card says "✅ Click at x, y received back from the hook as synthetic after N ms". *Synthetic → Last synthetic event* shows `MouseDown Left … [synthetic]`. |

### B. Key blocking

| # | Steps | Expected result |
|---|-------|-----------------|
| 8 | Open TextEdit (or any app that reacts to F9; on a Mac keyboard use **fn+F9** if the top row are media keys). Press F9 once: note what happens. | The app reacts normally (baseline). |
| 9 | **Test: block F9 for 30 s**, switch to the other app and press F9 several times (also hold it). | Nothing happens in the other app. The card counts the blocked key events ("Blocked key events so far: N"). *Last key event* still shows the F9 presses. |
| 10 | Wait until the 30 s are over, press F9 again in the other app. | "Test finished: N F9 key event(s) were blocked. F9 works normally again." F9 works again. |
| 11 | During a new block test, press `Shift+F9` and `F8`. | Those are **not** blocked (only plain F9). |

### C. Permissions and robustness

| # | Steps | Expected result |
|---|-------|-----------------|
| 12 | Exit, remove **Accessibility** only (System Settings → − ), start again. | Hook RUNNING "CGEventTap (listen-only) – cannot block keys"; live events still work; the F9 test says it cannot block keys; Auto Click / Key Control show the banner for Accessibility. |
| 13 | Grant Accessibility again while ErgoPro2 runs. | Within ~3 s the banners disappear. The hook keeps running (restart ErgoPro2 to get the active tap; the log contains "Input hook running"). |
| 14 | Exit, remove **Input Monitoring**, start. | Diagnostics: *Input hook* NOT RUNNING "Waiting for the permissions above." – no crash. Granting it starts the hook within ~3 s (log: `Input hook running ( permissions changed )`). |
| 15 | Put the Mac to sleep (Apple menu → Sleep), wake it, unlock. | Log: `System is going to sleep`, `System resumed from sleep`, `Reinstalling the input hook: resume from sleep` (and/or `session unlock`). Diagnostics *Watchdog → Reinstalls* increased; live events and the F9 test work immediately. |
| 16 | Lock the screen (Ctrl+Cmd+Q) and unlock. | Log: `Screen locked`, `Screen unlocked`, a reinstall; everything keeps working. |
| 17 | **Long run:** leave ErgoPro2 running for 2+ hours with normal use, including at least one sleep/wake. Then repeat steps 4, 7 and 9. | Everything still works. The log shows a health check every 30 s at Debug level and no `Input hook keeps failing`. |
| 18 | Exit ErgoPro2 while the F9 block test is running, then press F9 in another app. | F9 works normally (the block ends with the app). |

### D. Windows 11 and Debian (when a machine is available)

| # | Steps | Expected result |
|---|-------|-----------------|
| 19 | Windows: steps 3–11. | Backend "WH_KEYBOARD_LL + WH_MOUSE_LL – can block keys"; positions in logical pixels (also on a 150 % scaled monitor); synthetic click recognized; F9 blocked. *Self-repairs* counts the 30 s re-arms. |
| 20 | Windows: sleep / lock (Win+L) and back. | Reinstall logged; everything keeps working. |
| 21 | Debian X11: steps 3–11. | Backend "X11 XRecord + XGrabKey". F9 blocked in other apps. Start another program that grabs F9 first (e.g. a desktop shortcut on F9): the block test reports "F9 is already used by another application". |
| 22 | Debian Wayland with the `input` group granted (BUILD.md 6.3): steps 3–11. | Backend "evdev (Wayland)"; pointer position shown as *relative*; synthetic click recognized; F9 blocked. Unplug/replug a USB keyboard: it keeps working (hotplug). |
| 23 | Debian Wayland, build without `libevdev-dev`. | Diagnostics/banners explain that this build has no Wayland input support; no crash. |
