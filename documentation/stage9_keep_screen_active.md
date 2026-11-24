# Stage 9 – Keep Screen Active (v1.9.0)

> **Changed in 1.9.1 (owner request):** the Teams / Zoom click goes exactly to the window's top-left corner
> (replaces 4.5), and adding the hotkey or enabling it again in the hotkey list switches the feature on
> (replaces the "stays Disabled" rule in section 5). See Part B.10.
>
> **Status:** implemented in 1.9.0 (see CHANGELOG and Part B.10). Implementation notes: on Linux X11 a Teams that is
> only in the tray cannot be found through the window list and counts as not running.

Feature description agreed with the owner on 2026-11-18 (questions and answers at the end). Use it as the
stage prompt: implement exactly this, following Part A of `documentation/implementation_stages.md`
(architecture, versioning, coding, testing, workflow rules) and add it to Part B as **B.10 Function 5 –
Keep Screen Active**.

---

## 1. Purpose

While the user is away from the computer (no mouse/keyboard input), ErgoPro2 keeps the screen awake and
keeps Microsoft Teams / Zoom showing the user as active, by making a small input at a fixed interval. It
never interferes while the user is working: any real input stops it at once.

## 2. Terms

| Term | Meaning |
|---|---|
| **Real input** | Mouse movement, button, wheel or key from the user. Events injected by ErgoPro2 (tagged *synthetic*, B.6) are **not** real input. |
| **Idle time** | Time since the last real input. Sleep and screen lock count as idle time (wall clock). |
| **Cycle** | One keep-active action (section 4). |
| **Enabled** | The user switched the feature on with its hotkey (persisted). |

## 3. Settings (`settings.json`, section `keepScreenActive`)

Key names follow the 1.8.2 style (camelCase + unit). Validation, defaults and fallback as for all other keys
(B.2).

| Key | Range | Default | Shown in the UI | Meaning |
|---|---|---|---|---|
| `keepScreenActive.enabled` | bool | `false` | indirectly (card status) | On/off; changed only by the hotkey and the hotkey table (section 6). Remembered across restarts. |
| `keepScreenActive.startAfterIdleMinutes` | 1–60 | 3 | **no** | Idle time before the first cycle. |
| `keepScreenActive.intervalMinutes` | 1–60 | 3 | **no** | Time between two cycles while the user stays idle. |
| `keepScreenActive.maxActiveHours` (2.4.0) | 4–25, out of range → nearest limit | 4 | **no** | Hours without real input after which the feature switches itself off so the screen can sleep; **25 = never**. |

The two minute values can only be changed by editing `settings.json` (with ErgoPro2 closed, or picked up at
the next start like other system-only values). The interface never mentions them. Changing them in the file
while ErgoPro2 runs is not required to apply live.

## 4. Behavior

### 4.1 States

| State | When | Card text |
|---|---|---|
| **Disabled** | Feature switched off, or no hotkey configured for it. | "Disabled" |
| **Idle** | Enabled, but not acting: the user is working (idle time < `startAfterIdleMinutes`), **or** a pause condition (4.4) applies. | "Idle" |
| **Working** | Enabled, idle time ≥ `startAfterIdleMinutes`, no pause condition: cycles are running. | "Working" |

### 4.2 Timing

1. While Enabled, ErgoPro2 watches the idle time.
2. When the idle time reaches `startAfterIdleMinutes`, the state becomes **Working** and the **first cycle
   runs immediately**.
3. While Working, a cycle runs every `intervalMinutes` (measured from the previous cycle).
4. **Any real input** ends Working at once: state **Idle**, no further cycle, the idle time starts again from
   0. The feature's own injected events never count as input (they are synthetic).
5. When a pause condition ends (e.g. screen unlocked) and the user is still idle long enough, Working resumes
   with an immediate cycle.

### 4.3 One cycle (in this order; the first step that applies ends the cycle)

1. **Microsoft Teams** – if a Teams process is running:
   - find its main window (the largest top-level window of that process);
   - if the window is minimized or hidden (tray / menu bar only), **restore it** and bring it to the front;
   - **left-click once on an empty spot of its title bar** (see 4.5);
   - **move the pointer back** to where it was before the click;
   - the cycle ends.
2. **Zoom** – if Teams is not running but Zoom is: exactly the same as step 1 with Zoom's main window.
3. **Neither running** – **harmless input**: move the pointer by 1 logical pixel and immediately back. No
   click, no window change. This keeps the screen awake and resets the OS idle timer.

If a step fails (window not found, restore or click refused by the OS), it is logged as WARNING and the cycle
continues with the next step (finally the harmless input), so every cycle makes some input.

Process / window identification (to be verified during implementation and listed in the code):

| App | Windows | macOS | Linux X11 |
|---|---|---|---|
| Teams | `ms-teams.exe` (new Teams), `Teams.exe` (classic) | bundle id `com.microsoft.teams2`, `com.microsoft.teams` | window class `teams-for-linux`, `Microsoft Teams` |
| Zoom | `Zoom.exe` | bundle id `us.zoom.xos` | window class `zoom` |

### 4.4 Pause conditions (state Idle, no cycles)

- the screen is **locked** or the computer **sleeps** (clicks cannot reach apps behind the lock screen);
- the **break window** is shown (a click would hit the break overlay);
- **Drag Lock** holds the left button (a click would disturb it).

### 4.4a Time limit (2.4.0, owner request)

After `maxActiveHours` hours without real input (pauses do not reset the time) the feature is switched off,
exactly as with the hotkey, and a WARNING is logged. 25 hours or more: never.

### 4.5 Title-bar click position

The click must not hit a control in the title bar (search field, buttons, window controls). The
implementation picks a fixed relative spot of the window's top bar that is empty in the current Teams / Zoom
layouts on each OS (e.g. 8 px below the top edge, in a free area between the left controls and the centre),
documents it in the code and in the manual tests, and keeps it in one place so it can be adjusted. If the
window is too small for a safe spot, the step counts as failed (→ next step).

### 4.6 What the feature never does

- It does not count as activity anywhere: its moves and clicks are synthetic, so they are **not** counted by
  the **Statistics** (no mouse seconds, clicks, distance, usage %, and **not** in "clicks done for you") and
  **not** by the **Break Timer** (no work time, no reset of the idle/pause rules), and they do not trigger or
  disturb **Auto Click**.
- It does not change any other setting, does not keep the computer awake by OS power settings (only by
  input), and does not act on the lock screen.

## 5. Key Control – new action

- New action in the *Action* drop-down of the Add/Modify Hotkey dialog: **"Keep screen active on/off"**
  (pluggable action, stable id e.g. `toggleKeepScreenActive`, like the existing "Auto Click on/off").
- Only **one** hotkey may have this action; adding a second is refused with an explanation.
- Pressing the hotkey **toggles** Enabled ↔ Disabled (saved in `keepScreenActive.enabled`). A short tray
  notification is not required.
- **Disable** in the hotkey table → the hotkey stops working **and** the feature is switched to Disabled.
  **Enable** in the table makes the hotkey work again; the feature stays Disabled until the hotkey is
  pressed.
- **Removing** the hotkey → feature Disabled and the card (section 6) disappears.
- Like all hotkeys it works everywhere, including on the break window; it releases an active Drag Lock first
  (existing rule "any hotkey releases the drag lock").

## 6. Auto Click tab – new card "Keep screen active"

- At the **bottom** of the Auto Click tab, **only visible while a hotkey with this action exists** in Key
  Control (enabled or disabled).
- Content (read-only, no other items):
  - **Enabled / Disabled** badge (same badge style as the other cards);
  - **Hotkey:** the configured key combination (e.g. "F8"), with "(disabled in Key Control)" when the
    binding is disabled;
  - **Status:** *Idle* / *Working* / *Disabled* (4.1), updated live (at least once per second).
- No sliders and no mention of the two minute values.

## 7. Platform support

| OS | Teams / Zoom click | Harmless input | Notes |
|---|---|---|---|
| macOS | yes | yes | Needs Accessibility (already required for clicks) – restoring/raising other apps' windows uses the Accessibility API; window bounds from `CGWindowListCopyWindowInfo` (no Screen Recording permission needed for bounds/owner). |
| Windows | yes | yes | `EnumWindows` / process name, `ShowWindow(SW_RESTORE)`, `SetForegroundWindow` (handle the foreground-lock rules; the click itself also activates the window). Windows of apps running as Administrator cannot be clicked (existing limitation). |
| Linux X11 | yes | yes | Window list via EWMH (`_NET_CLIENT_LIST`, `WM_CLASS`), restore via `_NET_ACTIVE_WINDOW`. |
| Linux Wayland | **no** | yes | Other apps' windows cannot be found or targeted: always the harmless input (relative uinput move). The card shows a one-line note "On Wayland only a small pointer movement is used." |

All OS code stays in `src/platform` behind a new interface (e.g. `IAppWindows`: find main window of a process
list, restore + raise, window frame in logical pixels). No `#ifdef` elsewhere.

## 8. Architecture and logging

- **core:** `KeepScreenActiveEngine` – pure state machine driven by real-input notifications, pause
  conditions, enabled flag, settings and the injectable clock; decides *when* a cycle is due and reports the
  state (Disabled / Idle / Working). Fully unit-tested with `FakeClock`.
- **platform:** window lookup / restore / frame per OS (`IAppWindows`), clicks and moves through the existing
  `IClickInjector` (synthetic-tagged); a relative/absolute "move pointer" helper if missing.
- **ui:** `KeepScreenActiveController` (wiring: input events, power events, break window, drag lock, settings,
  Key Control action) and the card on the Auto Click tab.
- **Logging:** INFO when enabled/disabled (and why: hotkey, table, removed), when Working starts/stops; DEBUG
  for each cycle (which step was used, target window, position); WARNING for a failed step; nothing at INFO
  per cycle.

## 9. Tests

- **Unit (core):** first cycle exactly at `startAfterIdleMinutes`; next cycles every `intervalMinutes`; real
  input stops Working and restarts the idle count; synthetic events are ignored; each pause condition stops
  cycles and resumption gives an immediate cycle; Disabled never cycles; settings defaults/ranges/fallback
  (`tst_settings`); action registration (only one hotkey with this action; disable in table disables the
  feature).
- **Manual (`MANUAL_TESTS.md`, "Stage 9 – v1.9.0"):** per OS: hotkey toggle and card states; Teams running
  (visible, minimized, tray only) → restore + title-bar click + pointer back, status stays "Available"; Zoom
  only; neither → harmless input, screen does not dim; real input stops it immediately; pauses (lock, break
  window, Drag Lock); Statistics and Break Timer unchanged by the feature's input; Wayland fallback note.

## 10. Version and documentation

- Version **1.9.0** (new feature = MINOR), CHANGELOG entry, Part B **B.10** + B.2 settings rows + B.5
  action list, USER_GUIDE (Key Control action and Auto Click card), ARCHITECTURE, REQUIREMENTS_TRACE rows.

---

## Owner decisions (2026-11-18)

| Question | Decision |
|---|---|
| Neither Teams nor Zoom open (original step 3: minimize all windows + click the desktop) | **Replaced by a harmless input** (1 px pointer move and back); no window changes. |
| Teams/Zoom running but window minimized / only in the tray | **Restore it and click its title bar.** |
| Pointer after the title-bar click | **Moved back** to its previous position. |
| Default times | **Start after 3 min idle, repeat every 3 min**; ranges 1–60 each. |
| Pauses | **Screen locked / sleeping, break window shown, Drag Lock holding.** |
| Enabled state after restart | **Remembered.** |
| Linux | **X11 full; Wayland harmless input only**, explained on the card. |
| Version | **1.9.0**, written as a stage prompt. |
