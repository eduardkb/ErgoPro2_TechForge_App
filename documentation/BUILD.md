# ErgoPro2 – Build, Run and Install Guide

This guide explains, step by step, how to build ErgoPro2 from source, run the unit tests, start the
application, create a distributable package and start it automatically at login, on:

- [macOS (Tahoe 26)](#1-macos-tahoe-26)
- [Windows 11](#2-windows-11)
- [Debian 13 (Trixie)](#3-debian-13-trixie)

Shared information:
- [Where settings and logs are stored](#4-where-settings-and-logs-are-stored)
- [Install, Uninstall and starting automatically at login](#5-install-uninstall-and-starting-automatically-at-login)
- [Permissions (macOS, Linux Wayland, Windows)](#6-permissions-macos-linux-wayland-windows)
- [Troubleshooting](#7-troubleshooting)
- [How to test each stage](#8-how-to-test-each-stage)

> All commands are run from the **repository root** (the `ergoPro2` folder that contains
> `CMakeLists.txt`), unless stated otherwise.

**Requirements on every OS:** a C++20 compiler, CMake ≥ 3.21, Ninja (recommended) and
**Qt 6.8 or newer** with the modules *Core, Gui, Widgets, Network, Svg* (and *Test* for the unit tests).

**Platform libraries for the input layer** (since v1.2.0, all linked automatically by CMake):
macOS – AppKit, ApplicationServices, CoreServices, IOKit, Security, Carbon headers (all part of the
Xcode Command Line Tools); Windows – `user32`, `shcore`, `wtsapi32` (part of the Windows SDK); Linux –
see section 3.1.

**Version number:** the app version comes from `project(ErgoPro2 VERSION x.y.z)` in the root
`CMakeLists.txt`. You see it in the window title, on the General tab and in the tray tooltip.

---

## 1. macOS (Tahoe 26)

### 1.1 Install the prerequisites

1. **Xcode Command Line Tools** (compiler, `codesign`, `iconutil`):
   ```bash
   xcode-select --install
   ```
   If they are already installed you will see a message saying so.

2. **CMake, Ninja and Qt.** Choose **one** of the two options:

   **Option A – Homebrew (simplest, recommended for personal use).**
   Install Homebrew from <https://brew.sh> if you don't have it, then:
   ```bash
   brew install cmake ninja qt
   ```
   Homebrew installs the newest Qt 6 (6.8 or later is required). Its Qt is built **only for your Mac's own
   processor** (Apple Silicon *or* Intel), so it cannot produce a universal binary.

   **Option B – Official Qt online installer (needed for a universal arm64 + x86_64 binary).**
   1. Download the installer from <https://www.qt.io/download-qt-installer> and sign in with a free Qt account.
   2. Select **Qt → Qt 6.8.x → macOS** (Qt SVG is part of the base component). Qt is installed to
      e.g. `~/Qt/6.8.3/macos`.
   3. Install CMake and Ninja with Homebrew (`brew install cmake ninja`) or from the Qt installer
      (*Developer and Designer Tools*).

3. Check the tools:
   ```bash
   cmake --version
   ninja --version
   ```

### 1.2 Configure and build

Tell CMake where Qt is with `CMAKE_PREFIX_PATH`:

| Qt from | `CMAKE_PREFIX_PATH` value |
|---------|---------------------------|
| Homebrew | `"$(brew --prefix qt)"` |
| Qt online installer | `~/Qt/6.8.3/macos` (adapt to your version) |

**Debug build** (logs are also printed in the terminal):
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
```

**Release build:**
```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build-release
```

**Universal binary** (Apple Silicon + Intel; only with the official Qt installer, Option B):
```bash
cmake -S . -B build-universal -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=~/Qt/6.8.3/macos -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build-universal
lipo -archs build-universal/bin/ErgoPro2.app/Contents/MacOS/ErgoPro2   # prints: x86_64 arm64
```

After changing source files, only `cmake --build build` needs to be run again.

### 1.3 Run the unit tests
```bash
ctest --test-dir build --output-on-failure
```
All tests must show `Passed`.

### 1.4 Run the application
The build creates an app bundle at `build/bin/ErgoPro2.app`.

- From Finder: open the `build/bin` folder and double-click **ErgoPro2**.
- From the terminal (shows log output in Debug builds):
  ```bash
  ./build/bin/ErgoPro2.app/Contents/MacOS/ErgoPro2
  ```
  or, without terminal output:
  ```bash
  open build/bin/ErgoPro2.app
  ```

Useful command-line options: `--help`, `--version`, `--autostart` (simulates a start at login).

> **Permissions and the terminal:** when you start ErgoPro2 from Terminal (first command above), macOS
> treats **Terminal** as the app asking for Accessibility / Input Monitoring, so the permission status
> shown by ErgoPro2 is Terminal's. To test permissions, start ErgoPro2 from **Finder** or with
> `open` (see section 6.1).

### 1.5 Create a self-contained app and a .dmg

The bundle in `build/bin` uses the Qt libraries installed on your Mac. To copy the app to another
folder or Mac, bundle Qt inside it with `macdeployqt`:

```bash
# 1. Release build (see 1.2), then bundle the Qt frameworks and plugins into the app
"$(brew --prefix qt)/bin/macdeployqt" build-release/bin/ErgoPro2.app
#    (official Qt: ~/Qt/6.8.3/macos/bin/macdeployqt)

# 2. Homebrew Qt only: macdeployqt prints "ERROR: Cannot resolve rpath ... QtVirtualKeyboard".
#    ErgoPro2 does not use the virtual keyboard; remove that plugin folder:
rm -rf build-release/bin/ErgoPro2.app/Contents/PlugIns/platforminputcontexts

# 3. Sign the bundle again (ALWAYS after macdeployqt): macdeployqt adds the Qt frameworks and
#    plugins to the bundle, which breaks the signature made by the build (see 1.6).
#    Use the same identity as for your builds: "-" (ad-hoc) or your certificate name.
codesign --force --deep --sign - build-release/bin/ErgoPro2.app
#    with your certificate (1.6.3) instead:
#    codesign --force --deep --sign "ErgoPro2 Local Signing" build-release/bin/ErgoPro2.app

# 4. Create the disk image
mkdir -p dist
hdiutil create -volname ErgoPro2 -srcfolder build-release/bin/ErgoPro2.app -ov -format UDZO dist/ErgoPro2-1.0.0.dmg
```

To install from the `.dmg`: open it and drag **ErgoPro2** to your *Applications* folder (or to
`~/Applications`). The app's own **Install** button (General tab) can do this for you – see section 5.

### 1.6 Code signing (important for permissions)

ErgoPro2 needs the **Accessibility** and **Input Monitoring** permissions. macOS links these
permissions to the app's **code signature**:

- An **unsigned** or **ad-hoc signed** app gets a new identity on every rebuild, so macOS may
  "forget" the permissions after each build (the switch stays on in System Settings but has no effect).
- An app signed with the **same certificate** every time keeps its permissions across rebuilds.
- An app with an **invalid** signature is never matched: the switch is on, but ErgoPro2 still shows
  *MISSING* (this affects Accessibility in particular). The linker alone produces such a signature.

**The build signs `ErgoPro2.app` automatically** (since v1.1.3) after every build, by default ad-hoc
(`-`). The build output shows `Signing ErgoPro2.app (identity: -)`. To sign with your own certificate
instead (recommended, 1.6.3), configure with:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" -DERGOPRO2_CODESIGN_IDENTITY="ErgoPro2 Local Signing"
```
The setting is remembered in the build folder; later builds use it automatically.

Signing happens only when the build actually re-links the app; a build that reports `no work to do`
keeps the existing (still valid) signature.

**What this means for the permissions:**

| Signing | After a rebuild + reinstall |
|---------|-----------------------------|
| Ad-hoc (`-`, default) | The signature is valid, but macOS sees each new build as a **different app**. Remove the old ErgoPro2 entries and grant both permissions again (1.6.4, then section 6.1). |
| Your certificate (1.6.3) | The identity stays the same: the permissions are **kept**, nothing to do. |

**Anything that changes the bundle after the build breaks its signature** – in particular
`macdeployqt` (section 1.5). Sign again afterwards with the same identity (1.6.1). A normal build, run
and Install do not need this; Install copies the signed bundle unchanged.

#### 1.6.1 Ad-hoc signing by hand (e.g. after `macdeployqt`, or an older copy)
```bash
codesign --force --deep --sign - build/bin/ErgoPro2.app
```

#### 1.6.2 Verify a signature
```bash
codesign --verify --deep --strict --verbose=2 build/bin/ErgoPro2.app
codesign --display --verbose=2 build/bin/ErgoPro2.app
```
Expected: `valid on disk` and `satisfies its Designated Requirement`.

#### 1.6.3 Optional but recommended: a free self-signed certificate

Do this once; afterwards every build can be signed with the same identity, and permissions survive rebuilds.

1. Open **Keychain Access** (Applications → Utilities).
2. Menu **Keychain Access → Certificate Assistant → Create a Certificate…**
3. Fill in:
   - **Name:** `ErgoPro2 Local Signing`
   - **Identity Type:** *Self Signed Root*
   - **Certificate Type:** *Code Signing*
4. Click **Create**, then **Done**. The certificate is stored in your *login* keychain.
5. Double-click the new certificate → expand **Trust** → set **Code Signing** to *Always Trust*. Close
   the window and enter your password.
6. Check that it can be used for signing:
   ```bash
   security find-identity -p codesigning
   ```
   `ErgoPro2 Local Signing` must appear in the list.
7. Let the build sign with it: configure once with
   `-DERGOPRO2_CODESIGN_IDENTITY="ErgoPro2 Local Signing"` (see 1.6), or sign by hand:
   ```bash
   codesign --force --deep --sign "ErgoPro2 Local Signing" build/bin/ErgoPro2.app
   ```
   The first time, macOS asks whether `codesign` may use the key: click **Always Allow**.

#### 1.6.4 Reset stuck permissions

If ErgoPro2 shows a permission as missing although it is switched on in System Settings, reset it and
grant it again (for a complete reset before a test, use 1.6.6):
```bash
tccutil reset Accessibility com.techforge.ergopro2
tccutil reset ListenEvent com.techforge.ergopro2      # "Input Monitoring"
```
Then restart ErgoPro2 and grant the permissions when asked (System Settings → Privacy & Security →
*Accessibility* and *Input Monitoring*).

`tccutil` only removes entries recorded under the bundle id. Entries of an app that was signed with
another identifier (for example `ErgoPro2` from the linker-only signature of builds before v1.1.3) must be
removed by hand: in each list select every **ErgoPro2** entry and click **−**.

To see which identity macOS sees for a copy of the app:
```bash
codesign -dv ~/Applications/ErgoPro2.app 2>&1 | grep -E "Identifier|flags"
codesign --verify --strict ~/Applications/ErgoPro2.app && echo "signature valid"
```
`Identifier` must be `com.techforge.ergopro2` and the verification must print `signature valid`.
When the signature is the problem, ErgoPro2's log contains
`Accessibility cannot be matched to this copy of ErgoPro2: …` and the Accessibility hint says why.

#### 1.6.5 Gatekeeper / quarantine (personal use)

An app that was downloaded (e.g. a `.dmg` built by CI) is marked as "quarantined", and macOS may say
it "is damaged" or "cannot be opened" because it is not notarized. For personal use, remove the
quarantine flag:
```bash
xattr -dr com.apple.quarantine /Applications/ErgoPro2.app
```
Alternatively, right-click the app → **Open** → **Open**, or allow it in System Settings → Privacy &
Security → *Open Anyway*. Builds made on your own Mac are not quarantined.

---

#### 1.6.6 Remove all permissions (start a permission test from scratch)

Removing ErgoPro2 from the two lists in System Settings is usually enough, but macOS can keep entries that
are not shown there (e.g. a "Don't Allow" answer to a permission prompt). For a clean start do all steps:

1. **Exit ErgoPro2** (General tab → Exit, or menu bar icon → Exit) and check that it is not running:
   ```bash
   pgrep -fl ErgoPro2 || echo "not running"
   ```
2. **Remove the visible entries:** System Settings → Privacy & Security → **Accessibility** → select every
   **ErgoPro2** entry → **−**. Do the same under **Input Monitoring**.
3. **Reset what is left** under the bundle id (harmless if nothing is left – it still prints "Successfully
   reset"):
   ```bash
   tccutil reset Accessibility com.techforge.ergopro2
   ```
   ```bash
   tccutil reset ListenEvent com.techforge.ergopro2
   ```
   ```bash
   tccutil reset PostEvent com.techforge.ergopro2
   ```
   (`ListenEvent` = Input Monitoring; `PostEvent` = permission to send synthetic clicks, which macOS may
   record separately.)
4. **Check:** open both lists again – no ErgoPro2 entry may be left. macOS offers no supported command to
   list granted permissions (its permission database is protected), so the final check is ErgoPro2
   itself: start it with `open ~/Applications/ErgoPro2.app` (or the build copy) – the window "ErgoPro2
   needs your permission" must show **Accessibility** and **Input Monitoring** as **MISSING**, and the log
   contains `Permission Accessibility : MISSING` and `Permission Input Monitoring : MISSING`.

Optional – also start the installation from scratch: General tab → **Uninstall**, or with ErgoPro2 closed:
```bash
rm -rf ~/Applications/ErgoPro2.app
```
```bash
rm -f ~/Library/LaunchAgents/com.techforge.ergopro2.plist
```
Settings and logs are not affected by any of these steps.

## 2. Windows 11

### 2.1 Install the prerequisites

1. **Visual Studio 2022 Build Tools** (free): <https://visualstudio.microsoft.com/downloads/> →
   *Tools for Visual Studio* → **Build Tools for Visual Studio 2022**. In the installer select the
   workload **Desktop development with C++** (it includes the MSVC compiler, the Windows SDK, CMake and
   Ninja). The full Visual Studio 2022 Community edition works as well.
2. **Qt 6.8 (MSVC build):** download the Qt online installer from
   <https://www.qt.io/download-qt-installer>, sign in, and select **Qt → Qt 6.8.x → MSVC 2022 64-bit**
   (Qt SVG is part of this component). Default location: `C:\Qt\6.8.3\msvc2022_64`.
3. Open the **x64 Native Tools Command Prompt for VS 2022** (Start menu). **Use this prompt for all
   commands below** – it sets up the compiler. Check:
   ```bat
   cl
   cmake --version
   ninja --version
   ```

### 2.2 Configure and build
```bat
cd C:\path\to\ergoPro2
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
cmake --build build
```
Release build: use `-B build-release -DCMAKE_BUILD_TYPE=Release`.

### 2.3 Run the unit tests
The tests need the Qt DLLs, so add Qt's `bin` folder to `PATH` for this prompt first:
```bat
set PATH=C:\Qt\6.8.3\msvc2022_64\bin;%PATH%
ctest --test-dir build --output-on-failure
```

### 2.4 Run the application
With Qt's `bin` folder in `PATH` (see 2.3):
```bat
build\bin\ErgoPro2.exe
```
Double-clicking `ErgoPro2.exe` in Explorer only works after deployment (2.5), because Explorer does not
know where the Qt DLLs are.

### 2.5 Create a self-contained folder / zip
```bat
mkdir dist\ErgoPro2
copy build-release\bin\ErgoPro2.exe dist\ErgoPro2\
C:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe --release --no-translations dist\ErgoPro2\ErgoPro2.exe
powershell Compress-Archive -Path dist\ErgoPro2 -DestinationPath dist\ErgoPro2-1.0.0-windows.zip -Force
```
`dist\ErgoPro2` now contains everything needed. Copy the folder anywhere (e.g.
`%LOCALAPPDATA%\Programs\ErgoPro2`) and double-click `ErgoPro2.exe`. The app's own **Install** button
(General tab) copies it to `%LOCALAPPDATA%\Programs\ErgoPro2` for you – see section 5. Install only
works from a folder prepared with `windeployqt` (it must contain the Qt DLLs).

> Windows SmartScreen may warn about an unknown publisher the first time: click **More info → Run anyway**.

---

## 3. Debian 13 (Trixie)

### 3.1 Install the prerequisites
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git pkg-config \
                 qt6-base-dev qt6-base-dev-tools qt6-svg-dev libgl-dev \
                 libx11-dev libxtst-dev libevdev-dev
```
Debian 13 ships Qt 6.8, which is the minimum supported version.

What the input-related packages are for (since v1.2.0):

| Package | Needed for | Required? |
|---------|------------|-----------|
| `libx11-dev`, `libxtst-dev` | X11 input hook (XRecord), exclusive keys (XGrabKey), clicks (XTest) | **yes** – CMake stops with an error without them |
| `libevdev-dev` (+ `pkg-config`) | Wayland input (evdev/uinput) | optional – without it CMake prints `libevdev not found: Wayland input support disabled` and the input features are unavailable on Wayland |
| QtDBus (part of `qt6-base-dev`) | sleep/lock notifications (logind, screensaver) | optional – CMake prints a notice when missing |

The CMake output tells you which optional parts are enabled.

**System tray on GNOME:** GNOME (Debian's default desktop) has no tray area. Install the AppIndicator
extension, then log out and in again:
```bash
sudo apt install gnome-shell-extension-appindicator
```
and enable *AppIndicator and KStatusNotifierItem Support* in the **Extensions** app. KDE Plasma, Xfce,
Cinnamon and MATE have a tray out of the box. Without a tray, ErgoPro2 minimizes its window instead of
hiding it.

**X11 vs Wayland:** the input features of later stages work fully in an **X11 session** ("GNOME on Xorg"
on the login screen's gear menu). Wayland support is best effort and needs extra access – see section 6.3.
Check your session with `echo $XDG_SESSION_TYPE`.

### 3.2 Configure and build
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```
Release build: `cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build-release`.

### 3.3 Run the unit tests
```bash
ctest --test-dir build --output-on-failure
```

### 3.4 Run the application
```bash
./build/bin/ErgoPro2
```

### 3.5 Install for the current user / create a .deb

Until the `.deb` package is added (stage 7), install into your home folder:
```bash
cmake --install build-release --prefix ~/.local
~/.local/bin/ErgoPro2
```
This also installs the menu entry (`~/.local/share/applications/ergopro2.desktop`) and the icon.
ErgoPro2 then appears in the application menu. Alternatively, the app's own **Install** button
(General tab) copies the executable to `~/.local/opt/ErgoPro2` and creates the menu entry – see section 5.

> **Coming in stage 7:** `cpack -G DEB` will produce `ErgoPro2-<version>.deb`, installable with
> `sudo apt install ./ErgoPro2-<version>.deb`.

---

## 4. Where settings and logs are stored

ErgoPro2 uses Qt's standard per-user folders (organization `techforge`, application `ErgoPro2`).

| OS | Settings file | Log folder |
|----|---------------|------------|
| macOS | `~/Library/Preferences/techforge/ErgoPro2/settings.json` | `~/Library/Application Support/techforge/ErgoPro2/logs/` |
| Windows | `%LOCALAPPDATA%\techforge\ErgoPro2\settings.json` | `%APPDATA%\techforge\ErgoPro2\logs\` |
| Linux | `~/.config/techforge/ErgoPro2/settings.json` | `~/.local/share/techforge/ErgoPro2/logs/` |

- The exact paths are written to the log at every start (`Settings file: …` and `Log folder: …`), and
  the log folder is shown on the General tab.
- Log files: one per day, `ergopro2-YYYY-MM-DD.log`. A file larger than 5 MB continues in
  `ergopro2-YYYY-MM-DD-1.log`, `-2.log`, … Files older than the retention set on the General tab
  (1–180 days, default 30) are deleted automatically.
- To **reset all settings**, use General tab → **Reset all settings to defaults…**, or exit ErgoPro2 and
  delete `settings.json` (a new one with defaults is created at the next start). The General tab's
  **Open settings folder** / **Open log folder** buttons open these folders. A damaged `settings.json` is renamed to `settings.corrupt-<date>.json`
  automatically and you are informed.
- The macOS paths were verified on macOS Tahoe. The Windows and Linux paths follow Qt's
  documentation and will be confirmed when manual tests on those systems are possible.

---

## 5. Install, Uninstall and starting automatically at login

### 5.0 Using the app's Install / Uninstall buttons (recommended)

Open the **General** tab → **Installation** card:

- **Install** copies the *running* ErgoPro2 to a permanent folder in your user account, creates a menu
  entry / shortcut and registers the installed copy to **start at login** with `--autostart`. No
  administrator rights are needed. Afterwards ErgoPro2 offers to **restart from the installed copy**
  (recommended; the copy you started from can then be deleted).
- **Uninstall** stops the start at login and removes the installed copy and its menu entry / shortcut.
  Settings and log files are **kept**. If you click Uninstall in the installed copy itself, ErgoPro2
  exits and the folder is deleted right after it has closed.
- The card shows the status ("Installed at …, starts at login" / "Not installed – running from …") and
  warns when ErgoPro2 runs from a temporary folder, the Downloads folder or a disk image.
- The **Startup** card decides what happens with the window: *Started at login* (default **Hide in tray**)
  and *Started manually* (default **Show window**).

| OS | Installed copy | Start-at-login registration | Menu entry / shortcut |
|----|----------------|-----------------------------|------------------------|
| macOS | `~/Applications/ErgoPro2.app` | `~/Library/LaunchAgents/com.techforge.ergopro2.plist` | (the app in `~/Applications`) |
| Windows | `%LOCALAPPDATA%\Programs\ErgoPro2\` | Registry `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` → value `ErgoPro2` | Start Menu → `ErgoPro2` |
| Linux | `~/.local/opt/ErgoPro2/ErgoPro2` | `~/.config/autostart/ergopro2.desktop` | `~/.local/share/applications/ergopro2.desktop` |

> **macOS:** sign the app **before** clicking Install (section 1.6). The installed copy keeps the
> signature of the copy you installed from, and macOS ties the permissions to it.

**Check that start at login is registered:**

- **macOS:**
  ```bash
  cat ~/Library/LaunchAgents/com.techforge.ergopro2.plist
  ```
  The file must contain the path of the installed copy and `--autostart`. In **System Settings → General →
  Login Items & Extensions**, ErgoPro2 appears under *Allow in the Background* (macOS may show a
  "Background Items Added" notification). After logging out and in again:
  ```bash
  launchctl print gui/$(id -u)/com.techforge.ergopro2 | head -20
  ```
  shows the job, and the log contains `Started automatically at login (--autostart)`.
- **Windows:** Task Manager → **Startup apps** lists ErgoPro2, or in a command prompt:
  ```bat
  reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v ErgoPro2
  ```
- **Linux:**
  ```bash
  cat ~/.config/autostart/ergopro2.desktop
  ```
  `Exec=` must point to `~/.local/opt/ErgoPro2/ErgoPro2 --autostart`.

The General tab's **Permissions & diagnostics** card also shows the registration ("Start at login").

### Manual methods (troubleshooting only)

**Why not a system "service"?** A service (Windows service, `launchd` daemon, `systemd` system
service) runs outside your desktop session – often before anyone logs in, without access to the
screen, the tray or your mouse and keyboard. ErgoPro2 has a window and a tray icon and must
see your input, so it must run **inside your user session**, which is what a login item does.

### 5.1 macOS
- **Easy way:** System Settings → General → **Login Items & Extensions** → *Open at Login* → **+** →
  choose `ErgoPro2.app` (from Applications). (This method cannot pass `--autostart`.)
- **LaunchAgent (passes `--autostart`):** create `~/Library/LaunchAgents/com.techforge.ergopro2.plist`:
  ```xml
  <?xml version="1.0" encoding="UTF-8"?>
  <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
  <plist version="1.0">
  <dict>
      <key>Label</key><string>com.techforge.ergopro2</string>
      <key>ProgramArguments</key>
      <array>
          <string>/Applications/ErgoPro2.app/Contents/MacOS/ErgoPro2</string>
          <string>--autostart</string>
      </array>
      <key>RunAtLoad</key><true/>
  </dict>
  </plist>
  ```
  Load it now (it also loads automatically at every login):
  ```bash
  launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/com.techforge.ergopro2.plist
  ```
  Remove it:
  ```bash
  launchctl bootout gui/$(id -u)/com.techforge.ergopro2
  rm ~/Library/LaunchAgents/com.techforge.ergopro2.plist
  ```

### 5.2 Windows 11
- **Registry (passes `--autostart`):** in a normal command prompt (adapt the path):
  ```bat
  reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v ErgoPro2 /t REG_SZ /d "\"%LOCALAPPDATA%\Programs\ErgoPro2\ErgoPro2.exe\" --autostart" /f
  ```
  Remove: `reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v ErgoPro2 /f`
- **Startup folder:** press `Win+R`, type `shell:startup`, Enter, and put a shortcut to `ErgoPro2.exe`
  there (add ` --autostart` at the end of the shortcut's *Target*).
- Check: Task Manager → **Startup apps** lists ErgoPro2.

### 5.3 Debian / Linux
Create `~/.config/autostart/ergopro2.desktop` (adapt the path in `Exec`):
```ini
[Desktop Entry]
Type=Application
Name=ErgoPro2
Exec=/home/YOUR_USER/.local/bin/ErgoPro2 --autostart
Icon=ergopro2
X-GNOME-Autostart-enabled=true
```
Remove the file to disable autostart.

---

## 6. Permissions (macOS, Linux Wayland, Windows)

Auto Click, Key Control and the Break Timer's activity tracking need to observe and control the mouse
and keyboard in all applications. Each function tab shows a **yellow banner** while its feature is
unavailable, saying what is missing and offering a button to fix it. The General tab's **Permissions &
diagnostics** card lists every requirement with its status. Both update by themselves every 3 seconds
while something is missing.

### 6.1 macOS (Tahoe 26)

| Permission | Needed for | Where to grant it |
|------------|------------|-------------------|
| **Accessibility** | Auto Click, Key Control (clicking for you, keeping your hotkeys) | System Settings → Privacy & Security → **Accessibility** |
| **Input Monitoring** | Auto Click, Key Control, Break Timer (noticing mouse and keys) | System Settings → Privacy & Security → **Input Monitoring** |

1. Start ErgoPro2 from **Finder** (or `open ~/Applications/ErgoPro2.app`) – not from Terminal (see 1.4).
2. If a permission is missing, the window **"ErgoPro2 needs your permission"** opens at startup.
3. Click **Open System Settings** next to a permission. macOS adds ErgoPro2 to the list (switched off)
   and opens the right page. Switch **ErgoPro2** on and confirm with your password / Touch ID.
   If ErgoPro2 is not in the list, click **+** below the list and choose the ErgoPro2 app.
4. Come back to ErgoPro2: the status changes to **GRANTED** within 3 seconds (or click **Check again**).
   When all are granted the window closes by itself.
5. If a permission stays **MISSING** although it is switched on, click **Restart ErgoPro2** (some macOS
   versions apply the change only after a restart). Still missing? Reset it (section 1.6.4), make sure
   the app is signed consistently (1.6.3) and grant it again.

You can also close the window with **Continue without**: the affected tabs then show the banner
until the permissions are granted.

### 6.2 Linux on X11
Nothing to grant. The diagnostics card shows *X11 session – GRANTED*.

### 6.3 Linux on Wayland (best effort)
Wayland does not let applications observe the mouse and keyboard. ErgoPro2 therefore reads the input
devices directly, which needs:

| Requirement | Needed for |
|-------------|------------|
| Read access to `/dev/input/event*` (membership in the `input` group) | all input features |
| Write access to `/dev/uinput` (udev rule) | Auto Click, Key Control |

**Automatic:** click **Grant access…** on a banner or on the diagnostics card. ErgoPro2 explains what it
will change and asks for confirmation; then the system asks for the **administrator password**
(`pkexec` – ErgoPro2 never sees the password). Afterwards **log out and log in again**.

**Manual** (same result):
```bash
sudo usermod -aG input "$USER"
echo 'KERNEL=="uinput", GROUP="input", MODE="0660", OPTIONS+="static_node=uinput"' | sudo tee /etc/udev/rules.d/99-ergopro2-uinput.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
```
Then log out and log in again. Check with `groups` (must list `input`) and `ls -l /dev/uinput`
(group `input`, mode `crw-rw----`).

> Membership in the `input` group lets every program you run read your keyboard. If you prefer not to
> allow that, use an **X11 session** instead ("GNOME on Xorg" on the login screen).

### 6.4 Windows
No special permission is needed. Note: Windows does not let clicks and hotkeys reach windows of
programs running **as Administrator**, unless ErgoPro2 also runs as Administrator.

---

## 7. Troubleshooting

| Problem | Solution |
|---------|----------|
| CMake: `Could not find a package configuration file provided by "Qt6"` | Pass the Qt location: `-DCMAKE_PREFIX_PATH=...` (see 1.2 / 2.2). On Debian install `qt6-base-dev`. |
| CMake: `Qt6Svg` / `Qt6Test` not found | macOS Homebrew: `brew install qt`. Debian: `sudo apt install qt6-svg-dev qt6-base-dev`. Windows: re-run the Qt maintenance tool and make sure the MSVC 2022 64-bit component is complete. |
| CMake: `Qt6 ... version 6.x is not compatible` | Qt 6.8 or newer is required. |
| CMake changed generator / strange cache errors | Delete the build folder (`rm -rf build`) and configure again. |
| Windows: `The code execution cannot proceed because Qt6Core.dll was not found` | Add `C:\Qt\6.8.3\msvc2022_64\bin` to `PATH`, or deploy with `windeployqt` (2.5). |
| Windows: `cl` / `ninja` not found | Use the **x64 Native Tools Command Prompt for VS 2022**. |
| Starting the app does nothing / just brings an existing window to the front | ErgoPro2 is already running (single instance). Look for its tray / menu bar icon. To force-quit: macOS `pkill ErgoPro2`, Windows Task Manager, Linux `pkill ErgoPro2`. |
| "ErgoPro2 is already running but does not respond" | End the other ErgoPro2 process (see above) and start again. |
| No tray icon on Debian GNOME | Install and enable the AppIndicator extension (3.1). |
| macOS: "ErgoPro2 is damaged and can't be opened" | Remove the quarantine flag (1.6.5). |
| macOS: permission is on in System Settings but the app says it is missing | Check the signature and remove/reset the entries (1.6.4), sign consistently (1.6.3), reinstall and grant again. |
| macOS: `macdeployqt` prints `Cannot resolve rpath ... QtVirtualKeyboard` | Homebrew Qt quirk; remove `Contents/PlugIns/platforminputcontexts` (1.5 step 2). |
| macOS: the status of a permission never changes | Start ErgoPro2 from Finder or with `open`, not from Terminal (1.4). Then see 6.1 step 5. |
| Install: "not running from an application bundle (.app)" | Start the `.app` bundle (Finder / `open`), not a copied executable. |
| Windows Install: "folder does not contain the Qt libraries" | Prepare the folder with `windeployqt` first (2.5) and start ErgoPro2 from there. |
| Linux Wayland: still unavailable after "Grant access" | Log out and log in again (group changes apply only to new sessions). Check with `groups`. |
| ErgoPro2 does not start at login | Check the registration (5.0). macOS: System Settings → General → Login Items & Extensions → *Allow in the Background* must be on for ErgoPro2. |
| The app looks wrong / reports an error at start | Check the latest log file (section 4). Every error is logged there. In a Debug build, run it from a terminal to see the log live. |
| Settings were reset and a message mentions `settings.corrupt-…json` | The settings file was damaged; the old file was kept with that name for inspection. |

---

## 8. How to test each stage

> **Input features need the permissions of the copy you run.** From stage 2 on, test with the
> **installed, signed** copy (`~/Applications/ErgoPro2.app`, section 5) that has Accessibility and
> Input Monitoring granted (section 6.1). A freshly built `build/bin` copy is a "new app" for macOS
> when signed ad-hoc, so it has no permissions until you grant them to it as well.


1. Check the version: the window title shows `ErgoPro2 v1.N.x` and must match the stage
   (stage N ⇒ `1.N.0`, bug-fix releases increase the last number).
2. Build (Debug is fine) and run the unit tests (`ctest`), as described for your OS above. All tests
   must pass.
3. If the app is already running, exit it first (General tab → **Exit ErgoPro2**, or tray menu → **Exit**).
4. On macOS, sign the build (1.6) when the stage uses OS permissions (stage 1 and later) and start it
   from Finder / `open` so macOS attributes the permissions to ErgoPro2.
5. Start the new build and follow the checklist of that stage in
   [`MANUAL_TESTS.md`](MANUAL_TESTS.md). Earlier stages' checklists can be re-run as regression tests.
6. If something fails, collect the steps, the version and the relevant log lines and use the prompt in
   [`ai_bugFix.md`](ai_bugFix.md).
