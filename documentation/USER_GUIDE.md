# ErgoPro2 – User Guide

This guide is for people who **use** ErgoPro2. It applies to version **1.7.9** on Windows 11, macOS 26 Tahoe
and Debian 13. (Building ErgoPro2 from source is described in [BUILD.md](BUILD.md).)

**Contents**

1. [What ErgoPro2 does](#1-what-ergopro2-does)
2. [Download, install, update and uninstall](#2-download-install-update-and-uninstall)
3. [Permissions](#3-permissions)
4. [Window, tray and exit](#4-window-tray-and-exit)
5. [The tabs](#5-the-tabs) – [General](#51-general) · [Break Timer](#52-break-timer) ·
   [Auto Click](#53-auto-click) · [Key Control](#54-key-control) · [Statistics](#55-statistics)
6. [Limitations and troubleshooting](#6-limitations-and-troubleshooting)

---

## 1. What ErgoPro2 does

ErgoPro2 helps reduce strain on your wrists and hands from mouse and keyboard work. It runs quietly in the
background (system tray / menu bar) and offers four functions:

| Function | What it does for you |
|---|---|
| **Break Timer** | Counts how long you actively use the computer and reminds you to take a short break, with stretching tips. |
| **Auto Click** | Clicks the left mouse button for you when you stop moving the pointer – no finger press needed. |
| **Key Control** | Lets keyboard keys do mouse clicks (left, right, double click), hold the left button for dragging (**Drag lock**) or switch Auto Click on/off. |
| **Statistics** | Shows how much you used the mouse and keyboard per day, with charts. Only counts and times are stored – never what you type. |

ErgoPro2 never plays sounds.

---

## 2. Download, install, update and uninstall

### 2.1 Download

Open the project's GitHub page → **Releases** (https://github.com/eduardkb/ErgoPro2_TechForge_App/releases)
and download the file for your system from the newest release (named `ErgoPro2_v<version>`):

| System | File |
|---|---|
| Windows 11 (64-bit) | `ErgoPro2-<version>-windows-x64-setup.exe` |
| macOS (Apple silicon and Intel) | `ErgoPro2-<version>-macos.dmg` |
| Debian 13 (64-bit) | `ergopro2_<version>_amd64.deb` |

### 2.2 Windows 11

1. Double-click `ErgoPro2-<version>-windows-x64-setup.exe`. No administrator rights are needed.
2. **SmartScreen** may show "Windows protected your PC" because the program is not from a known publisher.
   Click **More info** → **Run anyway**.
3. Follow the setup. The first time you can choose the folder (default
   `%LOCALAPPDATA%\Programs\ErgoPro2`). Options:
   - a desktop icon (optional),
   - **Start ErgoPro2 automatically when I log in** (on by default).
4. ErgoPro2 appears in the Start Menu. Start it from there.

The General tab then shows "Installed at … Installed with the ErgoPro2 setup program."

### 2.3 macOS

1. Double-click `ErgoPro2-<version>-macos.dmg`. A window with **ErgoPro2** opens.
2. Double-click **ErgoPro2** in that window.
3. **Gatekeeper:** ErgoPro2 is not notarized by Apple, so macOS may say it cannot verify the app.
   Click **Done** (not "Move to Trash"), then open **System Settings → Privacy & Security**, scroll down to
   *Security* and click **Open Anyway** next to the message about ErgoPro2. Confirm with **Open Anyway** and
   your password / Touch ID. Start ErgoPro2 again.
4. ErgoPro2 starts and asks for two permissions (see [3.1](#31-macos)).
5. Go to the **General** tab → **Installation** card and click **Install**. This copies ErgoPro2 to
   `~/Applications/ErgoPro2.app` and makes it start when you log in. Answer **Yes** when ErgoPro2 offers to
   restart from the installed copy.
6. Eject the disk image (in Finder, click the eject symbol next to "ErgoPro2 <version>").

> As long as ErgoPro2 runs from the disk image, the General tab shows a warning ("running from a disk image
> or removable drive" or "App Translocation"). Click **Install** to fix this.

### 2.4 Debian 13

1. Open a terminal in the folder with the downloaded file and run:
   ```bash
   sudo apt install ./ergopro2_<version>_amd64.deb
   ```
2. Start **ErgoPro2** from the application menu (or type `ErgoPro2` in a terminal).
3. **Start at login:** the package does not do this by itself. Click **Install** on the General tab: it copies
   ErgoPro2 to `~/.local/opt/ErgoPro2/` and registers that copy to start at login.
4. **GNOME** (Debian's default desktop) has no tray area. To see ErgoPro2's tray icon, install
   `sudo apt install gnome-shell-extension-appindicator`, log out and in, and switch on
   *AppIndicator and KStatusNotifierItem Support* in the **Extensions** app. Without a tray, ErgoPro2
   minimizes its window instead of hiding it.
5. On a **Wayland** session, ErgoPro2 needs extra access to the input devices – see [3.3](#33-linux).

### 2.5 First start

- On macOS, the permissions window opens if permissions are missing (see [3.1](#31-macos)).
- The **Break Timer** is on from the start. **Auto Click** is off, and there are no hotkeys yet.
- Settings are saved automatically when you change them. There is no "Save" button.

### 2.6 Updating

ErgoPro2 does not update itself. Download the new release and:

| System | How to update |
|---|---|
| Windows | Run the new setup `.exe`. It closes the running ErgoPro2, installs into the same folder without asking, and keeps your settings, logs and statistics. |
| macOS | **Exit** ErgoPro2 first (tray menu → **Exit**). Open the new `.dmg`, start ErgoPro2 from it, then click **Reinstall** on the General tab and restart from the installed copy. Afterwards check the permissions (see the note below). |
| Debian | `sudo apt install ./ergopro2_<new version>_amd64.deb`. If you used **Install** on the General tab, also exit ErgoPro2, start it from the menu and click **Reinstall**, so the copy that starts at login is updated too. |

> **macOS and permissions after an update:** each release is a new app for macOS. If a function is shown
> as unavailable after the update although ErgoPro2 is switched on in System Settings, remove ErgoPro2 from
> both lists (Accessibility and Input Monitoring: select it, click **−**) and grant the permissions again
> (see [3.1](#31-macos)).

Only one ErgoPro2 can run at a time: starting a second copy just brings the running window to the front.
That is why you exit the old one before starting the new one on macOS and Debian.

### 2.7 Uninstalling

Your **settings, logs and statistics are always kept** when uninstalling (see [6.6](#66-where-settings-logs-and-statistics-are)
to delete them by hand).

| System | How to uninstall |
|---|---|
| Windows | General tab → **Uninstall** (runs the setup's uninstaller; ErgoPro2 closes first), or **Settings → Apps → Installed apps → ErgoPro2 → Uninstall**. |
| macOS | General tab → **Uninstall**. It stops the start at login and deletes `~/Applications/ErgoPro2.app` (when you click it in the installed copy, ErgoPro2 exits and the app is deleted right after). Optionally remove ErgoPro2 from the Accessibility and Input Monitoring lists in System Settings. |
| Debian | If you clicked **Install** before: General tab → **Uninstall** (removes `~/.local/opt/ErgoPro2`, the menu entry and the start at login). Then remove the package: `sudo apt remove ergopro2`. |

---

## 3. Permissions

Auto Click, Key Control, the Break Timer and the Statistics must see (and for clicks, control) the mouse
and keyboard in all applications. Whether this is allowed depends on the operating system.

When something is missing:

- every affected tab shows a banner at the top: **"<Function> is unavailable"**, what is missing, a button to
  fix it and **Check again**. The controls of that function are disabled;
- **General → Permissions & diagnostics** lists each requirement as **GRANTED** or **MISSING**, with a button
  to fix it.

Both re-check automatically every few seconds, so usually no restart is needed.

### 3.1 macOS

| Permission | Needed for | Where |
|---|---|---|
| **Accessibility** | Auto Click, Key Control (clicking for you, keeping your hotkeys for ErgoPro2) | System Settings → Privacy & Security → **Accessibility** |
| **Input Monitoring** | Auto Click, Key Control, Break Timer activity monitoring, Statistics recording | System Settings → Privacy & Security → **Input Monitoring** |

**Granting them**

1. If a permission is missing, the window **"ErgoPro2 needs your permission"** opens when ErgoPro2 starts.
2. Click **Open System Settings** next to a permission. The right page opens.
3. Switch on **ErgoPro2** and confirm with your password / Touch ID. If ErgoPro2 is not in the list, click
   **+** below the list and choose ErgoPro2 (in your `Applications` folder inside your home folder).
4. Come back to ErgoPro2. The status changes to **GRANTED** within a few seconds (or click **Check again**).
   When both are granted, the window says "All permissions are granted. Thank you!" and closes by itself.
5. Still **MISSING** although it is switched on? Click **Restart ErgoPro2** – some macOS versions apply the
   permission only after a restart.

You can close the window with **Continue without**. ErgoPro2 keeps running, the affected tabs show the
banner, and you can grant the permissions later from the banner or the General tab.

**What happens without them**

| Missing | Effect |
|---|---|
| Accessibility | Auto Click and Key Control are unavailable. |
| Input Monitoring | Auto Click, Key Control, the Break Timer and the Statistics are unavailable (the Break Timer cannot see your activity). |

**Removing a permission while ErgoPro2 runs:** ErgoPro2 notices within a second, stops watching the mouse
and keyboard and shows the banners. Your mouse and keyboard keep working normally. When you switch the
permission on again, ErgoPro2 starts again by itself.

**Re-granting after you removed ErgoPro2 from a list:** start ErgoPro2 (it asks again), click
**Open System Settings**, and add/switch on ErgoPro2 as above. If a switch is on but ErgoPro2 still shows
**MISSING**, remove the ErgoPro2 entry with **−**, add it again with **+**, and click **Restart ErgoPro2**.

### 3.2 Windows

No permission is needed – the diagnostics show "No special permission needed".

**Limitation:** Windows does not let clicks and hotkeys reach windows of programs that run **as
Administrator** (for example an installer, Task Manager or a program started with "Run as administrator"),
unless ErgoPro2 also runs as Administrator. In such windows Auto Click and Key Control do nothing, and your
hotkeys type normally there.

### 3.3 Linux

**X11 session:** nothing to grant. The diagnostics show **X11 session – GRANTED**.

**Wayland session (best effort):** Wayland does not let applications observe the mouse and keyboard, so
ErgoPro2 reads the input devices directly. It needs:

| Requirement | Needed for |
|---|---|
| Read access to input devices (`/dev/input`) – membership in the `input` group | all functions |
| Write access to `/dev/uinput` | Auto Click, Key Control |

1. Click **Grant access…** on a banner or on the General tab. ErgoPro2 explains the change and asks you to
   confirm; then the system asks for the **administrator password** (ErgoPro2 never sees it).
2. **Log out and log in again.** The change only applies to a new session.

Manual alternative (same result):
```bash
sudo usermod -aG input "$USER"
echo 'KERNEL=="uinput", GROUP="input", MODE="0660", OPTIONS+="static_node=uinput"' | sudo tee /etc/udev/rules.d/99-ergopro2-uinput.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
```
then log out and in.

> Membership in the `input` group lets every program you run read your keyboard. If you do not want this,
> choose an **X11 session** on the login screen (for example "GNOME on Xorg") instead.

---

## 4. Window, tray and exit

**Closing and minimizing.** Closing the window (X / red button) or minimizing it **does not exit**
ErgoPro2 – it hides the window in the tray (Windows, Linux) or the menu bar (macOS). The first time, a
notification says that ErgoPro2 is still running in the tray. On macOS, the Dock icon is shown only while the
window is open.

**Tray / menu bar icon.** On Windows and Linux, click (or double-click) the icon to open the window and
right-click it for the menu. On macOS, click the menu bar icon to open the menu:

| Menu item | What it does |
|---|---|
| **Show window** | Opens the ErgoPro2 window. |
| **AutoClick on/off** | Switches Auto Click on or off (check mark = on). |
| **Break Timer on/off** | Switches the Break Timer on or off (check mark = on). |
| **Start break now** | Shows the break window immediately. Available only while the Break Timer runs and no break is shown. |
| **Exit** | Exits ErgoPro2. |

The icon changes while **Auto Click is active** and while **Drag lock holds the left button** (Drag lock has
priority).

**Exiting.** ErgoPro2 exits only with **Exit ErgoPro2** (bottom of the General tab) or **Exit** in the tray
menu. There is no confirmation. On exit, a held Drag lock is released and all hotkeys are given back.

**Only one ErgoPro2.** Starting ErgoPro2 a second time brings the running window to the front.

**Window size.** The window can be resized but not maximized or made full screen. It can be at most 80 % of
the screen's usable width and height; the minimum is 720 × 560 (smaller on small screens). It starts at
820 × 620.

**Mouse wheel.** Scrolling over a slider, number box or drop-down list scrolls the page; it never changes the
value. Change values by dragging/clicking the slider, typing in the number box (or clicking its arrows), or
opening the list.

**Starting at login.** **Install** on the General tab (or the Windows setup option) makes ErgoPro2 start when
you log in. What the window does at start is set on the General tab → **Startup** (see [5.1](#51-general)).

---

## 5. The tabs

The tabs are, from left to right: **General**, **Break Timer**, **Auto Click**, **Key Control**,
**Statistics**. Every change applies immediately and is saved automatically.

### 5.1 General

**ErgoPro2 card** – shows **RUNNING** and the **Version**.

**Installation card**

| Item | What it does |
|---|---|
| Status | "✅ Installed at <folder>, starts at login." / "Not installed – running from <folder>." / repair hints if only one part is present. Also "Installed with the ErgoPro2 setup program." on Windows when the setup was used. |
| Warning | Shown when ErgoPro2 runs from a temporary folder, the Downloads folder, a disk image / removable drive, or (macOS) a temporary read-only copy (App Translocation). Click **Install**. |
| **Install** / **Reinstall** | Copies the running ErgoPro2 to a permanent folder in your user account and registers it to start at login. No administrator rights. Afterwards ErgoPro2 offers to restart from the installed copy. |
| **Uninstall** | Stops the start at login and removes the installed copy and its menu entry / shortcut (Windows setup installations: runs the setup's uninstaller). Settings, logs and statistics are kept. |

| System | Installed copy | Start at login |
|---|---|---|
| Windows | `%LOCALAPPDATA%\Programs\ErgoPro2\` (or the folder chosen in the setup) | Windows "Run" entry (Task Manager → **Startup apps** shows it) |
| macOS | `~/Applications/ErgoPro2.app` | Login item (System Settings → General → **Login Items & Extensions** → *Allow in the Background* must be on for ErgoPro2) |
| Linux | `~/.local/opt/ErgoPro2/` | `~/.config/autostart/ergopro2.desktop` |

**Startup card** – what ErgoPro2 does with its window when it starts:

| Setting | Choices | Default |
|---|---|---|
| **Started at login** | Hide in tray / Show window | Hide in tray |
| **Started manually** | Show window / Hide in tray | Show window |

**Logging card**

| Setting | What it does | Range | Default |
|---|---|---|---|
| **Log level** | Which messages are written to the log files: *Info only* · *Info + Warning* · *Info + Warning + Error* · *Info + Warning + Error + Critical (recommended)* · *Info + Warning + Error + Critical + Debug*. | 5 choices | Info + Warning + Error + Critical |
| **Keep log files for** | Older log files are deleted when ErgoPro2 starts and once a day – never right away when you change the value. | 1–180 days | 30 days |

The card also shows the log and statistics folders and has the buttons **Open settings folder**,
**Open log folder** and **Open statistics folder**.

**Permissions & diagnostics card** – operating system, desktop session (e.g. X11 / Wayland), where ErgoPro2
runs from, each permission (**GRANTED** / **MISSING** with a fix button), the **Input hook** (**RUNNING** /
**NOT RUNNING** with the reason) and **Start at login** (Registered / Not registered). **Check again**
re-reads everything.

**Input diagnostics card** – collapsed by default; click **Show**. A live view of what ErgoPro2 receives from
the mouse and keyboard, for troubleshooting. Nothing is recorded or stored. See [6.4](#64-input-diagnostics-explained).

**Reset card** – **Reset all settings to defaults…** restores every setting after a confirmation. This also
**deletes all hotkeys** (their keys work normally again). The installation, start at login, log files and
statistics files are not changed. It cannot be undone.

**Exit ErgoPro2** – exits the application (closing the window only hides it).

### 5.2 Break Timer

The Break Timer counts your **active work time** and shows a break window when it reaches the set time.

**How the work time counts**

- It starts with your first mouse or keyboard input after a reset.
- It counts while you use the mouse or keyboard. After **20 seconds** without any input it stands still
  (those 20 s still count) and continues with your next input. The state then shows **Paused**.
- After a pause of **Reset after a pause of** minutes (also sleep or a locked screen), the work time resets
  to zero – that pause counts as a break.
- Clicks made by Auto Click or Key Control are **not** counted as activity; your own input is.

**Top card** – switch **Break reminders** on/off (default **on**) and status badge **ACTIVE** / **INACTIVE**
(**UNAVAILABLE** when a permission is missing). Switching it off closes a break window and resets the work
time.

**Status card** (updated every second)

| Field | Meaning |
|---|---|
| **Active work time** | Work time of the current work period (mm:ss). |
| **Work started at** | Time of day (24 h) when the current work period started. "–" after a reset until you use the computer again, and while the timer is off. |
| **Next break in** | Time until the break window appears ("waiting (full-screen app)" while a break is held back by a full-screen app). |
| **State** | *Working*, *Paused* (no input for 20 s – the time stands still), *Idle* (waiting for your first input), *On break*, *Postponed*, *Disabled*. |

**Settings card** (changes apply immediately, also to the running work period)

| Setting | What it does | Range | Default |
|---|---|---|---|
| **Break after working** | Active work time until the break window appears. **Half** of it while the computer usage (Statistics) is above 100 % – see below. | 20–120 min | 50 min |
| **Reset after a pause of** | No mouse or keyboard use for this long (also sleep or a locked screen) counts as a break and resets the work time. | 2–15 min | 5 min |
| **Break length** | How long the break window stays before it closes by itself. | 3–15 min | 4 min |
| **Postpone by** | After **Postpone**, the break window returns after this time. | 3–20 min | 10 min |
| **No breaks during full-screen apps** | While a full-screen application (video, presentation, game) is in front, the break waits until it is no longer full-screen. Windows and macOS only; on Linux it is disabled ("Not available on this system"). | on / off | off |

**The break window**

- Covers **all screens** with a dark, semi-transparent layer, always on top. The main screen shows "Time to
  rest your wrists", a countdown (mm:ss) and five stretching tips (wrist flexor stretch, wrist extensor
  stretch, fist and fan, shoulder rolls, look into the distance). Other screens show "Time for a break – see
  the main screen" and the countdown.
- It cannot be closed with Esc, Alt+F4 or Cmd+W. You can still switch to other apps with the task switcher.
- **Countdown reaches zero:** the window closes and the work time starts again from zero. The computer
  usage (Statistics) is reset to 0 %.
- **Dismiss:** skips the break; the window closes and a new work period starts. The computer usage is **not**
  reset.
- **Postpone N min:** the window closes and returns after *Postpone by* minutes; the work time keeps counting.
  You can postpone as often as you like. If you are away for *Reset after a pause of* minutes during the
  postpone, the postponed break is cancelled and the work time resets.
- Using the mouse or keyboard during the break changes nothing (the countdown continues).
- Auto Click and your hotkeys work on the break window. An active Drag lock is released when it appears.
- No warning before the break, no sound.

**Half-time rule:** if you keep skipping breaks (Dismiss / Postpone), the computer usage on the Statistics
tab grows above 100 %. While it is above 100 %, the next break comes after **half** of *Break after working*
(e.g. 25 instead of 50 min). If the usage goes above 100 % while you are already past half the time, the
break starts at once. The full time applies again once the usage is back at 0 % (after a completed break or
a pause as long as *Break length*). See [5.5](#55-statistics).

### 5.3 Auto Click

Auto Click clicks the **left mouse button** for you when the pointer stops moving.

**How it works**

1. Move the pointer at least **Minimum movement** away from where the last click happened (or from where
   Auto Click was switched on). Auto Click is now *armed*.
2. Stop moving. After **Wait before clicking** without movement, it clicks **once** at the pointer position.
3. Resting the pointer never produces repeated clicks – you must move away again first.

It does **not** click:

- while you hold a mouse button (for example while dragging yourself),
- within 0.5 s after a key press (typing),
- while Drag lock holds the left button (after the release you must move *Minimum movement* again).

A manual click (any button) – or a click from a Key Control hotkey – cancels a pending auto-click.
Auto Click also works on the ErgoPro2 window and on the break window. Small movements below
**Ignore tremor below** do not restart the countdown.

**Status card** – badge **AutoClick is ACTIVE** / **INACTIVE** (briefly *STARTING…*), the
**Activate** / **Deactivate** button and a status line:
"Waiting: move the pointer to where you want to click.", "Armed: stop moving to click.", "Clicking when the
pointer rests…", or "Paused: …" (a mouse button is held / you are typing / Drag Lock is active). Auto Click
can also be switched with the tray menu or a Key Control hotkey. Whether it is on is remembered when you exit.

**Settings card** (changes apply immediately)

| Setting | What it does | Range | Default |
|---|---|---|---|
| **Wait before clicking** | How long the pointer must rest before the click. | 100–3000 ms | 200 ms |
| **Minimum movement** | How far the pointer must move away from the last click before the next auto-click. | 5–200 px | 15 px |
| **Ignore tremor below** | Small hand tremor up to this distance does not restart the countdown. | 1–10 px | 3 px |
| **Show countdown cross** | An orange X grows from the pointer tip until the click happens. It appears only after the pointer has been completely still for half of *Wait before clicking*. Not available on Linux Wayland. | on / off | off |

### 5.4 Key Control

Key Control lets keyboard keys do mouse actions. While a hotkey is **enabled**, other applications do
**not** receive that key – it belongs to ErgoPro2. All clicks happen at the current pointer position and
count as manual clicks for Auto Click. A hotkey fires once per key press (holding the key does not repeat).
Hotkeys also work on the ErgoPro2 window and on the break window.

**Actions** – each action can have **one** hotkey, and each key one action:

| Action | What it does |
|---|---|
| **Left mouse click** | One left click. |
| **Right mouse click** | One right click. |
| **Double left click** | A double click (using your system's double-click speed). |
| **Drag lock (left button)** | Holds the left button down until released (see below). |
| **Turn Auto Click on/off** | Switches Auto Click on or off. |

**Adding a hotkey**

1. Click **Add Hotkey**. While the dialog is open, all hotkeys are paused so you can press any key.
2. "Press the key or key combination…": hold the modifier keys (Shift, Ctrl, Alt/Option, Cmd/Win) and press
   the key. Function keys F1–F24 and keypad keys can be used alone. (**Cancel** closes the dialog.)
3. "You pressed:" shows the combination, e.g. `Ctrl+Shift+F9`. Click **Confirm**, or **Retry** to press
   another key. If the key is not allowed, a message says why and **Confirm** is disabled.
4. Choose the **Action** from the list (only actions without a hotkey are offered) and click **Save**.
5. If the system refuses the key, the dialog shows why and lets you press another key.

When every action already has a hotkey, **Add Hotkey** is disabled and a note above the list says so. Delete
a hotkey to free its action.

**The list** – columns **Key**, **Action**, **Status** and buttons:

| Status | Meaning |
|---|---|
| **ACTIVE** | The hotkey works. |
| **DISABLED** | Switched off; the key works normally in other applications. |
| **PAUSED** | Temporarily paused (e.g. while the Add/Modify Hotkey dialog is open). |
| **UNAVAILABLE** | Key Control is unavailable (see the banner at the top). |
| **ERROR** | The hotkey cannot work; the reason is shown below (e.g. assigned twice). |
| **HOLDING** | Drag lock is holding the left button right now. |

| Button | What it does |
|---|---|
| **Disable** / **Enable** | Gives the key back to other applications / takes it again. A disabled hotkey keeps its action. |
| **Modify…** | Opens the dialog with the current key and action. Change the action, or click **Change key…** to press a new key, then **Save**. |
| **Reset binding** | Releases and immediately re-registers the key – use it if a hotkey stopped working. |
| **Delete…** | Removes the hotkey after a confirmation; the key works normally again and the action becomes free. |

**Which keys can be used**

- Best: **F1–F24** and **keypad** keys, alone or with modifiers.
- Any other key **with at least one modifier** (Shift, Ctrl, Alt/Option, Cmd/Win).
- A letter, digit, space, Enter, Tab, arrow or other typing key **without** a modifier shows a warning
  (it would no longer type in other applications) – click **Use anyway** if you really want it.
- On Mac keyboards the top row may need the **fn** key for F1–F12.
- Not possible: a modifier alone, and keys ErgoPro2 cannot recognize (Fn / Globe, media keys).

**Reserved combinations** – rejected with "This combination is reserved by the operating system":

| System | Reserved |
|---|---|
| Windows | Ctrl+Alt+Del, Ctrl+Shift+Esc, Ctrl+Esc, Alt+Tab, Alt+Shift+Tab, Alt+Esc, Alt+F4, and **every combination with the Win key** |
| macOS | ⌘Tab, ⇧⌘Tab, ⌘Space, ⌃Space, ⌥⌘Esc, ⌃⌘Q, ⇧⌘3, ⇧⌘4, ⇧⌘5, ⇧⌘Q |
| Linux | Ctrl+Alt+Del, Ctrl+Alt+Backspace, Ctrl+Alt+F1 … F12, Alt+Tab, Alt+Shift+Tab |

**Drag lock**

Press the Drag lock hotkey: the left mouse button is **pressed and held** at the pointer. Move the pointer to
drag files, select text or move windows without holding the button. The row shows **HOLDING** and the tray
icon changes.

The button is let go (at the current pointer position) when:

| Event | Note |
|---|---|
| You press the Drag lock hotkey again | The key is not passed on. |
| The mouse does not move for **10 seconds** | Timeout; see [6.7](#67-advanced-settings-only-in-settingsjson). |
| You press any **non-modifier** key | The key still reaches the application normally. Shift, Ctrl, Alt/Option, Cmd/Win alone do **not** release. Another hotkey first releases, then does its action. |
| You press any **mouse button** | That click only releases – it is **not** passed on (Windows, macOS; see the Linux limitation in [6.1](#61-known-limitations)). |
| The break window appears | |
| Auto Click is switched off | |
| The Drag lock hotkey is disabled, deleted, modified or reset, or the Add/Modify Hotkey dialog opens | |
| Key Control becomes unavailable, the input hook stops, or ErgoPro2 exits | ErgoPro2 never leaves the button stuck down – even after a crash it releases it. |

While Drag lock holds the button, Auto Click does not click. After the release, the pointer must move
*Minimum movement* before the next auto-click.

### 5.5 Statistics

The Statistics tab shows how you used the mouse and keyboard, per day. Statistics are **always recorded**
while ErgoPro2 runs; there is no on/off switch. (On macOS this needs Input Monitoring; otherwise the tab shows
the "Statistics recording is unavailable" banner.)

**What is recorded**

- Only your **real** input. Clicks made by ErgoPro2 are counted separately as *Clicks done for you*.
- Nothing is recorded while ErgoPro2 is not running, the computer sleeps or the screen is locked.
- **Privacy:** only counts and durations are stored. **Never** which keys you pressed, never pointer
  positions, never window or application names. The files stay on your computer.

**Day navigation (fixed at the top)** – the shown date ("Today – 18 November 2026" for today, e.g. "Tuesday – 10 November 2026" for other days) and the buttons
**‹ Previous day**, **Today**, **Next day ›**. Previous/next jump to the nearest day that has statistics
(days without data are skipped). Today's view updates once per minute; past days are read-only.

**Computer usage** (line chart, one point per minute)

- **What the percent means:** your active work time since the last usage reset, divided by
  *Break after working* (Break Timer tab). It counts exactly like the Break Timer's active work time.
  **0 %** = no work time, **100 %** = you reached *Break after working* – normally the break comes now.
- **Reset to 0 %:** when there was **no input for *Break length*** minutes (sleep, a locked screen and
  ErgoPro2 not running count as no input), and when the break window **closes by itself** (countdown over).
- **Dismiss** and **Postpone** do **not** reset it – the value keeps growing beyond 100 %. While it is above
  100 %, breaks come after half the work time (half-time rule, [5.2](#52-break-timer)).
- **Maximum 500 %:** if breaks are ignored until the usage goes above 500 %, it is set back to **0 %** and the
  next break comes after the full *Break after working* time again.
- If you change *Break after working*, the new value becomes the 100 % reference when a new work period
  starts (or at once if the work time is 0). Recorded minutes are never recalculated.
- The usage also works when the Break Timer is switched off, and it is **not** reset at midnight.
- The chart shows the hours with data (an empty day shows 08:00–18:00 and "No data for this day"), a line at
  100 %, and markers for **break shown**, **dismissed**, **postponed**, **completed** (see the legend). Gaps
  are not connected. Hover for time, percent and event.

**Activity** – three bar charts on the same time axis, in 5-minute bars: mouse clicks, key presses, mouse
distance. Hover for the exact values.

**Day totals**

| Tile | Meaning |
|---|---|
| **Mouse clicks** | Your clicks (left · right · other). |
| **Clicks done for you** | Clicks made by Auto Click and Key Control (a double click counts 2, a Drag lock press 1). |
| **Key presses** | Key presses (no auto-repeat; modifiers and hotkeys count). |
| **Mouse wheel** | Wheel events. |
| **Mouse distance** | Approximate distance the pointer moved on screen (cm / m, from the screen's resolution). |
| **Mouse use** / **Keyboard use** / **Total active use** | Seconds with mouse input / keyboard input / any input (h:mm:ss). Mouse + keyboard can be more than the total, because both can happen in the same second. |
| **Highest usage** | Highest computer usage % of the day. |

A new day starts at local midnight (counters start at zero).

**Options**

| Setting | What it does | Range | Default |
|---|---|---|---|
| **Keep statistics for** | Statistics files older than this are deleted when ErgoPro2 starts and once a day (at midnight) – never right away when you lower the value. | 1–360 days | 90 days |

**Open statistics folder** opens the folder with the files: one CSV file per day,
`ergopro2-stats-YYYY-MM-DD.csv`, one row per minute with data. You can open them in a spreadsheet program.
Each finished minute is written right away, so at most the current minute is lost after a crash. If you
restart ErgoPro2 the same day, the day continues.

---

## 6. Limitations and troubleshooting

### 6.1 Known limitations

| Where | Limitation |
|---|---|
| Windows | Clicks and hotkeys do not reach programs running **as Administrator** unless ErgoPro2 also runs as Administrator. |
| Windows | Combinations with the **Win** key cannot be used as hotkeys. |
| Linux Wayland | Best effort only. Needs the `input` group and `/dev/uinput` access (see [3.3](#33-linux)). The absolute pointer position is not available (diagnostics show relative values), the **Show countdown cross** option is not available, and mouse distance is approximate. |
| Linux (X11 and Wayland) | The click that ends a Drag lock **cannot be swallowed**: it also reaches the application (a left click merges with the held button). |
| Linux | **No breaks during full-screen apps** is not available. |
| Linux GNOME | No tray without the AppIndicator extension ([2.4](#24-debian-13)); ErgoPro2 then minimizes instead of hiding. |
| macOS | Each new release may need the permissions to be granted again ([2.6](#26-updating)). |
| All | Key Control works only with keyboard keys (not mouse buttons). Only one Drag lock at a time. |

### 6.2 A function shows "… is unavailable"

Read the banner: it says what is missing. Click its button (**Open System Settings** / **Grant access…**),
then **Check again**. See [3](#3-permissions) for your system. On Linux Wayland, log out and in after granting
access.

### 6.3 Hotkeys do not work

| Check | What to do |
|---|---|
| Status in the Key Control list | Must be **ACTIVE**. **DISABLED** → click **Enable**. **ERROR** → read the reason; choose another key with **Modify…**. **UNAVAILABLE** → see 6.2. |
| A dialog is open | While the Add/Modify Hotkey dialog is open, all hotkeys are paused. |
| The key stopped working after a while (sleep, other apps) | Click **Reset binding** on its row. |
| Mac F-keys | Press **fn** with F1–F12, or use a key combination. |
| Windows, program "as Administrator" | Not possible unless ErgoPro2 also runs as Administrator (3.2). |
| Input hook | General → **Permissions & diagnostics** → **Input hook** must be **RUNNING**. |

### 6.4 Input diagnostics explained

General → **Input diagnostics** → **Show**:

| Row | Meaning |
|---|---|
| **Hook** | **RUNNING** / **NOT RUNNING**, the method used, and whether ErgoPro2 "can block keys" (needed for exclusive hotkeys). |
| **Pointer position** | Live pointer position (and screen). On Wayland only a relative position. |
| **Last key event** / **Last mouse button** | The last input seen – shows the hook is receiving events. Shown live only, never stored. |
| **Synthetic (injected)** | Whether the last event was made by ErgoPro2 itself (Auto Click, Key Control). |
| **Events per second** | Input rate; *dropped* should stay 0; *blocked keys* = hotkey presses kept from other apps. |
| **Watchdog** | **Reinstalls** and **Self-repairs** (see below). |

**Watchdog.** ErgoPro2 checks its mouse/keyboard hook every 30 seconds and after sleep/wake and screen
unlock.

- **Self-repairs** = small repairs in place, without restarting the hook:
  - **Windows:** the hook is re-armed as a precaution at every health check; that routine step is **not**
    counted (since 1.8.0 – before, this number grew by about 2 per minute). It counts only when the hook
    had to be installed a second time.
  - **macOS:** macOS switched the event tap off (for example after a slow moment) and ErgoPro2 switched it on
    again.
  - **Linux:** keys were grabbed again after a keyboard layout change (X11) or the input devices were scanned
    again after a device was plugged in (Wayland).
- **Reinstalls** = the hook was stopped and started again – after wake from sleep and screen unlock
  (normal), or because the health check found it not working.
- **PERSISTENT FAILURE** = several reinstalls in a row failed. ErgoPro2 keeps trying; check the permissions,
  then exit and start ErgoPro2 again. If it stays, look at the log file (6.6).

**Permission lost (macOS).** If Accessibility or Input Monitoring is removed while ErgoPro2 runs, the hook
shows **NOT RUNNING** ("Waiting for the permissions above."), the tabs show banners, and your mouse and
keyboard keep working normally. Grant the permission again and the hook starts by itself.

### 6.5 Other problems

| Problem | Solution |
|---|---|
| Starting ErgoPro2 does nothing / only shows a window | It is already running. Look for the tray / menu bar icon. If it does not respond, end it (Windows: Task Manager; macOS: Activity Monitor; Linux: `pkill ErgoPro2`) and start again. |
| No tray icon on Debian GNOME | Install the AppIndicator extension ([2.4](#24-debian-13)). |
| macOS: "cannot verify" / "damaged" | Use **Open Anyway** in System Settings → Privacy & Security ([2.3](#23-macos)). |
| macOS: permission is on but ErgoPro2 shows **MISSING** | Click **Restart ErgoPro2**. Still missing: remove ErgoPro2 from the list with **−**, add it again with **+**, restart. |
| ErgoPro2 does not start at login | General tab → Permissions & diagnostics → **Start at login** must say *Registered*; otherwise click **Install** / **Reinstall**. macOS: *Allow in the Background* must be on for ErgoPro2 (System Settings → General → Login Items & Extensions). |
| Break window never appears | Break Timer on? Status shows *Working*? *No breaks during full-screen apps* may hold it back ("waiting (full-screen app)"). Short pauses reset nothing, but a pause of *Reset after a pause of* resets the work time. |
| Breaks come after half the time | The computer usage is above 100 % (you dismissed or postponed breaks). Let a break finish (countdown to zero) or pause for *Break length*. |
| Auto Click clicks when I don't want it | Increase **Wait before clicking** and/or **Minimum movement**; switch it off quickly with the tray menu or a **Turn Auto Click on/off** hotkey. |
| Auto Click does not click | Status line says why (*Paused: …*). Move the pointer at least *Minimum movement* away from the last click first. |
| A message says the settings were reset and mentions `settings.corrupt-….json` | The settings file was damaged; ErgoPro2 kept it under that name and started with default settings. |

### 6.6 Where settings, logs and statistics are

The General tab shows the exact folders and opens them with **Open settings folder**, **Open log folder** and
**Open statistics folder**.

| System | Settings file | Logs and statistics |
|---|---|---|
| Windows | `%LOCALAPPDATA%\techforge\ErgoPro2\settings.json` | `%APPDATA%\techforge\ErgoPro2\logs\` and `…\statistics\` |
| macOS | `~/Library/Preferences/techforge/ErgoPro2/settings.json` | `~/Library/Application Support/techforge/ErgoPro2/logs/` and `…/statistics/` |
| Linux | `~/.config/techforge/ErgoPro2/settings.json` | `~/.local/share/techforge/ErgoPro2/logs/` and `…/statistics/` |

- Log files: one per day, `ergopro2-YYYY-MM-DD.log` (a file over 5 MB continues in `…-1.log`, `…-2.log`).
  When reporting a problem, attach the log of that day and note the time.
- **Reset settings:** General tab → **Reset all settings to defaults…**, or exit ErgoPro2 and delete
  `settings.json` (a new one with defaults is created at the next start).
- To remove everything after uninstalling, delete these folders by hand.

### 6.7 Advanced settings (only in settings.json)

Two values are not on any tab. Change them only if you need to: **exit ErgoPro2 first**, edit
`settings.json` with a text editor, then start ErgoPro2. A value outside the range is replaced by the default.

| Key | What it does | Range | Default |
|---|---|---|---|
| `draglock` → `DRAGLOCK_TIMEOUT` | Seconds without mouse movement before Drag lock lets go. | 1–60 s | 10 s |
| `breaktimer` → `BREAKTIMER_ACTIVITY_PAUSE` | Seconds without input after which the Break Timer's work time (and the computer usage) stands still (*Paused*). | 5–60 s | 20 s |
