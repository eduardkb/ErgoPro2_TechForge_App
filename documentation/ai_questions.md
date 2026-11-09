# ErgoPro2 – Improvement Suggestions & Questions for AI Implementation

This document reviews `baseDescription.md` and lists the questions and suggestions that should be settled **before** implementation starts. The whole application will be built by an AI tool, so anything ambiguous will be resolved by guessing. These questions are meant to remove those guesses.

**How to answer:** Each question has an ID (e.g. `G-03`) and, where useful, a **Suggested default** in *italics*. Write your answer under `Answer:`. If you agree with the suggested default, writing "default" or "OK" is enough.

---

## Part 1 – Critical topics (please read first)

These items have the biggest impact on architecture, effort or feasibility.

### 1.1 Linux: Wayland vs X11 (feasibility risk)
Debian 12/13 with GNOME uses **Wayland** by default. Wayland deliberately blocks applications from:
- reading the global mouse position and keyboard events outside their own window,
- injecting synthetic mouse clicks,
- grabbing global hotkeys exclusively.

This means **AutoClick, Key Control and the Break Timer activity monitor cannot work as described under Wayland** without workarounds (running under X11/XWayland session, using `/dev/uinput` + `libevdev` with root/`input` group permissions, or desktop-specific portals that do not support everything needed).

REVIEW_RESULT: implemnt as best as possible asking for root permissions wherever needed for the functionalities. if not possible at all, clearly state each unavailable functionality on its own function tab.

### 1.2 macOS permissions (Accessibility / Input Monitoring)
On macOS Tahoe, global input monitoring, key suppression and synthetic clicks need **Accessibility** and **Input Monitoring** permissions granted by the user in System Settings. Unsigned builds lose these permissions every time the binary changes (each rebuild), which makes testing annoying. Code signing (even with a free/personal certificate) helps a lot.
REVIEW_RESULT: when starting applciation on MacOS, open a window forcing user to give these permissions. if not given, on each affected functionality tab write that it is unavailable and why (instructing user to give permissions). On documentation on how to install the app in MacOS, make sure to add this code signing procedure.

### 1.3 "Exclusive" key binding
Making a key combination exclusive (the OS and other apps do not receive it) is possible, but the method is different on each OS:
- **Windows:** `RegisterHotKey` (exclusive by design) or a low-level keyboard hook (`WH_KEYBOARD_LL`) that swallows events.
- **macOS:** `CGEventTap` that drops the event (needs permissions from 1.2).
- **Linux X11:** `XGrabKey`. **Wayland:** not possible in general (see 1.1).
Some combinations can **never** be captured (e.g. `Ctrl+Alt+Del` on Windows, some macOS system shortcuts such as `Cmd+Tab` in some cases, the Fn key).

REVIEW_RESULT: for the events that can never be captured, do not allow it to be assigned and tell the user why this combination can't be used. Idenify the OS being used and act accordingly. wherever possible make low-level OS assignments or what works best for that OS.

### 1.4 "One thread per action" (Key Control)
Global keyboard hooks on every OS deliver events to **one** hook/callback (usually on one dedicated thread with an event loop). Creating one OS-level hook per action is not normal practice. It adds complexity and the risk of race conditions, and gives no benefit.

**Suggestion:** use a single "Input Hook" thread that receives all keyboard and mouse events and dispatches them to an action executor. AutoClick and the Break Timer activity monitor use the same thread. "Reset binding" would then unregister and re-register that specific key with the OS, which gives the same result you want from rebuilding a thread.
REVIEW_RESULT: Ok. Do as suggested for simplicity. but make sure this exclusive key bindings are not lost every other hour because of other processes or OS behavior.

### 1.5 AutoClick trigger timer range (5–500 ms)
Typical "dwell click" tools use **300 ms – 2000 ms**. With 5 ms the app would click almost every time the pointer slows down, which is dangerous (unwanted clicks, drag-and-drops, closed windows).

**Suggestion:** range 100 ms – 3000 ms, default 600 ms.
REVIEW_RESULT: OK. Suggestion accepted.

### 1.6 Interaction between features (unwanted clicks)
- AutoClick could **click on the Break window's "Dismiss" button** by itself, or click unexpectedly while the user reads the break window.
- Synthetic clicks produced by AutoClick or Key Control could be detected as **user activity** by the Break Timer.
- Synthetic clicks could be re-captured by the app's own hooks and cause loops.

**Suggestion:** all synthetic events are tagged and ignored by the app's own monitors. AutoClick is paused while the break window is shown. Synthetic clicks do not count as activity (only real mouse movement and key presses do).
REVIEW_RESULT: it is ok for auto-click to click buttons on the app's interface. including on the break window. so, if it would not cause any app issue, maintain auto-click and keyboard actions working even on the app's own interface.

---

## Part 2 – Improvement suggestions

| ID | Suggestion | Why |
|----|------------|-----|
| S-01 | Use **Qt 6 (6.8 LTS or newer)** with **CMake** as the build system and **C++20**. | Current long-term version, native CMake support on all 3 OSes. |
| S-02 | Split the code into layers: `core` (platform-independent logic: timers, state machines, settings), `platform` (one implementation per OS behind a common interface: input hook, click injection, autostart, hotkeys) and `ui` (Qt widgets). | Lets the AI implement and test logic on macOS while keeping Windows/Linux code isolated, with no `#ifdef` chaos. |
| S-03 | Store settings with `QStandardPaths::AppConfigLocation` (e.g. `~/Library/Application Support/ErgoPro2/settings.json` on macOS, `%APPDATA%\ErgoPro2\` on Windows, `~/.config/ErgoPro2/` on Linux). | Standard per-user location on each OS. |
| S-04 | Add a `schemaVersion` field in the JSON, validate every value against min/max, fall back to defaults on invalid values, and back up a corrupted file instead of overwriting it silently. | Robust start-up, consistent with the "never silently ignore errors" rule. |
| S-05 | Write settings to a temp file first, then rename it over the old one. Debounce slider changes (e.g. save 300 ms after the last change). | Avoids corrupt files and excessive disk writes while dragging sliders. |
| S-06 | Logging: use Qt's `qInstallMessageHandler` or a small custom logger, with log files in `AppDataLocation/logs`, daily or size-based rotation (e.g. 5 files × 5 MB), and the log level selectable on the General tab. | Logs do not grow forever, and debug output can be turned on when needed. |
| S-07 | Use the **autostart** mechanism native to each OS instead of a "service": Windows → `HKCU\...\Run` registry key; macOS → `SMAppService` login item / LaunchAgent plist; Linux → `~/.config/autostart/ergopro2.desktop`. | A system "service" runs without a desktop session and **cannot** show a GUI or tray icon or read user input. A per-user login item is what is actually needed. |
| S-08 | **Single-instance** guard (`QLockFile` or `QLocalServer`). A second launch brings the existing window to the front. | Two instances would double-click and fight over hotkeys. |
| S-09 | **Tray menu** with quick actions: Show window, Toggle AutoClick, Pause all features (e.g. for 15/30/60 min), Start break now, Exit. Tray icon changes color/state when AutoClick is active. | Main target users want to reduce clicks, and the tray gives one-click access. |
| S-10 | Add a **global hotkey to toggle AutoClick** on/off (as an action type in Key Control). | Users of dwell-click tools need a quick way to stop it, e.g. while selecting text. |
| S-11 | Make the hard-coded values configurable or at least constants in one place: drag-lock timeout (10 s), postpone duration (10 min). | Easy future tuning. |
| S-12 | **Visual feedback for AutoClick**: a small countdown ring or dot near the cursor before the click happens (optional, toggleable). | Standard in accessibility dwell-click tools. Reduces unexpected clicks. |
| S-13 | Handle **multiple monitors and HiDPI**: define whether `AUTOCLICK_TRIGGER_DISTANCE` is in physical or logical pixels (suggest logical). The break window covers all screens. | Avoid inconsistent behavior on Retina/4K displays. |
| S-14 | Add a **"Permissions / Diagnostics"** section on the General tab showing whether required OS permissions are granted (macOS Accessibility, Input Monitoring; Linux session type) with a button that opens the right system settings page. | Most "it doesn't work" problems will be missing permissions. |
| S-15 | Add **unit tests** (Qt Test or GoogleTest) for the core logic: timer state machine, settings load/save/validation, AutoClick decision logic, drag-lock state machine. Abstract time (an injectable clock) so tests don't need real waiting. | Lets the AI verify each stage automatically before moving on. |
| S-16 | Implement in **stages**, each ending with a working, testable build (see Part 4). | Matches your plan to script the implementation in stages. |
| S-17 | Optional: CI with GitHub Actions building on `windows-latest`, `macos-latest` and `ubuntu`/Debian container. | Confirms the cross-platform code compiles even though you test manually on macOS only. |
| S-18 | Keep the UI texts in `tr()` from the start, even if only English is shipped. | Translation later becomes trivial. |


REVIEW_RESULT: 
s-06: user can select what log level should be written (info only, info + warning, info + warning + error, info + warning + error + critical failure, info + warning + error + critical failure + debug) and for how many days log files should be kept on the global tab.
s-10: user will be able to add a keyboard action to do this. no special added items on interface is needed for this.
All other improvement suggestions are accepted as is.
---

## Part 3 – Questions

### 3.1 General / Project

**G-01** Which Qt version and license? Qt Open Source (LGPL) requires dynamic linking and providing the Qt license notices. A commercial license allows static linking.
*Suggested default: Qt 6.8 LTS, LGPL, dynamic linking.*
Answer: default

**G-02** Qt Widgets or Qt Quick/QML for the interface?
*Suggested default: Qt Widgets (simpler, native look, easier for tabs/forms).*
Answer: default

**G-03** Which exact OS versions must be supported? (Windows 11 only, or also 10? macOS Tahoe only, or also older? Debian 12 "Bookworm" or 13 "Trixie"?) On macOS: Apple Silicon only, or also Intel (universal binary)?
*Suggested default: Windows 11, macOS 26 Tahoe (Apple Silicon + Intel universal), Debian 13.*
Answer: default

**G-04** Linux: is it acceptable that the input features only work in an **X11 session** (with a clear warning shown when running under Wayland)? Or must Wayland be supported (much harder, may require `uinput` and extra user permissions)? See 1.1.
*Suggested default: X11 supported, Wayland shows a warning and disables the input features.*
Answer: default

**G-05** Will the app be distributed to other people (needs installer, code signing, macOS notarization), or is it for personal use only?
Answer: personal use

**G-06** Packaging format per OS? (Windows: plain folder/zip, or MSI/NSIS installer? macOS: `.app` in `.dmg`? Linux: `.deb`, AppImage, or plain binary?)
*Suggested default: zip/folder with `windeployqt` for Windows, `.app` + `.dmg` for macOS, `.deb` for Debian.*
Answer: default

**G-07** Do you have an Apple Developer certificate for code signing? If not, is ad-hoc signing acceptable (you will have to re-grant permissions after rebuilds)?
Answer: ad-hoc signing is acceptable

**G-08** Will a version control system (git) be used? Should the AI commit after each stage?
Answer: yes. git will be used. no. AI should not commit. I need to review and test each change.

**G-09** UI language(s)? English only?
Answer: yes. english only

**G-10** Is there a desired look and feel (colors, icon, logo, dark mode support)? Should the AI create a simple placeholder app/tray icon?
Answer: yes. do an application with a nice modern look and feel.

**G-11** Is the app name "ErgoPro2" final? (Used for the settings folder, bundle ID, executable name.) What bundle identifier / organization name should be used (e.g. `com.yourcompany.ergopro2`)?
Answer: yes. name is final. company name = com.techforge.ergopro2

### 3.2 Application window, tray and General tab

**W-01** What happens when the user clicks the window's **close (X) button**: minimize to tray, or exit? And the minimize button?
*Suggested default: close → hide to tray (with a one-time notification "ErgoPro2 is still running in the tray"); exit only through the Exit button or tray menu.*
Answer: default accepted. only close the app if the exit button inside general tab is clicked or if the exit option is clicked on the tray's icon menu.

**W-02** Should the app start **hidden in the tray** when launched automatically at OS login, and show the window when launched manually?
*Suggested default: yes.*
Answer: yes. starting behaviour can be changed by the user on general tab.

**W-03** On macOS, should the app appear in the **Dock** at all, or only in the menu bar (tray)?
*Suggested default: menu bar only while the window is hidden; Dock icon while the window is visible.*
Answer: default

**W-04** What exactly should the **General tab** contain besides "Start with OS" and "Exit"? Ideas: log level selector, "open log folder" button, "open settings folder", permissions status (S-14), "reset all settings to defaults", app version, a global "pause all features" switch.
Answer: all ideas accepted but the last one (a global "pause all features" switch should not be implemented)

**W-05** Should Exit ask for confirmation?
*Suggested default: no.*
Answer: default

**W-06** "Install to run at OS launch": should this be a simple **checkbox** (on/off), or "Install"/"Uninstall" buttons? Should it work for the current user only (no admin rights) or for all users?
*Suggested default: a checkbox, current user only, no admin rights needed.*
Answer: install/uninstall buttons. Wherever possible, no admin rights needed. if needed, ask user's permission.

**W-07** Should the executable be copied somewhere (e.g. `Program Files` / `/Applications`) by the "install" action, or does autostart simply point to wherever the executable currently is?
*Suggested default: point to the current location, and warn if it is in a temporary/download folder.*
Answer: the executable should be copied to a permanent folder if "install" button is clicked. if not it should execute from current location and warn if it is in a temporary/download folder.

**W-08** Should the **Statistics tab** exist now as an empty placeholder ("Coming soon")?
*Suggested default: yes.*
Answer: default

**W-09** Should the app collect any data now (e.g. click counts, break history) to be used later by Statistics, or nothing at all for now?
*Suggested default: nothing now, but design the event flow so it is easy to add later.*
Answer: default

### 3.3 Settings & Logging

**C-01** Is the settings location from S-03 OK, or do you want a specific path (e.g. next to the executable, "portable mode")?
Answer: s-03 ok

**C-02** Should parameters be applied **immediately** while the user changes them (e.g. slider), or only after pressing an "Apply" button?
*Suggested default: immediately, and saved automatically (debounced).*
Answer: default

**C-03** Which **UI controls** for numeric parameters: slider + spin box, spin box only?
*Suggested default: slider + spin box together, showing units.*
Answer: default

**C-04** What are the **default values** for each parameter? Proposal:

| Parameter | Proposed range | Proposed default |
|-----------|----------------|------------------|
| AUTOCLICK_TRIGGER_TIMER | 100–3000 ms (see 1.5) | 600 ms |
| AUTOCLICK_TRIGGER_DISTANCE | 5–200 px | 15 px |
| BREAKTIMER_IDLE_RESET | 1–30 min | 5 min |
| BREAKTIMER_MAX_WORK_TIME | 5–180 min | 50 min |
| BREAKTIMER_BREAK_TIME | 1–30 min | 5 min |
| Drag lock timeout | 1–60 s | 10 s |
| Postpone duration | 1–60 min | 10 min |

Answer:AUTOCLICK_TRIGGER_TIMER=200. all other defaults are ok.

**C-05** Should the enabled/disabled state of AutoClick and the Break Timer also be saved and restored at start-up?
*Suggested default: yes, all states persist.*
Answer: default

**C-06** Logging: where should log files go, how much history should be kept, and what is the default log level?
*Suggested default: `AppDataLocation/logs`, 5 files × 5 MB, level "Info" (Debug selectable on the General tab).*
Answer: default plus user selects how many days of historic logs (from min 1 day to max 180 days)

**C-07** Should errors that matter to the user (e.g. "permission missing", "hotkey already in use by another app") also be shown in the UI (message box / tray notification), not only logged?
*Suggested default: yes.*
Answer: yes.

### 3.4 Function 1 – AutoClick

**A-01** Please confirm the exact AutoClick algorithm:
1. The pointer moves at least `AUTOCLICK_TRIGGER_DISTANCE` px from the position of the last click (or from the position where AutoClick was armed) → AutoClick becomes **armed**.
2. When the pointer stops moving (no movement for `AUTOCLICK_TRIGGER_TIMER` ms) → one left click at the current position → **disarmed**.
3. No new click until step 1 happens again (so the user can rest the mouse without repeated clicks).

Is that right? Is "distance" measured as a straight line from the last click position, or as the total path travelled?
*Suggested default: straight-line distance from the last click position.*
Answer: default

**A-02** What counts as "stopped moving"? Should small jitter (e.g. 1–3 px of hand tremor) be ignored so the timer is not constantly restarted?
*Suggested default: yes, a movement below a small tolerance (e.g. 3 px, configurable) does not restart the timer.*
Answer: default

**A-03** If the user **clicks manually** while AutoClick is armed, should that cancel the pending auto-click and disarm?
*Suggested default: yes.*
Answer: default

**A-04** If the user is **holding a mouse button** (manual drag), should AutoClick be suppressed?
*Suggested default: yes.*
Answer: default

**A-05** Should AutoClick pause while a **keyboard key is pressed** or while the user is typing (e.g. for N ms after the last key press)?
*Suggested default: yes, suspend for 500 ms after any key press.*
Answer: default

**A-06** Should AutoClick **ignore clicks on the ErgoPro2 window itself** (so the user can safely change settings / turn it off)? And on the break window?
*Suggested default: yes for both (see 1.6).*
Answer: no. autoclick should work on app's interface as well.

**A-07** Should there be an exclusion list of applications where AutoClick is disabled (e.g. games, drawing apps)? Can this be a later feature?
Answer:no. it should work everywhere. no need to plan for a later feature.

**A-08** Should AutoClick show visual feedback before clicking (S-12)? Play a sound?
Answer: nice to have a visual feedback. not needed. Sound should never be played.

**A-09** Should the AutoClick on/off state be shown in the tray icon (e.g. different icon color)?
*Suggested default: yes.*
Answer: if easy to implement, yes. if not, it is not a must.

### 3.5 Function 2 – Key Control

**K-01** Is the single-hook architecture in 1.4 acceptable instead of literally one thread per action? (Functionally it behaves the same: every action is independent, can be disabled/reset individually, and the key is exclusively bound.)
Answer: yes. it is acceptable

**K-02** What exactly should **"Reset key binding"** do from the user's point of view? Options:
a) Unregister and immediately re-register the key with the OS (a "repair" action if the binding stopped working).
b) Remove the key from the action, so the action stays in the list without a key until a new one is assigned.
c) Release the key back to the OS (the action becomes unbound / disabled).
Answer: a

**K-03** **Disable** an action: should the key be released back to the OS while disabled (so it works normally in other apps), or stay captured but do nothing?
*Suggested default: released back to the OS while disabled.*
Answer: default

**K-04** Should there also be a **Delete** action (remove from the list completely)?
*Suggested default: yes, with confirmation.*
Answer: default

**K-05** Which keys are allowed? Single keys without modifiers (e.g. `F8`, or even plain letters), or only combinations with modifiers? Allowing plain keys like `A` would make that letter unusable for typing.
*Suggested default: allow function keys (F1–F24), numpad keys, and any key combined with at least one modifier; warn when a plain alphanumeric key is chosen.*
Answer: default

**K-06** Should **mouse buttons** (e.g. extra side buttons 4/5) be allowed as triggers, not only keyboard keys?
Answer: only keyboard keys.

**K-07** What happens when the user picks a combination that is **already used** by another action in the app, or by the OS/another app (registration fails)? Show an error and ask for another key?
*Suggested default: yes, with a clear message.*
Answer: default

**K-08** Should the **same key** be allowed for multiple actions? (Suggested: no.)
Answer: no

**K-09** When an action fires, where does the click happen: at the current mouse pointer position?
*Suggested default: yes.*
Answer: yes

**K-10** For the click actions (left/right/double), should holding the key **auto-repeat** the click, or should it only click once per press? Should the click happen on key **press** or key **release**?
*Suggested default: once per press, on key press, no auto-repeat.*
Answer: default

**K-11** Should the double-click interval follow the OS double-click setting?
*Suggested default: yes.*
Answer: default

**K-12** Could the list of possible actions grow later (e.g. middle click, scroll up/down, toggle AutoClick, start break, type text/macro)? Should the code be designed for easy extension now?
*Suggested default: yes, action types are pluggable. Include "Toggle AutoClick" and "Middle click" now?*
Answer: default. it should be expandable later but no additional action should be included right now.

**K-13** Should hotkeys work while the **break window** is shown?
Answer: yes

**K-14** Should all hotkeys be **paused** while the ErgoPro2 "Add Hotkey" capture dialog is open (to avoid triggering an existing action while capturing a new key)?
*Suggested default: yes.*
Answer: default

**K-15** "Add Hotkey" flow: please confirm.
1. Click "Add Hotkey" → dialog "Press the key or key combination…" (with Cancel).
2. User presses a combination → it is shown (e.g. `Ctrl+Shift+F9`), with "Confirm / Retry".
3. User picks an action from a dropdown → "Save".
4. The binding is registered with the OS; on failure an error is shown.

Answer: confirmed. the sequence is correct.

### 3.6 Drag Lock (Key Control action)

**D-01** Please confirm the drag-lock state machine:
- Press hotkey → left button **down** (held).
- Release happens when any of these occurs: hotkey pressed again, 10 s timeout, **any other** keyboard key pressed, any mouse button clicked.
- On release → left button **up** at the current pointer position.

Does "any keyboard key" include **modifier keys** alone (Shift/Ctrl, often used during drag operations, e.g. to copy files)?
*Suggested default: modifiers alone do not release the drag.*
Answer: default

**D-02** Is the 10 s timeout measured from when the drag started, or from the **last mouse movement** (so a slow, long drag is not cut off)?
*Suggested default: from the last mouse movement.*
Answer: default

**D-03** If the user releases the drag with a **mouse click**, should that click be **consumed** (swallowed) or passed through to the application in addition to the release?
*Suggested default: consumed. It only releases the drag.*
Answer: default

**D-04** AutoClick re-arming after drag-lock: confirm that after release the pointer must travel `AUTOCLICK_TRIGGER_DISTANCE` from the **release position** before AutoClick can click again.
Answer: yes. confirmed.

**D-05** Should a visual indicator (e.g. tray icon change, small overlay near the cursor) show that drag-lock is active?
*Suggested default: yes, tray icon change.*
Answer: default

**D-06** If the break window appears while drag-lock is active, should the drag be released automatically?
*Suggested default: yes.*
Answer: default

### 3.7 Function 3 – Break Timer

**B-01** Precise definition of INTERACTION_TIMER. Which one do you mean?
a) **Wall-clock time** since the first activity, reset only after `BREAKTIMER_IDLE_RESET` minutes of inactivity (short pauses still count as work).
b) Only **active** time (time is counted only while the user produces input, e.g. per-second/minute buckets with activity).
*Suggested default: (a), which is the usual model in break reminder tools.*
Answer: default

**B-02** What counts as activity: mouse movement, mouse clicks, scrolling, key presses? Should synthetic events from AutoClick/Key Control count? (See 1.6.)
*Suggested default: all real user input; synthetic events excluded.*
Answer: default is ok since synthetic events are always generated by real user input (auto-click needs mouse movement and keyboard actions need keyboard key presses)

**B-03** Should a natural idle period ≥ `BREAKTIMER_IDLE_RESET` count as a "break taken" (reset the timer)? Confirm `BREAKTIMER_IDLE_RESET` should normally be ≤ `BREAKTIMER_BREAK_TIME`, or are they independent?
Answer: yes. if BREAKTIMER_IDLE_RESET > idle period, reset the active timer. BREAKTIMER_IDLE_RESET and BREAKTIMER_BREAK_TIME are independant and have different functionality.

**B-04** **Where are the Dismiss / Postpone buttons?** The description says "Buttons on Break Timer tab", but from context they seem to belong on the **break window**. Please confirm. Should the Break Timer tab also have buttons like "Start break now" and "Reset timer"?
Answer: "Dismiss and postpone" buttons are only on the break window do close the window if user needs to keep working on the instance the break window is shown. No. break timer tab should not have buttons to break now or reset the timer.

**B-05** The break window "cannot be clicked away unless a button is pressed". Should it:
a) Be a normal top-most window on the main screen only, or
b) Be **full-screen on all monitors** (covering everything, dimmed background)?
Should keyboard shortcuts like Alt+Tab / Cmd+Tab / Esc be blocked while it is shown? (Fully blocking the OS is hard and may be undesirable for emergencies.)
*Suggested default: (b), semi-transparent full-screen overlay on all monitors, stays on top, Esc does nothing, but the OS task switcher is not blocked.*
Answer: default.

**B-06** What happens when the break window **closes automatically** after `BREAKTIMER_BREAK_TIME`? Does INTERACTION_TIMER reset (new cycle)?
*Suggested default: yes.*
Answer: yes.

**B-07** **Dismiss** resets INTERACTION_TIMER even if the user did not rest. Is this intended (the user effectively skips the break)? Should there be a confirmation or a limit?
Answer: yes. disiss resets the timer. no confirmation or limit is needed.

**B-08** **Postpone**: is the postpone duration always 10 min? Is there a maximum number of postpones per cycle? During postpone, does INTERACTION_TIMER keep counting?
*Suggested default: 10 min (configurable), unlimited postpones, timer keeps counting.*
Answer: default

**B-09** If the user is **idle during the postponed 10 minutes** for longer than `BREAKTIMER_IDLE_RESET`, should the postponed break be cancelled (they already rested)?
*Suggested default: yes.*
Answer: yes. break should be cancelled and INTERACTION_TIMER should be reset.

**B-10** What should the break window display? Ideas: countdown (mm:ss), a message ("Time to rest your wrists"), optional stretching tips/exercises, an image. Should text be configurable?
Answer: countdown, message and fixed streching tips with images. No need to be configurable.

**B-11** Should there be a **pre-warning** (e.g. tray notification 1 min before the break: "Break in 1 minute")?
*Suggested default: yes, optional.*
Answer: no. not needed.

**B-12** Should a **sound** play at the start/end of the break?
Answer: no.

**B-13** If the user keeps using the mouse/keyboard **during** the break window, should anything happen (e.g. countdown pauses/restarts)?
*Suggested default: nothing; countdown continues.*
Answer: default

**B-14** Should the Break Timer have its own **enable/disable** switch (like AutoClick)?
*Suggested default: yes, shown at the top of the tab like AutoClick.*
Answer: yes. shown at the top of break timer tab

**B-15** "Display actively working minutes at the bottom of the Break Timer tab": should it update live (every second/minute)? Also show "time until next break" and the current state (Working / Idle / On break / Postponed)?
*Suggested default: yes, update every second, show all of these.*
Answer: default

**B-16** What should happen when the computer goes to **sleep / screen lock** and comes back? Should sleep/lock time count as idle (resetting the timer if long enough)?
*Suggested default: yes, sleep/lock counts as idle.*
Answer: default

**B-17** Should the break be suppressed when a **full-screen application** is running (presentation, video call)?
*Suggested default: optional setting, off by default.*
Answer: default

---

## Part 4 – Proposed implementation stages (for your review)

Please confirm or reorder. Each stage ends with a build that compiles on macOS, has passing unit tests, and is manually verifiable.

| Stage | Content |
|-------|---------|
| 0 | Project skeleton: CMake, Qt 6, folder structure (`core` / `platform` / `ui` / `tests`), logging module, settings (JSON load/save/validate), main window with the 5 empty tabs, tray icon, single instance, Exit button. |
| 1 | General tab: start-with-OS (autostart per OS), log level, permissions/diagnostics panel. `BUILD.md` with build/run/autostart instructions for the 3 OSes. |
| 2 | Platform input layer: global mouse/keyboard monitoring thread + synthetic click injection (macOS first, then Windows, Linux X11). |
| 3 | Break Timer: activity tracking, INTERACTION_TIMER state machine, break overlay window, dismiss/postpone, tab UI. |
| 4 | AutoClick: algorithm, own thread, tab UI, interactions with the break window. |
| 5 | Key Control: hotkey capture dialog, exclusive OS registration, actions list (enable/disable/modify/reset/delete), click actions. |
| 6 | Drag Lock action + AutoClick interaction rules. |
| 7 | Hardening: Windows and Linux verification, packaging (`.dmg`, Windows zip/installer, `.deb`), final documentation. |
| 8 | (Later) Statistics. |

**P-01** Is this stage order OK?
Answer: if stage 4 and 5 are not dependant on 3, change order to 0, 1, 2, 4, 5, 3, 6, 7, 8

**P-02** After each stage, do you want to test manually and approve before the AI continues to the next stage?
*Suggested default: yes.*
Answer: default

**P-03** Do you have access to Windows 11 and Debian machines/VMs for testing, or should the AI rely on CI builds (S-17) to make sure the code compiles there?
Answer: rely on CI builds for Windows and LInux. They will be tested manually when available.
