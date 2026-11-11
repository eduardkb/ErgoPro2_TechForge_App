> [!IMPORTANT]
> **Disclaimer**
>
> **Free to use.** ErgoPro2 is provided free of charge. You may use, clone and modify it.
>
> **Not a medical device.** ErgoPro2 is an assistive productivity tool intended only to help reduce
> keyboard and mouse usage. It does not diagnose, treat, cure or prevent any medical condition, and it
> is not a substitute for professional medical advice, diagnosis or treatment. If you experience pain,
> numbness or other symptoms, consult a qualified healthcare professional.
>
> **No warranty.** The software is provided "as is", without warranty of any kind. Use it at your own
> risk.

# ErgoPro2

ErgoPro2 is a desktop application that helps people with wrist pain reduce mouse and keyboard use:

- **Auto Click** – clicks the left mouse button automatically when the pointer stops moving.
- **Key Control** – assign keyboard shortcuts to mouse actions (click, right click, double click, drag lock).
- **Break Timer** – reminds you to take a break after a period of continuous work.
- **Statistics** – daily charts and totals of your mouse and keyboard use, with a history of past days.

It runs on **macOS (Tahoe 26)**, **Windows 11** and **Debian 13**, and is written in C++20 with Qt 6 Widgets.

## Quick start

Download the latest version from **[GitHub Releases](https://github.com/eduardkb/ErgoPro2_TechForge_App/releases/latest)**
(the file for your operating system is listed under *Assets*):

| System | File | How to install |
|--------|------|----------------|
| **Windows 11** | `ErgoPro2-<version>-windows-x64-setup.exe` | Run the setup (no administrator rights needed). If SmartScreen warns about an unknown publisher: *More info › Run anyway*. Updating: run the newer setup – it closes ErgoPro2 and replaces the installed version. |
| **macOS** (Apple silicon and Intel) | `ErgoPro2-<version>-macos.dmg` | Open the `.dmg`, start ErgoPro2 and click **Install** on the *General* tab. If macOS says it cannot verify the app: System Settings › Privacy & Security › *Open Anyway*. Then grant the Accessibility and Input Monitoring permissions the app asks for. |
| **Debian 13** | `ergopro2_<version>_amd64.deb` | `sudo apt install ./ergopro2_<version>_amd64.deb`, then start ErgoPro2 from the application menu. |

Your settings, logs and statistics are kept when you update.

**Building from source:** the full build procedure for macOS, Windows and Debian (prerequisites, release
builds, packaging, code signing, autostart, troubleshooting) is in
**[`documentation/BUILD.md`](documentation/BUILD.md)**.

## Documentation

| File | Content |
|------|---------|
| [`documentation/BUILD.md`](documentation/BUILD.md) | Build, run, package, sign and autostart on each OS |
| [`documentation/initial development docs/MANUAL_TESTS.md`](documentation/initial%20development%20docs/MANUAL_TESTS.md) | Manual test checklists per stage |
| [`documentation/implementation_stages.md`](documentation/implementation_stages.md) | Rules, specification and stage prompts |
| [`CHANGELOG.md`](CHANGELOG.md) | Version history |

## Project layout

```
src/core/      platform-independent logic (logging, settings, clock)
src/platform/  OS abstraction: interfaces, factory, per-OS implementations
src/ui/        Qt Widgets user interface
tests/         unit tests (Qt Test, run with ctest)
resources/     icons, stylesheet, platform files (Info.plist, .rc, .desktop)
```
