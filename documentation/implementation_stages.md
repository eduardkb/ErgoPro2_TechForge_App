# ErgoPro2 – Implementation Stages

This document drives the implementation of ErgoPro2 by an AI coding tool, one stage at a time.

## How to use this document

1. Open an AI coding tool at the **repository root** (`ergoPro2/`), so it can read and write the project files.
2. Copy the prompt of the **next stage** from Part C (the full content of the `text` block) and paste it into the AI tool.
3. When the AI finishes, review the changes, build and test manually using `documentation/BUILD.md` and `documentation/MANUAL_TESTS.md`.
4. If you find bugs, use the prompt in `documentation/ai_bugFix.md` (this bumps the patch version, e.g. `1.3.0` → `1.3.1`).
5. Commit the stage yourself (the AI never commits), then continue with the next stage.

Every stage prompt tells the AI to read this document. **Part A (rules)** and **Part B (functional specification)** are the same for all stages, so they are not repeated inside each prompt.

## Stage overview

| Stage | Version | Content |
|-------|---------|---------|
| 0 | 1.0.0 | Project skeleton, build system, CI, logging, settings, main window with tabs, tray, single instance, look & feel, `BUILD.md` |
| 1 | 1.1.0 | General tab: install/uninstall + autostart, startup behavior, logging options, folders, reset settings, version, permissions/diagnostics, macOS permission window |
| 2 | 1.2.0 | Platform input layer: global mouse/keyboard monitoring, synthetic click injection, key suppression, health watchdog (macOS, Windows, Linux X11 + Wayland best effort) |
| 3 | 1.3.0 | AutoClick (Function 1) |
| 4 | 1.4.0 | Key Control (Function 2) – hotkeys and click actions |
| 5 | 1.5.0 | Break Timer (Function 3) |
| 6 | 1.6.0 | Drag Lock action + AutoClick interaction rules |
| 7 | 1.7.0 | Hardening, packaging (`.dmg`, Windows zip, `.deb`), final documentation |
| 8 | – | Statistics (Function 4) – not planned yet, no prompt |

> Order note: the original plan was reordered as requested (P-01). AutoClick and Key Control do not depend on the Break Timer. Drag Lock depends on all three, so it comes after them.

---

# Part A – Project rules (apply to every stage and every bug fix)

### A.1 Source documents and precedence
- `documentation/baseDescription.md` – original description.
- `documentation/ai_questions.md` – questions **with the owner's answers** (`Answer:` and `REVIEW_RESULT:` lines).
- `documentation/implementation_stages.md` (this file) – Part B is the **consolidated specification**.
- **Precedence when they conflict:** Part B of this file > answers in `ai_questions.md` > `baseDescription.md`.
- If something is still ambiguous, **stop and ask the user** instead of guessing. Do not invent features.

### A.2 Technology
- C++20, **Qt 6.8 LTS or newer**, **Qt Widgets** (no QML), **CMake** (≥ 3.21).
- Qt licensed under LGPL: link Qt **dynamically**.
- Unit tests with **Qt Test**, run through **CTest**.
- Native APIs only where Qt has no equivalent, and only inside `src/platform/<os>/`.
  - macOS: Objective-C++ (`.mm`), CoreGraphics (`CGEventTap`, `CGEventPost`), ApplicationServices (`AXIsProcessTrusted`), IOKit (`IOHIDCheckAccess`), ServiceManagement.
  - Windows: Win32 (`SetWindowsHookEx` `WH_KEYBOARD_LL` / `WH_MOUSE_LL`, `SendInput`, registry).
  - Linux: X11 (`XRecord` for monitoring, `XTest` for injection, `XGrabKey` for exclusive keys). For Wayland, best effort through `libevdev` + `/dev/uinput` (see B.1.4).
- Target OSes: Windows 11, macOS 26 Tahoe (universal binary arm64 + x86_64 when built with the official Qt installer), Debian 13.

### A.3 Repository layout
```
ergoPro2/
├── CMakeLists.txt            # single source of the version number
├── CHANGELOG.md
├── .github/workflows/ci.yml
├── documentation/            # all documentation (BUILD.md, MANUAL_TESTS.md, ...)
├── resources/                # icons, images (SVG/PNG), stylesheet (.qss), .qrc
├── src/
│   ├── main.cpp
│   ├── core/                 # platform-independent logic (no OS headers, no widgets)
│   ├── platform/             # interfaces + factory
│   │   ├── mac/
│   │   ├── win/
│   │   └── linux/
│   └── ui/                   # Qt Widgets: main window, tabs, dialogs, break window, tray
└── tests/                    # Qt Test unit tests for src/core
```
- `core` must not include widget or OS headers. Time must come from an **injectable clock interface**, so state machines can be unit-tested without real waiting.
- `platform` exposes abstract interfaces (e.g. `IInputHook`, `IClickInjector`, `IAutostart`, `IInstaller`, `IPermissions`, `IPowerEvents`) and a factory that picks the implementation for the current OS at compile time. No `#ifdef` outside `src/platform/` and the factory.
- Platform implementations that are not done yet must still compile and return a clear "not supported on this OS yet" status (logged and shown in the UI), never crash.

### A.4 Versioning
- Format `MAJOR.MINOR.PATCH`. **MAJOR is always 1.**
- **Each stage bumps MINOR and resets PATCH to 0** (Stage N ⇒ `1.N.0`, e.g. from `1.3.2` a new stage goes to `1.4.0`).
- **Each bug-fix run bumps PATCH** (`1.3.0` → `1.3.1`).
- The **single source of truth** is `project(ErgoPro2 VERSION x.y.z ...)` in the root `CMakeLists.txt`. A generated header (`configure_file`) exposes it to the code. The version is shown in the window title, on the General tab and in the tray tooltip, and is set with `QCoreApplication::setApplicationVersion`.
- Every version change adds an entry at the top of `CHANGELOG.md` (Keep a Changelog style: `## [1.N.0] - YYYY-MM-DD` with `Added / Changed / Fixed`).

### A.5 Coding rules
- **No error is ever silently ignored.** Every failed OS call, file operation or unexpected state is logged with context (function, OS error code/message). Errors relevant to the user are also shown in the UI (message box or tray notification).
- No `catch (...) {}` without logging. Install a top-level handler for uncaught exceptions that logs as `CRITICAL` before exit.
- Every user-visible string is wrapped in `tr()` (English only for now).
- Threads: worker objects moved to `QThread`, communication with signals/slots (queued connections). No UI access from worker threads.
- Every parameter change is applied immediately and saved to the settings JSON (debounced 300 ms, atomic write).
- Keep code readable: small classes, clear names, a short comment on every non-obvious OS call.
- No sound is ever played by the application.

### A.6 Testing
- Every stage adds/updates unit tests for its `core` logic. `ctest` must pass before the stage is reported as done.
- The project must build **without warnings** that matter (enable `-Wall -Wextra` / `/W4`) on macOS locally. Windows and Linux are verified through CI (GitHub Actions).
- Every stage appends a section `## Stage N – vX.Y.0` to `documentation/MANUAL_TESTS.md` with a numbered manual test checklist (steps + expected result) for the new functionality.

### A.7 Workflow rules for the AI
- **Do not run `git commit`, `git push` or create branches.** The user reviews and commits.
- Implement **only** the current stage. Do not start features of later stages (leave clean extension points instead).
- Before starting, check that the version in `CMakeLists.txt` is `1.(N-1).x`. If it is not, stop and tell the user.
- At the end, build the project and run the tests, then reply with:
  1. Summary of what was implemented.
  2. List of created/modified files.
  3. Build and test results (copy the relevant output).
  4. Known limitations / anything not verified (e.g. Windows/Linux only compile-checked by CI).
  5. Pointer to the new section in `MANUAL_TESTS.md`.
  6. Any open question for the user.

---

# Part B – Consolidated functional specification

## B.1 General application

### B.1.1 Window and look & feel
- Main window with top tabs, in this order: **General**, **Break Timer**, **Auto Click**, **Key Control**, **Statistics**.
- Modern look: Qt Fusion style + application stylesheet (`resources/style.qss`), card-like grouped sections with rounded corners, consistent spacing, one accent color, modern toggle-switch style for on/off states, clear "Active / Inactive" status badges. Must look good in **both light and dark OS mode** (follow the OS color scheme).
- Every tab shows a small icon next to its name (accent color; black on the selected tab, in both themes – 1.4.1).
- App and tray icons: create simple, clean SVG icons (a stylized hand/wrist or mouse). Tray icon variants: normal, **AutoClick active**, **Drag Lock active** (if easy to implement on the OS; not mandatory for AutoClick per A-09).
- Statistics tab: placeholder "Coming soon".
- Numeric parameters: **slider + spin box** side by side with units, applied immediately.

### B.1.2 Tray, close and exit
- Closing (X) or minimizing the window **hides it to the tray**. The first time, a tray notification says "ErgoPro2 is still running in the tray".
- The app **exits only** through the **Exit** button on the General tab or **Exit** in the tray menu. No confirmation.
- Tray menu: *Show window*, *AutoClick on/off*, *Break Timer on/off*, *Start break now*, *Exit*. (No global "pause all features": rejected in W-04.)
- Left click/double click on the tray icon shows the window.
- macOS: Dock icon visible only while the window is visible; menu bar icon only while hidden (switch `NSApplication` activation policy Regular ↔ Accessory).
- Single instance: a second launch brings the running instance's window to the front and exits.

### B.1.3 General tab
- **Install / Uninstall** buttons (W-06, W-07):
  - *Install*: copies the application to a permanent per-user folder and registers it to **start at login**, pointing to the installed copy. No admin rights where possible; if admin rights are ever needed, ask the user first.
    - macOS: copy `.app` to `~/Applications/ErgoPro2.app`; login item via `SMAppService` (fallback LaunchAgent plist in `~/Library/LaunchAgents/com.techforge.ergopro2.plist`).
    - Windows: copy the whole deployed folder to `%LOCALAPPDATA%\Programs\ErgoPro2\`; `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` value; Start Menu shortcut.
    - Linux: copy to `~/.local/opt/ErgoPro2/`; `~/.config/autostart/ergopro2.desktop` and `~/.local/share/applications/ergopro2.desktop`.
    - After install, offer to restart from the installed location.
  - *Uninstall*: removes autostart entry, shortcuts and the installed copy. If the running executable is the installed copy, schedule the deletion for after exit (and exit), with a clear message. Settings and logs are kept (state this in the message).
  - Status label: "Installed at <path>, starts at login" / "Not installed – running from <path>".
  - If not installed and running from a temporary or Downloads folder, show a warning.
  - The autostart command passes `--autostart` so the app knows how it was started.
- **Startup behavior** (W-02): two options
  - "When started automatically at login": *Hide in tray* (default) / *Show window*.
  - "When started manually": *Show window* (default) / *Hide in tray*.
- **Logging**: level selector (cumulative, as requested in S-06), log retention days (1–180), buttons *Open log folder*, *Open settings folder*.
- **Reset all settings to defaults** (with confirmation; hotkey list is cleared too and its OS bindings released).
- **Application version**.
- **Permissions / Diagnostics** section: OS + version, session type (Linux X11/Wayland), status of each required permission (Granted / Missing) with a button that opens the matching system settings page, status of the input hook (running / failed + reason).
- **Exit** button.

### B.1.4 OS permissions and feature availability
- A central `FeatureAvailability` service decides, for each feature (AutoClick, Key Control, Break Timer activity monitoring), whether it is **available** and, if not, **why** and **how to fix it**.
- **Every function tab shows a clear banner at the top** when its feature is unavailable: what is missing, why, and a button to fix it (open settings / grant permission / retry). Controls of an unavailable feature are disabled.
- **macOS**: at startup, if Accessibility or Input Monitoring is missing, show a **modal permissions window** explaining each permission, with buttons to open the right System Settings pane and a *Check again* button. The window can be closed (the app keeps running), but then the affected tabs show the "unavailable" banner. Re-check permissions automatically every few seconds while the window/banners are shown, so no restart is needed when possible. If macOS requires a restart for the permission to apply, say so and offer a *Restart app* button.
- **Linux X11**: full support through X11 APIs.
- **Linux Wayland**: best effort (REVIEW_RESULT 1.1). Use `libevdev` to read `/dev/input/event*` and `/dev/uinput` to inject clicks and grab keys. This requires the user to be in the `input` group (or a udev rule). The app detects missing access and offers to fix it by running the needed command through `pkexec` (asks for the admin password with the system dialog; never handle passwords in the app), then tells the user to log out/in. Features that remain impossible are clearly marked unavailable on their own tab. If Wayland support turns out impossible for a feature, explain this on that tab.
- **Windows**: no special permissions. Note in the diagnostics that hooks cannot click into windows of apps running as Administrator unless ErgoPro2 also runs elevated.

## B.2 Settings (JSON)
- File: `QStandardPaths::AppConfigLocation` / `settings.json` (organization `techforge`, application `ErgoPro2`, bundle id `com.techforge.ergopro2`).
- Contains `schemaVersion`. Each value is validated (type + range); invalid values fall back to the default and a warning is logged. A file that cannot be parsed is renamed `settings.corrupt-<timestamp>.json`, defaults are used, and the user is notified.
- Atomic write (`QSaveFile`), debounced 300 ms.
- All states persist (enabled/disabled of features, hotkey list, window options).

| Key | Range | Default | Stage |
|-----|-------|---------|-------|
| `general.startupAutoMode` | `tray` / `window` | `tray` | 1 |
| `general.startupManualMode` | `window` / `tray` | `window` | 1 |
| `general.logLevel` | see B.3 | `info_warning_error_critical` | 0 |
| `general.logRetentionDays` | 1–180 | 30 | 0 |
| `general.trayHintShown` | bool | false | 0 |
| `autoclick.enabled` | bool | false | 3 |
| `AUTOCLICK_TRIGGER_TIMER` (ms) | 100–3000 | **200** | 3 |
| `AUTOCLICK_TRIGGER_DISTANCE` (logical px) | 5–200 | 15 | 3 |
| `autoclick.jitterTolerancePx` | 1–10 | 3 | 3 |
| `autoclick.typingSuspendMs` | fixed 500 (constant) | 500 | 3 |
| `autoclick.showVisualFeedback` | bool | false | 3 |
| `keycontrol.actions` | list (see B.5) | empty | 4 |
| `DRAGLOCK_TIMEOUT` (s) | 1–60 | 10 | 6 |
| `breaktimer.enabled` | bool | true | 5 |
| `BREAKTIMER_IDLE_RESET` (min) | 1–30 | 5 | 5 |
| `BREAKTIMER_MAX_WORK_TIME` (min) | 5–180 | 50 | 5 |
| `BREAKTIMER_BREAK_TIME` (min) | 1–30 | 5 | 5 |
| `BREAKTIMER_POSTPONE_TIME` (min) | 1–60 | 10 | 5 |
| `breaktimer.suppressWhenFullscreen` | bool | false | 5 |

Group the keys in the JSON by section (`general`, `autoclick`, `keycontrol`, `draglock`, `breaktimer`). Each stage adds only its own keys (and their schema/validation).

## B.3 Logging
- Levels: DEBUG, INFO, WARNING, ERROR, CRITICAL.
- User selection on the General tab (cumulative, S-06):
  1. Info only
  2. Info + Warning
  3. Info + Warning + Error
  4. Info + Warning + Error + Critical (**default**)
  5. Info + Warning + Error + Critical + Debug
- Files in `QStandardPaths::AppDataLocation/logs/`, one file per day (`ergopro2-YYYY-MM-DD.log`). If a file gets larger than 5 MB, roll over to `...-1.log`, `...-2.log`. Files older than `general.logRetentionDays` are deleted at startup and every day at midnight.
- Line format: `2026-11-09 14:33:01.123 [LEVEL] [thread] [module] message`.
- Qt messages (`qDebug`/`qWarning`/…) are redirected through `qInstallMessageHandler`.
- Thread-safe. In debug builds, also print to the console.

## B.4 Function 1 – AutoClick
- Runs in its **own thread** and gets mouse/keyboard events from the shared input hook (B.6).
- Tab: top **status badge** ("AutoClick is ACTIVE" / "INACTIVE") + **Activate/Deactivate** button; parameters below.
- Algorithm (A-01 – A-05):
  1. **Armed** when the pointer is at a straight-line distance ≥ `AUTOCLICK_TRIGGER_DISTANCE` (logical px) from the **last reference point** (position of the last click, manual or automatic, or the position where AutoClick was enabled).
  2. While armed, every movement greater than `jitterTolerancePx` restarts the stop timer. Smaller movements are ignored (tremor).
  3. When there has been no movement (beyond the tolerance) for `AUTOCLICK_TRIGGER_TIMER` ms → **one left click** at the current position → reference point = click position → **disarmed**.
  4. A **manual mouse click** (any button) while armed cancels the pending click, disarms, and sets the reference point to that position. A click produced by **Key Control** behaves the same as a manual click.
  5. No auto-click while any mouse button is held down (manual drag).
  6. No auto-click within 500 ms after any key press (typing).
  7. While **Drag Lock** is active: no auto-click. After drag-lock release, the reference point = release position (B.5.3).
- Works **everywhere**, including the ErgoPro2 window and the break window (A-06, REVIEW 1.6). No app exclusion list.
- Synthetic clicks are **tagged** (B.6) so they are never treated as manual clicks or as user activity.
- Optional visual feedback (S-12, A-08, off by default): an orange X centered on the pointer tip that grows outward until the click, up to about the size of the mouse cursor (changed in 1.4.1; was a ring). It appears only after the pointer has made no movement at all – not even tremor below the tolerance – for half the trigger time, and any movement hides it again (1.4.2); the click timing is not affected. Click-through, never takes focus.
- Tray icon shows the active state if easily possible.
- No sound, ever.

## B.5 Function 2 – Key Control

### B.5.1 Bindings
- Only **keyboard** keys/combinations (no mouse buttons).
- Allowed: F1–F24, numpad keys, and any key combined with ≥ 1 modifier. A plain alphanumeric/symbol key without a modifier shows a warning ("this key will no longer type in other applications") and requires confirmation.
- **Rejected with explanation** (REVIEW 1.3): combinations the current OS never lets applications capture, or that would break the system. Keep the list per OS in one place, for example:
  - Windows: `Ctrl+Alt+Del`, `Win+L`, `Ctrl+Shift+Esc`, `Alt+Tab`, `Win+Tab`, any `Win+…` reserved by the shell, Fn.
  - macOS: Fn/Globe, `Cmd+Tab`, `Cmd+Space` (if used by Spotlight), `Cmd+Option+Esc`, `Ctrl+Cmd+Q`, power/media keys.
  - Linux: `Ctrl+Alt+Del`, `Ctrl+Alt+F1…F12` (VT switching), `Ctrl+Alt+Backspace`.
  - Also reject keys reserved by the OS that registration reports as failing.
- One key combination ↔ at most one action (K-08), and one action ↔ at most one hotkey (owner request, 1.4.1): the Add/Modify dialog offers only actions that have no hotkey yet (plus the hotkey's own action when modifying). Disabled hotkeys keep their action; deleting the hotkey frees it. When every action has a hotkey, Add Hotkey is disabled with an explanation. Stored duplicates are shown with an Error status.
- Bindings are **exclusive**: while an enabled action is bound, the OS and other apps do **not** receive that key combination (swallowed by the low-level hook).
- Fire **once per key press**, on key-down, no auto-repeat (K-10).
- Hotkeys also work while the break window is shown (K-13) and on the ErgoPro2 window.
- While the **Add/Modify Hotkey** dialog is open, all actions are paused (K-14) so the user can press any key.

### B.5.2 Actions list (tab)
- **Add Hotkey** flow (K-15): dialog "Press the key or key combination…" (Cancel) → shows the captured combination (e.g. `Ctrl+Shift+F9`) with *Confirm / Retry* → choose action from a dropdown → *Save* → register; on failure show a clear error and keep the dialog open to choose another key.
- List columns: Key combination | Action | Status (Active / Disabled / Error + reason) | buttons **Enable/Disable**, **Modify** (key and/or action), **Reset binding**, **Delete** (with confirmation).
  - *Disable*: releases the key back to the OS (K-03).
  - *Reset binding*: unregisters and immediately re-registers the key with the OS (a "repair" action) (K-02a).
- Available actions (pluggable design, one class per action type, registered in an action registry):
  1. Left mouse click
  2. Right mouse click
  3. Double left mouse click (interval = OS double-click time)
  4. Drag lock (left button) – **Stage 6**
  5. Toggle AutoClick on/off (S-10 review)
- All clicks happen at the current pointer position.
- Stored in settings as a list: `{ id (uuid), keyCombo (portable string, e.g. QKeySequence::PortableText), nativeInfo (optional), action (string id), enabled (bool) }`.

### B.5.3 Drag Lock (Stage 6)
- Press hotkey → left button **down** (held) at the current position.
- Released (button **up** at the current position) when any of these happens:
  - the drag-lock hotkey is pressed again (swallowed),
  - `DRAGLOCK_TIMEOUT` seconds pass **without mouse movement** (D-02),
  - any **non-modifier** keyboard key is pressed (modifiers alone do not release; the key press is passed on normally),
  - any mouse button is clicked (that click is **consumed**: it only releases the drag) (D-03),
  - the break window appears (D-06),
  - the app exits or AutoClick/Key Control is disabled (safety: never leave the button stuck down).
- While active: AutoClick does not click. After release, AutoClick's reference point = release position, so the pointer must travel `AUTOCLICK_TRIGGER_DISTANCE` again before the next auto-click (D-04).
- Tray icon shows drag-lock active (D-05).

### B.5.4 Binding robustness (REVIEW 1.4)
- One shared input hook (no thread per action). Keys stay exclusive over hours/days:
  - macOS: handle `kCGEventTapDisabledByTimeout` / `kCGEventTapDisabledByUserInput` by re-enabling the tap immediately (and logging it). Keep the callback very fast.
  - Windows: keep the `WH_KEYBOARD_LL` callback very fast (hand work to other threads), because Windows silently removes slow hooks. Run a **health watchdog** that detects a dead hook and reinstalls it.
  - Linux: re-grab after keyboard mapping changes (`MappingNotify`) and after device hotplug (evdev).
  - All OSes: re-install hooks after **sleep/resume** and **session unlock**, and log every re-install.
  - Health check every 30 s; failures are logged and shown in the Status column / tab banner.

## B.6 Shared input layer (Stage 2)
- One **input hook thread** that receives all global keyboard and mouse events (move, button down/up, wheel, key down/up with modifiers), normalized into platform-independent event structs with a monotonic timestamp.
- Distributes events to subscribers (AutoClick, Key Control, Break Timer activity monitor) through queued signals. Key Control can decide synchronously and quickly whether a key event is **swallowed**.
- **Click injector**: left/right down/up/click, double click (OS double-click interval).
- **Synthetic event tagging**: every injected event is marked (macOS: `kCGEventSourceUserData`; Windows: `dwExtraInfo` magic value; X11/uinput: track pending injected events) and flagged `synthetic = true` in the normalized event, so the app never re-processes its own events and the Break Timer never counts them as activity.
- Coordinates in **logical pixels**, multi-monitor aware (HiDPI correct).
- **Power/session events**: sleep, resume, screen lock, unlock → signals (used by the watchdog and Break Timer).

## B.7 Function 3 – Break Timer
- Tab: top **toggle** Break Timer enabled/disabled (on by default) + status badge; parameters; at the bottom a live status panel updated **every second**: active work time (`INTERACTION_TIMER`, mm:ss), time until next break, current state (*Working*, *Idle*, *On break*, *Postponed*, *Disabled*). No "break now"/"reset" buttons on the tab (B-04). (*Start break now* exists only in the tray menu.)
- **INTERACTION_TIMER = wall-clock time** since the start of the current work cycle (B-01 a). The cycle starts at the first real user activity after a reset.
- Activity = real mouse movement, clicks, wheel, key presses. Synthetic events excluded (B-02).
- **Idle reset**: if there is no activity for ≥ `BREAKTIMER_IDLE_RESET` minutes, the timer resets (counts as a break taken) (B-03). Sleep and screen lock count as idle (B-16).
- When INTERACTION_TIMER ≥ `BREAKTIMER_MAX_WORK_TIME` → show the **break window**.
- **Break window** (B-05, B-10):
  - Full-screen semi-transparent overlay on **all monitors**, always on top, not closable with Esc/Alt+F4/Cmd+W. The OS task switcher is not blocked.
  - Shows: message (e.g. "Time to rest your wrists"), **countdown mm:ss** of `BREAKTIMER_BREAK_TIME`, **fixed stretching tips with images** (create 4–6 simple SVG illustrations in `resources/`, e.g. wrist flexor stretch, wrist extensor stretch, fist-open-fan, shoulder rolls, look away 20 s), and buttons **Dismiss** and **Postpone (N min)**.
  - **Countdown reaches zero** → window closes automatically, INTERACTION_TIMER resets (B-06).
  - **Dismiss** → closes immediately and resets INTERACTION_TIMER (no confirmation, no limit) (B-07).
  - **Postpone** → closes and reopens after `BREAKTIMER_POSTPONE_TIME` minutes. The timer keeps counting; unlimited postpones (B-08). If the user is idle ≥ `BREAKTIMER_IDLE_RESET` during the postpone, the postponed break is cancelled and INTERACTION_TIMER resets (B-09).
  - Activity during the break does nothing (countdown continues) (B-13).
  - AutoClick and hotkeys keep working on the break window (A-06, K-13). An active Drag Lock is released when it appears (D-06).
  - No pre-warning, no sound (B-11, B-12).
- **Suppress when a full-screen app is in the foreground** (B-17): optional setting, off by default. When on and a full-screen app is active, the break is delayed until it is no longer full-screen (best effort per OS; if detection is unsupported on an OS, disable the option with an explanation).
- Disabling the Break Timer closes any break window and resets the timer.
- Changing `BREAKTIMER_MAX_WORK_TIME` takes effect immediately on the current cycle.

## B.8 Function 4 – Statistics
- Not implemented. Placeholder tab only. Design events (clicks, breaks) so a statistics collector can subscribe later, but do not store anything (W-09).

---

# Part C – Stage prompts

Copy the full content of one `text` block into the AI tool.

## Stage 0 – Project skeleton (v1.0.0)

```text
You are implementing STAGE 0 of the ErgoPro2 desktop application (C++20 / Qt 6 Widgets / CMake), cross-platform: Windows 11, macOS 26 Tahoe, Debian 13.

FIRST read completely:
- documentation/baseDescription.md
- documentation/ai_questions.md (the "Answer:" and "REVIEW_RESULT:" lines are the owner's decisions)
- documentation/implementation_stages.md (Part A = mandatory rules, Part B = consolidated specification; Part B wins on conflicts)

This is the first stage: the repository contains only the documentation folder. Create the project from scratch.

GOAL: a runnable, good-looking application shell with all infrastructure in place, plus build documentation so the owner can build and test every following stage manually.

IMPLEMENT:
1. Repository layout exactly as in Part A.3. Root CMakeLists.txt with project(ErgoPro2 VERSION 1.0.0 ...), C++20, warnings enabled, Qt 6.8+ (Widgets, Network for QLocalServer, Test), configure_file version header, macOS bundle settings (bundle id com.techforge.ergopro2, Info.plist with NSAccessibilityUsageDescription-style descriptions where applicable), Windows WIN32 executable with icon/resource file, Linux .desktop file. CTest enabled.
2. .gitignore for build folders and IDE files.
3. Logging module (Part B.3): levels, cumulative user-selectable level setting, daily files + 5 MB roll-over, retention cleanup by days, qInstallMessageHandler redirect, thread-safe, console output in debug builds, top-level handler for uncaught exceptions/terminate that logs CRITICAL.
4. Settings module (Part B.2): JSON in AppConfigLocation, schemaVersion, typed accessors, validation with min/max and defaults, corrupt-file backup and user notification, atomic debounced save (QSaveFile, 300 ms), change signals. In this stage register only the "general.*" keys listed for stage 0; design it so later stages add their keys in one place each.
5. Injectable clock interface in src/core (real clock + fake clock for tests).
6. Platform layer skeleton (Part A.3/B.6): abstract interfaces (IInputHook, IClickInjector, IAutostart, IInstaller, IPermissions, IPowerEvents) and an OS factory. In this stage the implementations are stubs that report "not implemented yet" (logged) – no crash.
7. Main window with the 5 tabs in order General, Break Timer, Auto Click, Key Control, Statistics. Placeholder content in each tab ("Coming in a later version"), Statistics shows "Coming soon". General tab in this stage: app version, log level selector, log retention days (1–180, slider + spin box), Exit button. Window title shows "ErgoPro2 v<version>".
8. Modern look & feel (Part B.1.1): Fusion style, resources/style.qss, light and dark mode following the OS, reusable widgets for "card" sections, toggle switch, status badge, slider+spinbox parameter row with unit label and immediate apply. Create SVG app icon and tray icons (normal, autoclick-active, draglock-active) in resources/ and wire them via a .qrc. Generate platform icon files where the build needs them (.icns / .ico) or document how they are generated.
9. Tray (Part B.1.2): tray icon with menu (Show window, Exit; other menu entries are added in later stages – leave an extension point), close/minimize hides to tray with one-time notification, exit only via Exit button or tray Exit. macOS Dock icon only while the window is visible (activation policy switch in src/platform/mac).
10. Single instance (QLockFile + QLocalServer): second launch activates the running window and exits.
11. Command-line flag --autostart parsed (behavior is used in stage 1).
12. Unit tests (tests/): settings load/save/validation/corrupt file, logging level filtering and retention cleanup (using temp dirs), clock fake.
13. CI: .github/workflows/ci.yml building and running ctest on macos-latest, windows-latest (install Qt 6.8 via jurplel/install-qt-action) and a debian:trixie container (Qt 6 from apt). Upload build artifacts.
14. documentation/BUILD.md – detailed, step-by-step for a non-expert, for EACH OS (macOS, Windows 11, Debian 13):
    a. Prerequisites and how to install them (compiler/Xcode command line tools / Visual Studio Build Tools, CMake, Ninja, Qt 6.8 – official online installer and alternatives: Homebrew on macOS (note: native arch only, no universal binary), apt packages on Debian).
    b. Configure, build (Debug and Release), run the unit tests.
    c. How to run the built executable directly.
    d. How to produce a self-contained, distributable build: macdeployqt (+ .dmg via hdiutil), windeployqt (zip folder), Debian .deb via CPack (or document it as coming in stage 7 if not yet configured – the run-from-build-folder method must work now).
    e. macOS code signing (REVIEW 1.2): ad-hoc signing (codesign --force --deep --sign - ErgoPro2.app), verification commands, and the OPTIONAL but recommended creation of a free self-signed "code signing" certificate in Keychain Access so macOS keeps Accessibility/Input Monitoring permissions across rebuilds; how to reset permissions with tccutil when they get stuck. Gatekeeper/quarantine (xattr -dr com.apple.quarantine) for personal use.
    f. Where settings and logs are stored on each OS.
    g. A "Starting automatically at login" section explaining that this will be done by the app's Install button (stage 1), plus the manual method per OS (LaunchAgent / Login Items, HKCU Run key / Startup folder, ~/.config/autostart .desktop) and why a system "service" is not appropriate for a GUI app.
    h. Troubleshooting section.
    i. A short "How to test each stage" section pointing to documentation/MANUAL_TESTS.md.
15. documentation/MANUAL_TESTS.md with the "Stage 0 – v1.0.0" checklist.
16. CHANGELOG.md with the 1.0.0 entry. A short README.md at the root (what the app is, link to BUILD.md).

OUT OF SCOPE: autostart/install, permissions, input hooks, AutoClick, Key Control, Break Timer logic.

ACCEPTANCE: builds and runs on macOS; tabs, tray, hide-to-tray, single instance, exit, log level and retention settings persist across restarts; settings file and log files created in the documented locations; ctest passes; CI workflow is valid.

Follow all workflow rules in Part A.7 (no git commits; final report format).
```

## Stage 1 – General tab, install/autostart, permissions (v1.1.0)

```text
You are implementing STAGE 1 of ErgoPro2. Current version must be 1.0.x.

FIRST read completely: documentation/baseDescription.md, documentation/ai_questions.md (owner's Answer/REVIEW_RESULT lines), documentation/implementation_stages.md (Part A rules, Part B specification – Part B wins on conflicts). Then study the existing code before changing it.

GOAL: complete General tab, install/uninstall with start-at-login on all three OSes, startup behavior, permission detection and the macOS permissions window, and the feature-availability framework used by the next stages.

IMPLEMENT:
1. Version: bump to 1.1.0 (CMakeLists.txt) and add the CHANGELOG entry.
2. IInstaller + IAutostart implementations for macOS, Windows and Linux exactly as in Part B.1.3 (copy to permanent per-user folder, register login start pointing to the installed copy with --autostart, Start Menu/.desktop entries, uninstall incl. deferred deletion when running from the installed copy). No admin rights; if any step would need them, ask the user first. Every failure is logged and shown to the user.
3. General tab, complete (Part B.1.3): Install / Uninstall buttons + status label, warning when running from a temporary/Downloads folder, startup behavior options (automatic start / manual start), log level + retention (already there), Open log folder, Open settings folder, Reset all settings to defaults (confirmation), version, Permissions/Diagnostics section, Exit.
4. Apply the startup behavior at launch using the --autostart flag.
5. IPermissions implementations:
   - macOS: Accessibility (AXIsProcessTrusted / AXIsProcessTrustedWithOptions) and Input Monitoring (IOHIDCheckAccess / IOHIDRequestAccess); buttons opening the exact System Settings panes (x-apple.systempreferences URLs).
   - Linux: detect X11 vs Wayland (XDG_SESSION_TYPE, WAYLAND_DISPLAY), and for Wayland check read access to /dev/input/event* and write access to /dev/uinput; offer a fix through pkexec (add user to the "input" group and/or install a udev rule for uinput), explaining that a log out/in is needed. Never handle passwords inside the app.
   - Windows: report "no special permission needed" plus the elevated-apps note.
6. FeatureAvailability service (Part B.1.4) with per-feature status {available, reason, fixAction}. Add a reusable "unavailable" banner widget and place it at the top of the Break Timer, Auto Click and Key Control tabs (the tabs still have placeholder content). Banner updates live when the status changes (periodic re-check every 3 s while something is missing).
7. macOS startup permissions window (REVIEW 1.2): modal, explains each permission and why ErgoPro2 needs it, buttons to open settings, "Check again", "Restart app" when needed. If closed without granting, the banners explain what is unavailable and how to fix it.
8. Tray tooltip shows the version.
9. Unit tests for any core logic added (e.g. temp/Downloads folder detection, availability rules, startup-mode decision).
10. Update documentation/BUILD.md (install/uninstall via the app, autostart verification per OS, permission granting steps with the exact System Settings paths on macOS Tahoe, Wayland input group/udev steps) and add "Stage 1 – v1.1.0" to documentation/MANUAL_TESTS.md.

OUT OF SCOPE: the input hook itself, AutoClick, Key Control, Break Timer logic.

ACCEPTANCE (macOS manual + CI for others): Install copies the app to ~/Applications and it starts at login hidden in the tray; Uninstall removes it; startup options work; permission window appears when permissions are missing and the banners are shown on the three function tabs; diagnostics show correct statuses; ctest passes.

Follow all workflow rules in Part A.7 (no git commits; final report format).
```

## Stage 2 – Platform input layer (v1.2.0)

```text
You are implementing STAGE 2 of ErgoPro2. Current version must be 1.1.x.

FIRST read completely: documentation/baseDescription.md, documentation/ai_questions.md (owner's Answer/REVIEW_RESULT lines), documentation/implementation_stages.md (Part A rules, Part B specification – especially B.1.4, B.5.4 and B.6). Then study the existing code.

GOAL: a robust, shared global input layer on every OS that later stages (AutoClick, Key Control, Break Timer) build on. No end-user feature yet, but a diagnostics view to test it manually.

IMPLEMENT:
1. Version 1.2.0 + CHANGELOG entry.
2. Normalized input event model in src/core (mouse move, button down/up with button id, wheel, key down/up with key code + modifiers + isAutoRepeat, synthetic flag, monotonic timestamp, logical-pixel position with screen info).
3. IInputHook implementations running on ONE dedicated thread with its own OS event loop:
   - macOS: CGEventTap (session level, head insert) for mouse + keyboard on a CFRunLoop in the hook thread; re-enable on kCGEventTapDisabledByTimeout/ByUserInput.
   - Windows: WH_KEYBOARD_LL + WH_MOUSE_LL on a thread with a message loop; callbacks must return fast.
   - Linux X11: XRecord for monitoring; XGrabKey for exclusive keys (with all NumLock/CapsLock modifier variants).
   - Linux Wayland: best effort with libevdev (read /dev/input/event*, with device hotplug via udev or inotify) and uinput (virtual device for injection). If access is missing, report through FeatureAvailability; do not crash. Make libevdev an optional CMake dependency so the build works without it (feature then reported unavailable).
4. Key suppression API: a subscriber (Key Control in stage 4) can register a fast, thread-safe "should swallow this key event?" predicate based on a set of registered key combos. Implement the swallow mechanism on each OS (drop event in the tap / return 1 from the LL hook / XGrabKey / EVIOCGRAB + re-emit non-bound keys through uinput on Wayland). Provide register/unregister/re-register functions per key combo with clear error results (e.g. combo already grabbed by another app on X11).
5. IClickInjector: left/right down/up/click and double click using the OS double-click interval (NSEvent.doubleClickInterval / GetDoubleClickTime / X11 or default 400 ms), at the current pointer position. Every injected event is tagged as synthetic (Part B.6) and comes back from the hook with synthetic=true.
6. IPowerEvents: sleep, resume, screen lock, unlock signals per OS.
7. Health watchdog (B.5.4): every 30 s check that hooks are alive; reinstall after failure, after resume and after unlock; log every reinstall; report persistent failures through FeatureAvailability.
8. Input dispatcher in src/core that forwards events to subscribers with queued signals (subscribers will run in their own threads).
9. Diagnostics UI on the General tab (collapsible "Input diagnostics" card): hook status, live pointer position, last key event (with modifiers), last mouse button event, synthetic flag, counter of events/s, a "Test left click in 3 s" button and a "Test: swallow F9 for 30 s" button (to verify suppression manually), and the watchdog reinstall count.
10. Performance: hook callbacks must never block (no logging I/O, no locks held for long inside callbacks – use a lock-free queue or a fast mutex + condition).
11. Unit tests for the core parts (event normalization helpers, dispatcher, key-combo matching incl. modifiers, watchdog logic with fake clock).
12. Update BUILD.md (new dependencies: X11/XTest/XRecord dev packages, optional libevdev; macOS frameworks) and add "Stage 2 – v1.2.0" to MANUAL_TESTS.md (including a long-running test: leave the app running for 2+ hours and through sleep/resume, then verify the hook and the F9 swallow test still work).

OUT OF SCOPE: AutoClick, Key Control UI, Break Timer logic.

ACCEPTANCE: on macOS with permissions granted, diagnostics show live events; test click works and is flagged synthetic; F9 is swallowed system-wide during the test; hooks recover after sleep/resume; Windows/Linux compile in CI; ctest passes.

Follow all workflow rules in Part A.7 (no git commits; final report format).
```

## Stage 3 – AutoClick (v1.3.0)

```text
You are implementing STAGE 3 of ErgoPro2: Function 1 – AutoClick. Current version must be 1.2.x.

FIRST read completely: documentation/baseDescription.md, documentation/ai_questions.md (owner's Answer/REVIEW_RESULT lines), documentation/implementation_stages.md (Part A rules, Part B specification – especially B.4 and B.6). Then study the existing code, especially the input layer from stage 2.

GOAL: a reliable dwell-click feature exactly as specified in Part B.4.

IMPLEMENT:
1. Version 1.3.0 + CHANGELOG entry.
2. Settings keys for AutoClick (Part B.2) with validation.
3. AutoClick engine in src/core as a pure state machine (states such as Disabled, WaitingForMovement/Disarmed, Armed, Countdown, Suppressed) driven by input events and the injectable clock, implementing every rule of Part B.4 (distance from reference point, jitter tolerance, trigger timer, manual click cancels and moves the reference point, no click while a button is held, 500 ms after key presses, hook for Drag Lock suppression and "reset reference point to position X" to be used in stage 6, Key Control clicks treated like manual clicks – expose a method for that).
4. AutoClick worker running in its OWN thread (QThread), subscribing to the input dispatcher, using IClickInjector. It ignores synthetic events except to recognise its own click.
5. Auto Click tab: unavailable banner (from stage 1) at the top, then a large status badge "AutoClick is ACTIVE/INACTIVE" + Activate/Deactivate button, parameter rows (trigger timer 100–3000 ms, trigger distance 5–200 px, jitter tolerance 1–10 px, visual feedback on/off) with short help texts.
6. Optional visual feedback overlay (B.4): click-through, frameless, translucent ring near the cursor that fills during the countdown; never steals focus; off by default. If it is hard to make reliable on some OS, disable the option there with an explanation.
7. Tray: menu entry "AutoClick on/off" (checkable) and tray icon variant while active (if easy on the OS).
8. State persists (enabled + parameters). Parameter changes apply immediately to the running engine.
9. AutoClick works on all windows, including the ErgoPro2 window (A-06).
10. If AutoClick is unavailable (permissions/Wayland), the controls are disabled and the banner explains why.
11. Unit tests for the state machine covering every rule with the fake clock (arming distance, jitter, timer, cancel by manual click, held button, typing suspension, drag-lock suppression hooks, reference point reset, parameter change while armed).
12. Add "Stage 3 – v1.3.0" to MANUAL_TESTS.md.

OUT OF SCOPE: Key Control, Drag Lock action, Break Timer.

ACCEPTANCE: with AutoClick active, moving the pointer more than the distance and stopping clicks once after the trigger time; resting the mouse does not produce repeated clicks; typing and manual clicks behave as specified; the toggle and tray entry work; settings persist; ctest passes; CI green.

Follow all workflow rules in Part A.7 (no git commits; final report format).
```

## Stage 4 – Key Control (v1.4.0)

```text
You are implementing STAGE 4 of ErgoPro2: Function 2 – Key Control (without Drag Lock, which is stage 6). Current version must be 1.3.x.

FIRST read completely: documentation/baseDescription.md, documentation/ai_questions.md (owner's Answer/REVIEW_RESULT lines), documentation/implementation_stages.md (Part A rules, Part B specification – especially B.5.1, B.5.2, B.5.4 and B.6). Then study the existing code, especially the input layer (stage 2) and AutoClick (stage 3).

GOAL: user-defined, exclusive global hotkeys that trigger mouse actions, robust over long periods.

IMPLEMENT:
1. Version 1.4.0 + CHANGELOG entry.
2. Settings key keycontrol.actions (Part B.5.2 structure) with validation; invalid entries are logged and shown with an Error status, not dropped silently.
3. Key combo model in src/core: portable representation, display text per OS (e.g. ⌘⇧F9 on macOS, Ctrl+Shift+F9 elsewhere), modifier normalization (left/right), validation rules of B.5.1 (allowed keys, warning for plain alphanumeric keys, per-OS list of never-capturable/reserved combos rejected with a clear explanation, duplicate detection).
4. Pluggable action system: IKeyAction interface + registry. Implement: Left click, Right click, Double left click, Toggle AutoClick. Leave Drag Lock as a registered-but-hidden placeholder ready for stage 6 (or add it in stage 6 – your choice, but no Drag Lock behavior now).
5. Key Control service: registers enabled bindings with the input layer's suppression API, fires actions once per key press on key-down (ignore auto-repeat, swallow the repeats and the key-up too), clicks at the current pointer position via IClickInjector, notifies AutoClick that a Key-Control click happened (treated like a manual click). Hotkeys work while the break window is shown (stage 5) and on the ErgoPro2 window.
6. Pause all bindings while the Add/Modify dialog is open (K-14) and restore them afterwards.
7. Robustness (B.5.4): bindings are re-registered after watchdog hook reinstalls, resume and unlock; per-binding status (Active / Disabled / Error + reason) updates live.
8. Key Control tab: unavailable banner, "Add Hotkey" button, list/table of actions with Enable/Disable, Modify, Reset binding, Delete (with confirmation) exactly as in B.5.2; Add/Modify dialog flow of K-15 (press combo → Confirm/Retry → choose action → Save → register; on failure show the reason and let the user pick another combo).
9. Behavior details: Disable releases the key to the OS; Reset binding unregisters and re-registers; Modify can change key and/or action; Delete releases and removes.
10. Unit tests: key combo parsing/normalization/validation (including reserved combos per OS), duplicate detection, action registry, service logic with a fake input layer (fire once per press, auto-repeat ignored, pause during capture, re-registration after hook reinstall).
11. Add "Stage 4 – v1.4.0" to MANUAL_TESTS.md (include: verify the bound key no longer reaches other apps, e.g. a text editor; verify a disabled binding types normally again; verify bindings still work after 2+ hours and after sleep/resume).

OUT OF SCOPE: Drag Lock behavior, Break Timer.

ACCEPTANCE: hotkeys added through the UI perform their clicks system-wide, the key is not received by other applications, Toggle AutoClick works, all list operations work and persist; reserved combos are rejected with explanations; ctest passes; CI green.

Follow all workflow rules in Part A.7 (no git commits; final report format).
```

## Stage 5 – Break Timer (v1.5.0)

```text
You are implementing STAGE 5 of ErgoPro2: Function 3 – Break Timer. Current version must be 1.4.x.

FIRST read completely: documentation/baseDescription.md, documentation/ai_questions.md (owner's Answer/REVIEW_RESULT lines), documentation/implementation_stages.md (Part A rules, Part B specification – especially B.7, plus B.4/B.5 interactions). Then study the existing code.

GOAL: break reminders based on real user activity, with a full-screen break window, exactly as in Part B.7.

IMPLEMENT:
1. Version 1.5.0 + CHANGELOG entry.
2. Settings keys for the Break Timer (Part B.2) with validation.
3. Break Timer engine in src/core as a pure state machine driven by activity events, power/lock events and the injectable clock: states Disabled, WaitingForActivity, Working, BreakShown, Postponed; INTERACTION_TIMER as wall-clock time since the first activity of the cycle; idle reset after BREAKTIMER_IDLE_RESET minutes without activity (sleep/lock count as idle, including time passed while the machine was asleep); break when INTERACTION_TIMER ≥ BREAKTIMER_MAX_WORK_TIME; auto-close after BREAKTIMER_BREAK_TIME with reset; Dismiss resets; Postpone reopens after BREAKTIMER_POSTPONE_TIME while the timer keeps counting; idle ≥ BREAKTIMER_IDLE_RESET during postpone cancels it and resets; activity during the break changes nothing; parameter changes apply immediately; disabling closes the window and resets.
4. Activity monitor subscribing to the input dispatcher: only real (non-synthetic) mouse movement, clicks, wheel and key presses count.
5. Break window (B.7): full-screen semi-transparent overlay on ALL screens (one window per screen, handle screens added/removed), always on top, cannot be closed with Esc/Alt+F4/Cmd+W/Cmd+Q-to-window, does not block the OS task switcher. Main screen shows message, mm:ss countdown, stretching tips with images, Dismiss and Postpone (N min) buttons. Create 4–6 clean SVG stretching illustrations in resources/ with a short caption each. Use platform code where needed to keep it above full-screen apps and other always-on-top windows (macOS window level + collection behavior to appear on all Spaces and over full-screen apps; Windows HWND_TOPMOST; X11 _NET_WM_STATE_ABOVE/fullscreen hints). Matches the app's modern look in light and dark mode. Clicks from AutoClick and Key Control work on it.
6. Optional "Do not show breaks while a full-screen application is active" setting (off by default) with OS-specific best-effort detection; if not supported on an OS, the option is disabled with an explanation.
7. Break Timer tab: unavailable banner (activity monitoring needs the input hook), top enable toggle + status badge, parameter rows (idle reset, max work time, break time, postpone time, full-screen suppression), live status panel at the bottom updated every second (active work time, time until next break, state). No break-now or reset buttons on this tab.
8. Tray menu: "Break Timer on/off" (checkable) and "Start break now".
9. Provide a hook/signal "break window shown" that stage 6 will use to release Drag Lock.
10. Unit tests for the state machine covering every rule with the fake clock (idle reset, sleep/lock, threshold, auto close, dismiss, postpone, idle during postpone, parameter change mid-cycle, disable/enable, synthetic events ignored).
11. Add "Stage 5 – v1.5.0" to MANUAL_TESTS.md (tip: set short values, e.g. max work 5 min, break 1 min, idle reset 1 min, postpone 1 min, for manual testing).

OUT OF SCOPE: Drag Lock, Statistics.

ACCEPTANCE: breaks appear after the configured work time on all monitors, cannot be dismissed except via the buttons or the countdown, idle and sleep reset the timer, postpone works, tab status updates live; ctest passes; CI green.

Follow all workflow rules in Part A.7 (no git commits; final report format).
```

## Stage 6 – Drag Lock (v1.6.0)

```text
You are implementing STAGE 6 of ErgoPro2: the Drag Lock action of Key Control and its interaction with AutoClick and the Break Timer. Current version must be 1.5.x.

FIRST read completely: documentation/baseDescription.md, documentation/ai_questions.md (owner's Answer/REVIEW_RESULT lines, especially the "Function 2" Drag Lock description and D-01..D-06), documentation/implementation_stages.md (Part A rules, Part B specification – especially B.5.3, B.4 rule 7 and B.7). Then study the existing code (input layer, AutoClick engine, Key Control service, Break Timer).

GOAL: a safe drag-lock that never leaves the mouse button stuck, fully integrated with AutoClick.

IMPLEMENT:
1. Version 1.6.0 + CHANGELOG entry.
2. Setting DRAGLOCK_TIMEOUT (1–60 s, default 10) with validation; parameter row on the Key Control tab ("Drag lock timeout").
3. Drag Lock state machine in src/core (Idle, Holding) with the injectable clock, implementing exactly B.5.3: press hotkey → left button down at the current position; release (left button up at the current position) on: hotkey pressed again (swallowed), timeout measured since the last mouse movement, any non-modifier key press (passed through to the OS), any mouse button click (consumed – swallowed by the hook, only releases), break window shown, app exit, Key Control disabled/binding disabled/deleted/modified, hook reinstall failure. Only one drag lock can be active at a time.
4. Mouse-click consumption: extend the input layer's API so the drag-lock can swallow the next real mouse button down/up pair on every OS (macOS tap, Windows WH_MOUSE_LL, X11/evdev). Do this cleanly and only while Holding.
5. While Holding, mouse movement must produce a proper drag on every OS (on macOS, moving with a synthetic held button may need left-mouse-dragged events to be posted instead of mouse-moved – handle it so drag-and-drop and text selection work).
6. AutoClick integration (B.4 rule 7, D-04): no auto-click while Holding and none on release; on release the AutoClick reference point is set to the release position so the pointer must travel AUTOCLICK_TRIGGER_DISTANCE before the next auto-click.
7. Break Timer integration: release when the break window appears (D-06).
8. Indicators: tray icon "drag-lock active" variant (D-05) and the status in the Key Control list row.
9. Safety: on any unexpected error, crash handler or exit, send left button up if Holding (log it).
10. Make "Drag lock (left button)" selectable in the Add/Modify Hotkey dialog.
11. Unit tests for the state machine with the fake clock (every release condition, modifiers not releasing, timeout reset by movement, consumed click, AutoClick reference point update, break release).
12. Add "Stage 6 – v1.6.0" to MANUAL_TESTS.md (drag a file to a folder, select text, drag a window, each release condition, AutoClick not clicking after release until the distance is moved).

OUT OF SCOPE: Statistics, packaging.

ACCEPTANCE: drag-lock works for drag-and-drop and text selection on macOS, all release conditions work, the button is never left stuck, AutoClick rules hold; ctest passes; CI green.

Follow all workflow rules in Part A.7 (no git commits; final report format).
```

## Stage 7 – Hardening, packaging and final documentation (v1.7.0)

```text
You are implementing STAGE 7 of ErgoPro2: hardening, packaging and final documentation. Current version must be 1.6.x.

FIRST read completely: documentation/baseDescription.md, documentation/ai_questions.md (owner's Answer/REVIEW_RESULT lines), documentation/implementation_stages.md (Part A rules and the full Part B specification), documentation/BUILD.md, documentation/MANUAL_TESTS.md and CHANGELOG.md. Then review the whole codebase.

GOAL: a release-quality 1.7.0 build for all three OSes with complete documentation.

IMPLEMENT:
1. Version 1.7.0 + CHANGELOG entry.
2. Requirements audit: create documentation/REQUIREMENTS_TRACE.md – a table mapping every requirement in baseDescription.md, every answered question in ai_questions.md and every item in Part B to the implementing file(s)/class(es), the unit test(s) and the manual test(s). Mark gaps clearly. Fix every gap that is small and within the existing scope; list the others for the owner (do not add new features).
3. Code review pass: error handling (no silent failures), thread safety (hook callbacks, shutdown order), resource cleanup on exit (hooks removed, keys released, mouse button never left down), memory leaks, log noise at INFO level, consistent UI texts and styling, light/dark mode check of every window.
4. Cross-platform review of the Windows and Linux (X11 and Wayland) implementations against the macOS behavior; fix discrepancies found by reading the code and by CI. Make CI run the full test suite on all three OSes and publish packaged artifacts.
5. Packaging:
   - macOS: Release build, macdeployqt, ad-hoc signing (and optional self-signed identity via a CMake/script parameter), .dmg creation script.
   - Windows: windeployqt, zip of the folder (optional: CPack NSIS/WiX installer if simple).
   - Debian: CPack .deb with correct dependencies, .desktop file, icon, and post-install note about the input group for Wayland.
   - Scripts in a scripts/ folder (package-macos.sh, package-windows.ps1, package-linux.sh), all documented.
6. Final documentation:
   - documentation/BUILD.md fully updated (build, package, install, run, autostart, signing, permissions, troubleshooting) for each OS.
   - documentation/USER_GUIDE.md: end-user guide for every tab and feature, including OS permission instructions and limitations (e.g. Wayland, elevated Windows apps, reserved key combos).
   - documentation/ARCHITECTURE.md: modules, threads, event flow, how to add a new Key Control action, where statistics will hook in later.
   - MANUAL_TESTS.md: "Stage 7 – v1.7.0" full regression checklist across all features (and a long-run/sleep-resume test).
7. Make sure ctest passes and the app builds without relevant warnings.

OUT OF SCOPE: Statistics (Function 4), new features.

ACCEPTANCE: packages produced by the scripts install and run on macOS (manually verified by the owner) and are built by CI for Windows and Debian; trace document shows no unexplained gaps; documentation complete.

Follow all workflow rules in Part A.7 (no git commits; final report format).
```

## Stage 8 – Statistics (future)

No prompt yet. Function 4 is out of scope for now (baseDescription.md). When it is time to implement it, write the requirements first (as was done with `ai_questions.md`), and then add a Stage 8 prompt here that bumps the version to `1.8.0`.
