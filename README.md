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
- **Statistics** – planned.

It runs on **macOS (Tahoe 26)**, **Windows 11** and **Debian 13**, and is written in C++20 with Qt 6 Widgets.

> Current status: **v1.2.0 – Stage 2** (application shell, settings, logging, tray, install / start at
> login, permission checks, global input layer with diagnostics). The features above are
> implemented stage by stage; see [`documentation/implementation_stages.md`](documentation/implementation_stages.md).

## Quick start (macOS with Homebrew)

```bash
brew install cmake ninja qt
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
ctest --test-dir build --output-on-failure
open build/bin/ErgoPro2.app
```

Full instructions for all three operating systems (prerequisites, release builds, packaging, code
signing, autostart, troubleshooting): **[`documentation/BUILD.md`](documentation/BUILD.md)**.

## Documentation

| File | Content |
|------|---------|
| [`documentation/BUILD.md`](documentation/BUILD.md) | Build, run, package, sign and autostart on each OS |
| [`documentation/MANUAL_TESTS.md`](documentation/MANUAL_TESTS.md) | Manual test checklists per stage |
| [`documentation/implementation_stages.md`](documentation/implementation_stages.md) | Rules, specification and stage prompts |
| [`documentation/ai_bugFix.md`](documentation/ai_bugFix.md) | Prompt for AI-assisted bug fixes |
| [`CHANGELOG.md`](CHANGELOG.md) | Version history |

## Project layout

```
src/core/      platform-independent logic (logging, settings, clock)
src/platform/  OS abstraction: interfaces, factory, per-OS implementations
src/ui/        Qt Widgets user interface
tests/         unit tests (Qt Test, run with ctest)
resources/     icons, stylesheet, platform files (Info.plist, .rc, .desktop)
```
