# ErgoPro2 – Manual Test Checklists

Each stage (and bug fix) adds a section here. Build and start the app as described in
[`BUILD.md`](../BUILD.md). Paths of the settings file and log folder for each OS are listed in
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
Monitoring** to the installed copy (BUILD.md 6.1). With ad-hoc signing, remove the old ErgoPro2 permissions
completely first (BUILD.md 1.6.6). Start ErgoPro2 with `open ~/Applications/ErgoPro2.app`.

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

---

## Stage 3 – v1.3.0

Goal: Function 1 – Auto Click (dwell click).

**Preparation (macOS):** build, **Reinstall** from the build copy and restart from the installed copy
(BUILD.md 5.0). If permissions were granted to an older build, reset them first (BUILD.md 1.6.6) and grant
**Accessibility** and **Input Monitoring** again (BUILD.md 6.1). Start with
`open ~/Applications/ErgoPro2.app`. Use TextEdit and Finder as test targets.

> Tip: start with a longer trigger time (e.g. 800 ms) to see what happens, then return to 200 ms.

### A. Tab, settings and persistence

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 11 test programs pass (adds `tst_autoclick`, 22 cases). |
| 2 | Open the **Auto Click** tab. | No banner (permissions granted). Card with "AutoClick is INACTIVE" and **Activate**; Settings card with *Wait before clicking* **200** ms, *Minimum movement* **15** px, *Ignore tremor below* **3** px, *Show countdown ring* off. |
| 3 | Change the three values (slider and number box). | Values stay in sync, limits are 100–3000 ms, 5–200 px, 1–10 px. `settings.json` → `autoclick` section shows the new values shortly after. |
| 4 | Exit and start again. | The values and the on/off state are restored. |
| 5 | Remove Accessibility (BUILD.md 1.6.6, only that one) and start. | Banner "Auto Click is unavailable…"; Activate, sliders and the ring switch are **disabled** (grey). Grant it again: within ~3 s the banner disappears and the controls are enabled. |

### B. Clicking behavior (Auto Click active)

| # | Steps | Expected result |
|---|-------|-----------------|
| 6 | Click **Activate**. | Badge **AutoClick is ACTIVE** (green), button **Deactivate**; the menu bar icon changes to the Auto Click variant; state line "Waiting: move the pointer…". |
| 7 | Move the pointer more than 15 px (e.g. onto a Finder file) and stop. | After 200 ms **one** left click (the file is selected). State line: "Armed…" while moving, "Clicking when the pointer rests…", then "Waiting…". Log: `AutoClick: left click at x , y`. |
| 8 | Keep the mouse completely still for 10 s. | **No further clicks.** |
| 9 | Move only a few pixels (less than 15 px from the last click) and stop. | No click. |
| 10 | Rest your hand on the mouse with tiny tremor (1–3 px) after arming. | The click still happens once 200 ms after the last *real* movement; tremor does not delay it forever. |
| 11 | Move > 15 px and click **manually** before the 200 ms pass (left or right button). | Only your manual click happens; no extra auto-click. The next auto-click needs a new 15 px movement from the manual click position. |
| 12 | Drag a file in Finder manually (hold the button, move, release). | No auto-click during the drag and none at the drop position after releasing. |
| 13 | Move > 15 px onto a TextEdit window, stop, and start typing immediately. | The click is delayed while you type and happens 0.5 s after the last key; keys keep working. Pressing only Shift/Cmd does not delay it. |
| 14 | Hover over buttons of the ErgoPro2 window itself (e.g. the tabs) and rest. | Auto Click clicks them too (A-06). |
| 15 | Tray menu → **AutoClick on/off**. | Toggles Auto Click; the tab badge, the button and the tray icon follow. Toggling in the tab updates the tray entry's check mark. |
| 16 | General → Input diagnostics → Show, then let Auto Click click. | *Last synthetic event* shows `MouseDown Left … [synthetic]`; *Last mouse button* is not changed by it, and Auto Click does not treat its own click as a manual click (it still clicks normally next time). |

### C. Visual feedback

| # | Steps | Expected result |
|---|-------|-----------------|
| 17 | Switch on **Show countdown ring**, move > 15 px and stop. | A small ring appears next to the pointer (down-right) and fills up; the click happens when it is full; the ring disappears. It never takes focus (typing in TextEdit continues) and clicks go "through" it. |
| 18 | Move again before the ring is full. | The ring follows the pointer and restarts. |
| 19 | Switch the ring off. | No ring is shown any more. |

### D. Robustness

| # | Steps | Expected result |
|---|-------|-----------------|
| 20 | With Auto Click active, put the Mac to sleep and wake it. | After unlocking, Auto Click keeps working (hook reinstalled, see the log). |
| 21 | Exit ErgoPro2 while armed (move, then quickly use tray → Exit). | No click happens after exit. |
| 22 | Windows / Debian (when available): steps 6–19. | Same behavior. On Wayland the ring option is disabled with an explanation and positions are relative. |

---

## Stage 4 – v1.4.0

Goal: Function 2 – Key Control (exclusive hotkeys that click the mouse). Drag Lock follows in stage 6.

**Preparation (macOS):** build, **Reinstall** and restart from the installed copy (BUILD.md 5.0). This build
is a new app for macOS: reset and grant **Accessibility** and **Input Monitoring** again (BUILD.md 1.6.6,
6.1). Start with `open ~/Applications/ErgoPro2.app`. Use TextEdit and Finder as test targets.

> On Mac keyboards the top row are media keys: press **fn** + F-key, or turn on *Use F1, F2, etc. keys as
> standard function keys* (System Settings → Keyboard → Keyboard Shortcuts → Function Keys).

### A. Adding hotkeys and the dialog

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 12 test programs pass (adds `tst_keycontrol`, 32 cases). |
| 2 | Open **Key Control**. | No banner; "No hotkeys yet…"; **Add Hotkey** enabled; a Tips card. |
| 3 | **Add Hotkey**, press **F9**. | Page "You pressed: F9" with **Confirm** / **Retry**. |
| 4 | **Retry**, then hold ⌃⇧ and press **F9**. | While holding, the held modifiers are shown (⌃⇧). Then "You pressed: ⌃⇧F9". |
| 5 | **Confirm** → choose *Left mouse click* → **Save**. | The list shows `⌃⇧F9 · Left mouse click · ACTIVE`. Log: `Hotkey added: ⌃⇧F9 -> Left mouse click`. |
| 6 | Add Hotkey and press **⌘Tab** (or ⌘Space). | "This combination is reserved by the operating system: …", Confirm disabled. (macOS may switch apps for ⌘Tab: that is the reason it is not allowed.) |
| 7 | Add Hotkey and press the plain letter **A**. | Warning "This key will no longer type…", button **Use anyway**. Cancel. |
| 8 | Add Hotkey and press **⌃⇧F9** again. | "⌃⇧F9 is already assigned to "Left mouse click"", Confirm disabled. Cancel. |
| 9 | Add **F10** → *Turn Auto Click on/off*, **⌃⌥F8** → *Right mouse click*, **Num 7** (keypad, if available) → *Double left click*. | All shown as ACTIVE. |
| 10 | Open the Add dialog and, while it is open, press ⌃⇧F9 in it. | Nothing is clicked: all hotkeys are paused while the dialog is open (status PAUSED in the list behind it). After Cancel they are ACTIVE again. |

### B. Hotkeys in use

| # | Steps | Expected result |
|---|-------|-----------------|
| 11 | Point at a Finder file, press **⌃⇧F9**. | The file is selected (left click at the pointer). Holding the keys clicks **once** only. |
| 12 | Press **⌃⌥F8** over the desktop. | A right click: the context menu opens. |
| 13 | Press **Num 7** over a Finder folder. | Double click: the folder opens. |
| 14 | Press **F10** twice, watching the Auto Click tab or the menu bar icon. | Auto Click turns on, then off. |
| 15 | Open TextEdit with some text; press ⌃⇧F9, F10 and ⌃⌥F8 there. | No text is typed and TextEdit does not react to those keys (they are exclusive). Other keys type normally. |
| 16 | With Auto Click active: move > 15 px, then press ⌃⇧F9 before the trigger time. | Only the hotkey click happens; Auto Click does not add a second click (a Key Control click counts as a manual click). |
| 17 | Press a hotkey while the ErgoPro2 window is in front. | Works there too. |

### C. List operations and persistence

| # | Steps | Expected result |
|---|-------|-----------------|
| 18 | **Disable** ⌃⌥F8, then press it in TextEdit. | Status DISABLED; the key reaches TextEdit again (works as it normally would); no right click. **Enable** → ACTIVE and exclusive again. |
| 19 | **Modify…** F10: keep the key, change the action to *Left mouse click* → Save. | The list shows F10 · Left mouse click; F10 now clicks. |
| 20 | **Modify…** F10 → **Change key…** → press **F11** → Confirm → Save. | The row shows F11; F10 works normally again in other apps, F11 clicks. |
| 21 | **Reset binding** on any row. | Stays ACTIVE (log: `Hotkey binding reset: … OK`). |
| 22 | **Delete…** a row → **No**, then **Delete…** → **Yes**. | First nothing happens; then the row disappears and the key works normally again. |
| 23 | Exit and start ErgoPro2. | All hotkeys are restored with their keys, actions and enabled state. `settings.json` → `keycontrol.actions` lists them (`"keyCombo": "Ctrl+Shift+F9"`, …). |
| 24 | Exit, edit `settings.json`: change one `"keyCombo"` to `"Hyper+X"` and one `"action"` to `"teleport"`; start. | Both rows show **ERROR** with the reason ("Unknown key combination…", "Unknown action…"); the other hotkeys work; the log lists both entries. Delete them or fix the file. |
| 25 | General → **Reset all settings to defaults…** → Yes. | The hotkey list is empty and every key works normally again. |

### D. Permissions and robustness

| # | Steps | Expected result |
|---|-------|-----------------|
| 26 | Remove **Accessibility** (BUILD.md 1.6.6, only that one) and start. | Banner on Key Control; rows show UNAVAILABLE; Add Hotkey and the row buttons are disabled; the keys reach other apps normally. Grant it again and restart: rows ACTIVE. |
| 27 | Put the Mac to sleep, wake and unlock; also lock/unlock the screen. | Hotkeys keep working right away (log: hook reinstall; `Key Control: re-registered …` if a key had been lost). |
| 28 | **Long run:** leave ErgoPro2 running for 2+ hours with normal use (include a sleep/wake), then repeat steps 11 and 15. | Still exclusive and working. |
| 29 | Exit ErgoPro2, then press the hotkeys in TextEdit. | They behave like normal keys again (released on exit). |
| 30 | Windows / Debian X11 (when available): steps 3–25. | Same behavior; combinations shown as Ctrl+Shift+F9 / Win / Super; Windows rejects Win+… and Ctrl+Alt+Del; on X11 a key already grabbed by another program shows "already used by another application (X11 grab refused)". |

---

## Bug fix – v1.4.1

Owner-requested changes. Build, Reinstall, re-grant the permissions if needed (BUILD.md 1.6.6), start the
installed copy.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 12 test programs pass (`tst_keycontrol`: 38 cases). |
| 2 | **Key Control:** with hotkeys for some actions (e.g. F7 → Left click), click **Add Hotkey**, press a free key, Confirm. | The *Action* list does **not** contain actions that already have a hotkey (e.g. no "Left mouse click"). |
| 3 | **Modify…** the F7 hotkey. | Its own action "Left mouse click" is in the list (and selected); actions of other hotkeys are not. |
| 4 | **Disable** a hotkey, then Add Hotkey. | Its action is still not offered (disabled hotkeys keep their action). |
| 5 | **Delete…** a hotkey → Yes, then Add Hotkey. | Its action is offered again. |
| 6 | Assign all four actions (Left, Right, Double left click, Auto Click on/off). | **Add Hotkey** is disabled; the text "Every action already has a hotkey…" is shown. Deleting one enables it again. |
| 7 | Exit, edit `settings.json` so two entries have the same `"action"`, start. | The second entry shows **ERROR** "… is already assigned to …"; the first works. |
| 8 | Look at the tab bar in light and dark mode; click through all tabs. | Every tab (General, Break Timer, Auto Click, Key Control, Statistics) has an icon at the size of the tab text. Unselected icons are teal; the selected tab's icon is **black**. |
| 9 | **Auto Click:** switch on **Show countdown cross**, activate Auto Click, move > distance and stop. | An orange X appears at the pointer tip, grows outward and is about cursor-sized when the click happens, then disappears. It does not block the click (the item under the pointer is clicked) and never takes focus. |
| 10 | Move again before the click. | The X follows the pointer and starts small again. |

---

## Bug fix – v1.4.2

Build, Reinstall, re-grant the permissions if needed (BUILD.md 1.6.6), start the installed copy. Auto Click
tab: **Activate** and switch on **Show countdown cross**.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 12 test programs pass (`tst_autoclick`: 26 cases). |
| 2 | Move the mouse very slowly across the screen for a few seconds. | **No orange X** appears while moving (previously it flashed constantly). |
| 3 | Move more than the minimum distance and stop completely. | After about half the click time (e.g. 75 ms of a 150 ms click time) the X appears at the pointer tip, grows, and the click happens on time. |
| 4 | Set *Wait before clicking* to 1000 ms; move, stop, and touch the mouse slightly after ~0.7 s. | The X appears after 0.5 s and disappears when you touch the mouse. The click still happens at 1 s (tiny movements do not delay the click). |
| 5 | Repeat step 3 with the cross switched off. | The click happens as before; no X. |

---

## Stage 5 – v1.5.0

Goal: Function 3 – Break Timer.

**Preparation (macOS):** build, **Reinstall** and restart from the installed copy (BUILD.md 5.0); reset and
grant the permissions again if needed (BUILD.md 1.6.6, 6.1). Start with `open ~/Applications/ErgoPro2.app`.

> **Tip – short values for testing:** *Break after working* **5** min, *Reset after a pause of* **1** min,
> *Break length* **1** min, *Postpone by* **1** min. Set them back to your normal values afterwards.

### A. Tab and settings

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 13 test programs pass (adds `tst_breaktimer`, 21 cases). |
| 2 | Open **Break Timer**. | No banner; *Break reminders* switched on, badge **ACTIVE**; the four durations (defaults 50 / 5 / 5 / 10 min) and the full-screen option (off); Status panel. No "break now" or "reset" button. |
| 3 | Set the short test values (tip above). | `settings.json` → `breaktimer` section shows them. |
| 4 | Do not touch mouse or keyboard for a moment, then move the mouse. | State changes from **Idle** to **Working**; *Active work time* counts up every second; *Next break in* counts down from 05:00. |

### B. Breaks

| # | Steps | Expected result |
|---|-------|-----------------|
| 5 | Keep working (move the mouse / type now and then) for 5 min. | The break window covers **every** screen: semi-transparent, the main screen shows "Time to rest your wrists", a 01:00 countdown, five stretching tips with pictures, **Dismiss** and **Postpone 1 min**. Other screens show "Time for a break – see the main screen" and the countdown. State **On break**. |
| 6 | Press **Esc**, **Cmd+W**, **Cmd+Q**, click outside the card. | The window stays. Cmd+Tab still switches apps, but the break window stays on top. |
| 7 | Keep using the mouse during the break. | Nothing changes; the countdown continues. |
| 8 | Wait until the countdown reaches 00:00. | The window closes by itself; state **Idle**; the next activity starts a new work period from 00:00. |
| 9 | Work until the next break, click **Dismiss**. | The window closes immediately; work time restarts from 00:00. |
| 10 | Work until the next break, click **Postpone 1 min**, keep working. | The window closes; state **Postponed**; *Active work time* keeps counting (e.g. 05:30); after 1 min the window appears again. Postpone can be used again. |
| 11 | Postpone again and do not touch mouse/keyboard for 1 min (idle reset). | The postponed break is cancelled: state **Idle**, the window does not come back. |
| 12 | With Auto Click active, rest the pointer on **Dismiss**. | Auto Click clicks it (A-06). A Key Control click on **Postpone** also works. |
| 13 | Open a full-screen app (e.g. a video in full-screen, or a full-screen Safari window) and wait for a break. | The break window appears **above** the full-screen app. |
| 14 | Tray → **Start break now**. | The break window appears immediately; the entry is disabled while the break is shown. |
| 15 | Tray → **Break Timer on/off** (off) during a break. | The window closes; state **Disabled**; badge **INACTIVE**; no more breaks. Switching on again starts at **Idle**. The tab switch follows the tray entry and vice versa. |

### C. Idle, sleep and lock

| # | Steps | Expected result |
|---|-------|-----------------|
| 16 | Work 2 min, then leave mouse and keyboard alone for 1 min. | State **Idle**; work time reset (the pause counted as a break). |
| 17 | Work 2 min, put the Mac to sleep for more than 1 min, wake and unlock. | State **Idle**; work time reset. Log: `System is going to sleep` … `Break Timer: Working -> Idle`. |
| 18 | Work 2 min, lock the screen (Ctrl+Cmd+Q) for more than 1 min, unlock. | Same as 17 (typing the password does not count as work). |
| 19 | Work 4 min 50 s, lock the screen for 30 s (less than the idle reset), unlock. | The break window appears **after** unlocking, not behind the lock screen. |
| 20 | Change *Break after working* while Working (e.g. to a value below the current work time). | The break starts at once (changes apply to the running period). |

### D. Full-screen option and screens

| # | Steps | Expected result |
|---|-------|-----------------|
| 21 | Switch on **No breaks during full-screen apps**, play a video full-screen when the break is due. | *Next break in* shows "waiting (full-screen app)"; the break appears when you leave full-screen. |
| 22 | During a break, connect or disconnect an external display. | The overlay follows: every connected screen is covered. |
| 23 | Look at the break window in light and dark mode. | Readable and consistent with the app in both. |
| 24 | Debian (when available): the full-screen option is disabled with "Not available on this system…". Windows: steps 5–15, Alt+F4 does not close the window. | As described. |

---

## Stage 6 – v1.6.0

Goal: Drag Lock (Key Control action) and its interaction with Auto Click and the Break Timer.

**Preparation (macOS):** build, **Reinstall** and restart from the installed copy (BUILD.md 5.0); reset and
grant the permissions again if needed (BUILD.md 1.6.6, 6.1). Start with `open ~/Applications/ErgoPro2.app`.
Key Control → **Add Hotkey** → press e.g. **F8** → *Confirm* → action **Drag lock (left button)** → *Save*.
Keep the log open (General tab › *Open log folder*): every press and release is logged with its reason.

### A. Setup and indicators

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 14 test programs pass (adds `tst_draglock`). |
| 2 | Open the Add Hotkey dialog. | **Drag lock (left button)** is in the action list (unless it already has a hotkey). |
| 3 | Look at the Key Control tab. | New card **Drag lock** with *Drag lock timeout* (default **10 s**, range 1–60). Changing it is stored in `settings.json` → `draglock` → `DRAGLOCK_TIMEOUT`. |
| 4 | Show the ErgoPro2 window (Key Control tab) next to some empty desktop space. Point at the empty space and press **F8**. Look at the tray icon and the hotkey row without clicking. | Tray icon shows the drag-lock variant; the F8 row shows **HOLDING** with "Left button held…". Log: `Drag lock: left button held at (x, y)`. |
| 5 | Press **F8** again. | Button released; tray icon and row back to normal. Log: `… released at (x, y) - hotkey pressed again`. |

### B. Dragging (acceptance)

| # | Steps | Expected result |
|---|-------|-----------------|
| 6 | Finder: point at a file, **F8**, move the pointer onto a folder, **F8**. | The file is moved into the folder (the drag shows the usual file image while moving). |
| 7 | Same as 6, but hold **Option** while moving. | Modifiers do not release the drag: the file is **copied** (green +). |
| 8 | TextEdit / Safari: point before a word, **F8**, move across some text, **F8**. | The text is selected. |
| 9 | Point at a window's title bar, **F8**, move, **F8**. | The window moves with the pointer and stays where released. |
| 10 | Drag a slider or a scroll bar thumb with F8. | It follows the pointer. |

### C. Release conditions

| # | Steps | Expected result |
|---|-------|-----------------|
| 11 | **F8**, then do not move the mouse for 10 s. | Released after 10 s (log: `timeout without mouse movement`). Keep moving slowly for 20 s instead: it stays held; it is released 10 s after the last movement. |
| 12 | **F8** in TextEdit after selecting with the drag, then press a letter key (e.g. **A**). | Released; the letter **is typed** (the key is passed on). Log: `key pressed`. |
| 13 | **F8**, press and release **Shift**, **Ctrl**, **Option**, **Cmd** alone. | Still held (badge HOLDING). |
| 14 | **F8**, move a little, then **click** the left mouse button. | Released. The click itself does nothing else (e.g. a click on a button under the pointer does **not** press it; no double click). Log: `mouse click (consumed)`. |
| 15 | Same as 14 with the **right** button over a Finder item. | Released; **no** context menu opens. |
| 16 | Add a second hotkey (e.g. F9 = Left mouse click). **F8**, then **F9**. | The drag is released first, then F9 clicks. |
| 17 | Break Timer short values (Stage 5 tip). **F8** shortly before the break is due and keep moving. | When the break window appears the button is released (log: `break window shown`). |
| 18 | **F8**, then press **Cmd+Tab**. | Released by the Tab key (Cmd alone would not release). Note: disabling, modifying or deleting the hotkey and opening the hotkey dialog always need a click or a key press first, which already releases the drag; those extra release rules are safety nets covered by the code and the log (`drag-lock hotkey disabled, deleted or modified`). |
| 19 | **F8**, then quit ErgoPro2 with `killall ErgoPro2` in Terminal (SIGTERM). | The button is released before the app ends (test: click and drag something normally afterwards; no stuck button). |
| 20 | **F8**, then tray → **Exit** (that click itself releases). Repeat with **F8** and logout. | Never a stuck left button after ErgoPro2 has ended. |

### D. Auto Click interaction

| # | Steps | Expected result |
|---|-------|-----------------|
| 21 | Auto Click **on**. **F8**, drag a file far away, stop moving and wait 2 s. | No auto-click while held (it would drop the file early). |
| 22 | Release with **F8** and leave the pointer still. | No auto-click after the release. |
| 23 | Move the pointer by less than the *trigger distance* (default 15 px) and stop. | Still no auto-click. |
| 24 | Move further than the trigger distance and stop. | Auto Click clicks again as usual. |
| 25 | With *Show countdown cross* on, hold a drag lock and stop. | No orange X while held. |

### E. Other systems (when available)

| # | Steps | Expected result |
|---|-------|-----------------|
| 26 | Windows: steps 4–24 in Explorer / Notepad. | As described; a releasing click is not passed on. |
| 27 | Debian (X11): steps 4–24. | As described, **except** 14–15: the releasing click cannot be swallowed on Linux; a left click just ends the drag, a right click may also open a context menu (known limitation). |

---

## Bug fix – v1.6.1

Build, **Reinstall** and restart from the installed copy (BUILD.md 5.0); permissions granted.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 14 test programs pass (`tst_breaktimer`: 25 cases). |
| 2 | Key Control: have 5 hotkeys (one per action). | All 5 rows are visible, the list has **no** scroll bar of its own; "Every action already has a hotkey…" is shown in orange **above** the list, readable, not overlapping. *Add Hotkey* is disabled. |
| 3 | Make the window smaller (height and width). | A scroll bar appears on the page (right edge); scrolling shows every hotkey and the Tips card. Rows never overlap. |
| 4 | Delete one hotkey, add it again. | The warning disappears/appears again; the list shrinks/grows by one row. |
| 5 | Key Control tab. | No **Drag lock** card. `settings.json` still contains `draglock.DRAGLOCK_TIMEOUT`; changing it there (app closed) changes the drag-lock timeout. |
| 6 | Break Timer tab right after starting the app (permissions granted). | Switch, sliders and spin boxes are **enabled** and can be changed. |
| 7 | Break Timer tab layout. | Order: on/off card, **Status**, Settings. |
| 8 | General › *Reset all settings*, then Break Timer. | *Break length* = **4 min**. |
| 9 | Use mouse/keyboard for 1 min, then do nothing for 1 min, watch *Active work time*. | It counts while you work, keeps counting for 20 s after the last input, then stands still; *Next break in* stands still too. It continues with the next mouse move. |
| 10 | Short values (*Break after working* 5 min): work with pauses of ~1 min. | The break appears only after 5 min of **active** time, not 5 min of clock time. A pause of *Reset after a pause of* still resets the timer. |
| 11 | `settings.json`: set `"BREAKTIMER_ACTIVITY_PAUSE": 5` in `breaktimer` (app closed), restart. | The work time stops 5 s after the last input. Log: `work time stops after 5 s without input`. |

---

## Stage 7 – v1.7.0

Goal: Function 4 – Statistics.

**Preparation (macOS):** build, **Reinstall** and restart from the installed copy (BUILD.md 5.0); permissions
granted. Statistics folder: `~/Library/Application Support/techforge/ErgoPro2/statistics/` (General tab ›
*Open statistics folder*).

> **Tip – short values:** Break Timer › *Break after working* **5** min and *Break length* **1** min. Then
> 5 minutes of continuous use reach 100 %, Dismiss lets it grow beyond 100 %, and 1 minute without input resets
> it. Set your normal values back afterwards.

### A. Tab and live data

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 16 test programs pass (adds `tst_statistics`, `tst_statisticsstore`). |
| 2 | Open **Statistics** right after the first start. | No banner; "Today – <date>", "Live – updated every minute"; both charts show "No data for this day" (08:00–18:00); totals 0; *Today* and *Next day ›* disabled. |
| 3 | Use mouse and keyboard for 3 minutes, wait for the next full minute. | A line appears in *Computer usage*, bars in *Activity*; totals count up once per minute; the x axis covers the hours with data. |
| 4 | Point at the line and at a bar. | Tooltips: time + usage %, and the 5-minute values (clicks, key presses, distance). |
| 5 | Short values (tip): use the computer continuously for 5 min. | The line reaches the dashed **100 %** line; a ▲ marker shows where the break window appeared. |
| 6 | Click **Dismiss**, keep working 2 min. | ✕ marker; the line continues **above 100 %** (about 140 %). Postpone (◆) does the same. |
| 7 | Leave mouse and keyboard alone for 1 min (break length), then use them. | The line starts again from about 0 %. A break that ends by its countdown shows ✓. |
| 8 | Auto Click on: let it click 3 times; press a Key Control double-click hotkey once. | *Clicks done for you* +5; *Mouse clicks* counts only your own clicks. |
| 9 | Hold a key down for 3 s in a text editor. | *Key presses* +1 (repeats are not counted). |
| 10 | Move the mouse about 10 cm on the desk. | *Mouse distance* grows by roughly the distance the pointer moved on screen (approximate). |
| 11 | Lock the screen for 2 min, type the password, unlock. | No activity is recorded while locked; the usage starts again from 0 % (pause ≥ break length). |
| 12 | Switch the Break Timer **off**, use the computer. | The usage percent is still calculated. |

### B. Files

| # | Steps | Expected result |
|---|-------|-----------------|
| 13 | Open the folder, open today's `ergopro2-stats-YYYY-MM-DD.csv` in Numbers/Excel. | Header `time,clicks_left,…,usage_percent,events`; one row per minute with data; no key names or positions. |
| 14 | Exit ErgoPro2 and start it again within a minute. | Today's totals and chart continue (nothing lost; a minute written twice is merged). Log: `Statistics: continuing today with N stored minute(s)`. |
| 15 | With ErgoPro2 closed, append a broken line (e.g. `12:00,1,2`) without a line break to today's file; start ErgoPro2. | The line is removed (log WARNING "removed an incomplete last line"); the rest is shown. |
| 16 | Add a line `garbage` in the middle of a file and view that day. | The day is shown; the log names the skipped line. |

### C. History and retention

| # | Steps | Expected result |
|---|-------|-----------------|
| 17 | With ErgoPro2 closed, copy today's file to two older dates (e.g. `…-2026-11-01.csv`, `…-2026-11-05.csv`); start. | *‹ Previous day* shows 5 Nov, then 1 Nov (days without a file are skipped), then it is disabled. *Next day ›* goes forward; *Today* returns. Past days say "Saved statistics (read-only)". |
| 18 | Set **Keep statistics for** to 3 days. | Files older than 3 days (today counts) are deleted immediately; log `Statistics: deleted …`; navigation updates. |
| 19 | Leave ErgoPro2 running over midnight (or set the Mac clock forward). | A new file starts after midnight; the usage percent is not reset by midnight; old files beyond the retention are deleted. |

### D. Look and other systems

| # | Steps | Expected result |
|---|-------|-----------------|
| 20 | Light and dark mode; smallest window size. | Charts, markers and texts readable in both; the page scrolls, nothing is cut off. |
| 21 | Without Input Monitoring. | Banner "Statistics recording is unavailable"; saved days can still be viewed. |
| 22 | Windows / Debian (when available): steps 2–19. | As described (folder: `%APPDATA%\techforge\ErgoPro2\statistics` / `~/.local/share/techforge/ErgoPro2/statistics`). |

---

## Bug fix – v1.7.1

Build, **Reinstall** and restart from the installed copy (BUILD.md 5.0); permissions granted.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 16 test programs pass (`tst_breaktimer`: 26 cases). |
| 2 | With ErgoPro2 closed, copy today's statistics file to an old date (e.g. `ergopro2-stats-2026-10-01.csv`); start ErgoPro2. Statistics tab › *Keep statistics for*: move the slider down to **1** day. | The file is **not** deleted (check the folder); the hint says "Older files are deleted when ErgoPro2 starts and once a day." Log: `Statistics retention set to 1 day(s); applied at the next start or day change`. |
| 3 | Move the slider back to 90 and restart ErgoPro2. | The old file is still there (within 90 days) – nothing was lost by the mistake. |
| 4 | Set it to 1 day and restart. | Now the old file is deleted at startup (log `Statistics: deleted … (startup)`). |
| 5 | Same as 2–4 for General › *Keep log files for* with an old log file (`ergopro2-2026-10-01.log`). | Not deleted when the slider moves; deleted at the next start (or the next day) when it is outside the retention. |
| 6 | Break Timer: work 1 min, then leave mouse and keyboard alone. | After 20 s *State* changes from **Working** to **Paused** (active work time and next break stand still); the next mouse move shows **Working** again. After *Reset after a pause of* it shows **Idle**. |
| 7 | Postpone a break and do nothing for 30 s. | *State* stays **Postponed**. |

---

## Bug fix – v1.7.2

Packages from the CI: push, wait until the CI run on GitHub is green, open the run (Actions tab) and
download the three files under **Artifacts**.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | macOS: download `ErgoPro2-1.7.2-macos.dmg`, double-click it. | It opens (no "unsupported format"); the window shows **ErgoPro2**. If macOS refuses to open the app: BUILD.md 1.6.5. |
| 2 | Double-click ErgoPro2 in that window. | The app starts; the General tab warns that it runs from a disk image and offers **Install**. Version 1.7.2. |
| 3 | Click **Install**, restart from `~/Applications/ErgoPro2.app`, grant the permissions (BUILD.md 6.1). | Everything works as before (Auto Click, hotkeys, Break Timer, Statistics). |
| 4 | Windows 11: download `ErgoPro2-1.7.2-windows-x64.zip`, right-click › *Extract All*, open the `ErgoPro2` folder. | About 10–15 files: `ErgoPro2.exe`, `Qt6*.dll`, `msvcp140*.dll` / `vcruntime140*.dll`, folders `platforms`, `imageformats`, `iconengines` – no `translations`, no `opengl32sw.dll`, no `D3Dcompiler_47.dll`. |
| 5 | Double-click `ErgoPro2.exe` (SmartScreen: *More info › Run anyway*), also on a PC without Visual Studio or the VC++ Redistributable. | The app starts, icons are visible (tabs, tray). |
| 6 | Debian 13: `sudo apt install ./ergopro2_1.7.2_amd64.deb`, start ErgoPro2 from the menu. | Installs with its dependencies; the app starts with icons. `sudo apt remove ergopro2` removes it. |

---

## Bug fix – v1.7.3

macOS: install the 1.7.3 build (BUILD.md 5.0) and start it from `~/Applications`. Set the log level to
"… + Debug" (General tab) to see the notifications.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 17 test programs pass, including `tst_macpowerevents`. |
| 2 | Lock the screen with **Ctrl+Cmd+Q**, wait 1 min, unlock. Repeat 3 times. | ErgoPro2 keeps running (tray icon still there). Log: `Screen locked` / `Screen unlocked`, no `CRITICAL`. |
| 3 | Leave the Mac alone until it locks / the display sleeps by itself (as in the bug report), then unlock. | ErgoPro2 still runs; Break Timer shows **Idle** after the pause; Statistics continue. |
| 4 | Apple menu › **Sleep**, wake after 1 min. | Log: `System is going to sleep` … `System resumed from sleep`, `Reinstalling the input hook: resume from sleep`; app keeps running, hotkeys work. |
| 5 | Leave ErgoPro2 running overnight with the lid closed. | Next morning it is still running; no `CRITICAL` in the log. |

---

## Bug fix – v1.7.4

Release and Windows setup program. Push to `main`; wait for the green CI run.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 17 test programs pass. |
| 2 | GitHub › **Releases**. | Release **ErgoPro2_v1.7.4** (tag v1.7.4) with the 1.7.4 CHANGELOG text, a download table and 3 files: `.dmg`, `-setup.exe`, `.deb`. |
| 3 | Push another commit to `main` without changing the version. | No second release; the release job says "already exists". |
| 4 | Windows 11: run the setup (SmartScreen: *More info › Run anyway*). | No administrator prompt. The folder page proposes `C:\Users\<you>\AppData\Local\Programs\ErgoPro2`; options *start at login* (on) and *desktop icon* (off). After *Finish* ErgoPro2 starts. |
| 5 | General tab. | "✅ Installed at …\Programs\ErgoPro2, starts at login. Installed with the ErgoPro2 setup program." *Install* is disabled. |
| 6 | Start Menu and Windows Settings › Apps. | Entry **ErgoPro2** in both. |
| 7 | With ErgoPro2 running, run the setup again (or a newer one). | No folder page (same folder is used); ErgoPro2 closes by itself (log: `Exit requested by "ErgoPro2 --quit"`), is updated and starts again. Settings, hotkeys and statistics are unchanged. |
| 8 | First installation into another folder (e.g. `D:\Tools\ErgoPro2`), then run a newer setup. | The update goes to `D:\Tools\ErgoPro2` without asking. The General tab shows that folder. |
| 9 | Run an old portable `ErgoPro2.exe` (zip, before 1.7.4), then the setup. | The old copy is ended (it does not know `--quit`); the setup continues. |
| 10 | General tab › **Uninstall** › Yes. | ErgoPro2 exits, the uninstaller removes the folder, Start Menu entry, desktop icon and start at login. `%APPDATA%\techforge\ErgoPro2` (settings, logs, statistics) is still there. |
| 11 | macOS/Linux: `ErgoPro2 --quit` while ErgoPro2 runs. | ErgoPro2 exits normally (statistics saved); the command returns 0. With nothing running it also returns 0. |

---

## Bug fix – v1.7.5

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 17 test programs pass (`tst_breaktimer`: 27 cases). |
| 2 | Break Timer › Status right after starting ErgoPro2, without touching mouse/keyboard. | Four fields: *Active work time*, **Work started at**, *Next break in*, *State*. *Work started at* shows "–" (State *Idle*). |
| 3 | Move the mouse at e.g. 14:05:12. | *Work started at* shows **14:05:12** and keeps that value while you work, during short pauses (*Paused*), during a break and a postpone. |
| 4 | Do nothing for *Reset after a pause of* (e.g. 1 min with the short test values). | State *Idle*, *Work started at* "–". The next input shows the new start time. |
| 5 | Switch Break reminders off. | *Work started at* "–". |
| 6 | On every tab with sliders (General, Break Timer, Auto Click, Statistics): put the pointer on a slider and on its number box and turn the mouse wheel / scroll on the trackpad. | The value does **not** change; the page scrolls (if it is longer than the window). |
| 7 | Drag a slider, click on the slider track, type a value into the box, click its arrows. | The value changes as before and is saved. |
| 7a | General tab: scroll with the mouse wheel / trackpad over *Started at login*, *Started manually* and *Log level*; Key Control › Add Hotkey › action list (closed). | The selection does **not** change; the page scrolls. Clicking a box opens the list and a value can be chosen as before (scrolling inside the open list works). |
| 8 | General › Input diagnostics › Show. | Only the live information (hook, pointer, last key/button, synthetic, events per second, watchdog); no "Test left click" or "Test: block F9" buttons. |

---

## Bug fix – v1.7.6

macOS (installed 1.7.6). Short values help: *Break after working* 10 min, *Break length* 3 min.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 17 test programs pass. |
| 2 | Work until the Statistics usage shows about 50 %, then change *Break after working* (e.g. 10 → 7 min, then 10 → 15 min). | The usage % on the Statistics tab does **not** jump (the next minutes continue at the old rate). Log: `usage 100 % = 10 min of use (setting: 7 min, applies when the usage restarts at 0)`. |
| 3 | Set *Break after working* below the current *Active work time* (Break Timer tab). | The break window appears immediately. |
| 4 | Let the break window close by itself (countdown), even while moving the mouse during the break. | Usage goes back to **0 %** (log `break completed - usage reset to 0 %`); the next minutes use the new *Break after working* as 100 %. |
| 5 | Work until a break, click **Dismiss** while the usage is above 100 % (or keep working past 100 %). | Log `usage above 100 % - break after …`; *Next break in* shows **half** of *Break after working* (e.g. 05:00 for 10 min); the break comes after half the time. After a completed break (usage 0 %) the full time applies again. |
| 6 | Break Timer tab › *Break length* slider. | Range 3–15 min. In `settings.json` set `BREAKTIMER_BREAK_TIME` to 1 (app closed), start: the value is 4 (log names the invalid value). |
| 7 | Break Timer and Auto Click tabs: hints under the sliders. | Only descriptions, no `BREAKTIMER_…` / `AUTOCLICK_…` names. |

---

## Bug fix – v1.7.7

> The short test values used in earlier sections (5 / 1 / 1 min) are no longer possible. Shortest values now:
> *Break after working* **20** min, *Reset after a pause of* **2** min, *Break length* **3** min, *Postpone by* **3** min.

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 17 test programs pass. |
| 2 | Break Timer tab: drag each slider to both ends, type values outside the range into the boxes. | *Break after working* 20–120, *Reset after a pause of* 2–15, *Postpone by* 3–20 (values outside cannot be entered). |
| 3 | General › *Reset all settings*, then Break Timer. | 50 / 5 / 4 (break length) / 10 min. |
| 4 | With ErgoPro2 closed, set in `settings.json` › `breaktimer`: `BREAKTIMER_MAX_WORK_TIME` 10, `BREAKTIMER_IDLE_RESET` 30, `BREAKTIMER_POSTPONE_TIME` 1; start ErgoPro2. | The tab shows 50 / 5 / 10; the log has a WARNING "Invalid value for …" for each of the three. |

## Bug fix – v1.7.8

| # | Steps | Expected result |
|---|-------|-----------------|
| 1 | `ctest --test-dir build --output-on-failure` | All 18 test programs pass. |
| 2 | macOS: ErgoPro2 running with Accessibility and Input Monitoring granted (Key Control hotkey working). System Settings › Privacy & Security › Accessibility: switch ErgoPro2 **off**. | Mouse and keyboard keep working everywhere (at most a 1-second hiccup). Within a few seconds the function tabs show the "permission missing" banner. Log: CRITICAL "Accessibility permission was removed while the event tap was active…" and "Input hook stopped by itself…". |
| 3 | Continue: switch ErgoPro2 **on** again in Accessibility. | Within a few seconds the banners disappear and the hook runs again (General › Input diagnostics: hook running, events counting; hotkeys work). No restart needed (if macOS asks for one, *Restart app*). |
| 4 | macOS: repeat 2 with only **Input Monitoring** removed (Accessibility on). | No freeze; banners appear; the hook starts again after granting it again. |
| 5 | Break after working = 20 min. Work normally (type, read with short pauses) until the break window appears; open Statistics. | The usage line reaches about 100 % at the minute the break window appeared (not ~70 % as before). Active work time and usage % grow together. |
| 6 | With usage at e.g. 30 %, change *Break after working* 20 → 40 min. | The usage % does not jump. After the next active-work-time reset (e.g. pause of 5 min, or Dismiss of a break) the percent is shown on the 40-min scale (log: "Statistics: usage 100 % = 40 min (was 20 min) because the active work time was reset"). |
| 7 | Same change while the Break Timer tab shows *Active work time* 00:00. | The new scale applies at once (log line "…because the active work time is 0"). |
| 8 | Break after working = 20 min; dismiss every break and keep working (about 100 min of active work in total) until the usage passes 500 %. | At just above 500 % the usage drops to 0 % (log: "usage above 500 % - reset to 0 %"); the next break comes after the full 20 min again (not 10). The chart axis never goes above 500 %. |
| 9 | Windows / macOS / Debian: look at the window's title bar. | No maximize button (Windows), green button without zoom / full screen (macOS). Double-clicking the title bar does not maximize. Win+Up (Windows) / window-manager maximize (Linux) does not maximize either (the window returns to its normal size). |
| 10 | Drag the window's corner to make it as large as possible. | It stops at 80 % of the screen's usable width and height. It can still be made smaller down to 720 × 560 (less on a small screen). |
| 11 | Move the window to a second screen with a different resolution (or change the resolution). | The limits follow the new screen (log: "Main window size limits for screen …"); a too-large window shrinks to 80 % of it. |
