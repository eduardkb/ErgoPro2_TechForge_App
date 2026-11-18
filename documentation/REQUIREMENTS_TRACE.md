# ErgoPro2 – Requirements Trace (v1.8.0 audit)

## Purpose

This document maps every requirement of the three source documents to the code that implements it, the unit
tests that check it and the manual tests that verify it:

- Table A – `documentation/initial development docs/baseDescription.md` (owner's original description)
- Table B – `documentation/initial development docs/ai_questions.md` (every answered question / REVIEW_RESULT)
- Table C – `documentation/implementation_stages.md` Part B (consolidated specification, B.1–B.9)

Precedence (A.1): Part B > answers in `ai_questions.md` > `baseDescription.md`. Later owner changes (marked
"(1.x.y, owner request)" in Part B) override older answers. The last section lists every gap.

## Legend

| Status | Meaning |
|--------|---------|
| ✅ | Implemented and covered by at least one unit test (plus manual tests where listed). |
| 🟡 | Implemented; verified by manual test only, because it is OS hook / UI / packaging code that is not unit-testable (or not unit-tested). |
| ⚠️ | Gap: missing, incomplete, untested core logic, or outdated documentation. Listed in [Gaps](#gaps). |
| ➖ | Superseded or rejected by a later owner decision (the decision is named in the row). |

**Notation**

- Paths are relative to the repository root. `src/core/autoclick/AutoClickEngine` without extension means the
  `.h` + `.cpp` pair.
- Unit tests: `tst_<file>::<testFunction>` (files in `tests/`).
- Manual tests (`documentation/initial development docs/MANUAL_TESTS.md`): `S3#7` = section
  "Stage 3 – v1.3.0", row 7; `F1.7.8#2` = section "Bug fix – v1.7.8", row 2. Rows in sub-sections (A., B., …) keep
  the stage's running number.
- Windows and Debian rows of the manual tests ("when a machine is available") are not yet run (P-03); those OSes
  are built and unit-tested by CI only.

---

## Table A – baseDescription.md

| ID | Requirement (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------------|----------------|--------------|----------------|----------------|
| BD-1 | Function 1 AutoClick: left click when the pointer stops | `src/core/autoclick/AutoClickEngine`, `src/ui/autoclick/AutoClickController`, `AutoClickWorker` | `tst_autoclick::armsAtDistanceAndClicksOnceAfterTriggerTime` | S3#6–16 | ✅ |
| BD-2 | Function 2 Key Control: key combos automate actions | `src/core/keycontrol/KeyControlEngine`, `src/ui/keycontrol/KeyControlController`, `src/ui/tabs/KeyControlTab` | `tst_keycontrol::addRegistersAndFiresOncePerPress`, `::allActionsDispatch` | S4#1–30 | ✅ |
| BD-3 | Function 3 Break Timer: periodic break suggestion | `src/core/breaktimer/BreakTimerEngine`, `src/ui/breaktimer/*` | `tst_breaktimer::breakAfterMaxWorkTime` | S5#1–24 | ✅ |
| BD-4 | Function 4 Statistics, implemented last | `src/core/statistics/*`, `src/ui/statistics/*`, `src/ui/tabs/StatisticsTab` | `tst_statistics`, `tst_statisticsstore` | S7#1–22, F1.7.9#1–6 | ✅ Implemented in stage 7; "last" ➖ by the owner's reorder (stage overview note: statistics before hardening). |
| BD-5 | Backend C++, interface Qt | `CMakeLists.txt:19` (C++20), `:64` (Qt 6.8 Widgets) | – | S0#1 | 🟡 Build configuration. |
| BD-6 | Same code runs on Windows 11, macOS Tahoe, Debian | `src/platform/{mac,win,linux}/*`, `src/platform/PlatformFactory`, `.github/workflows/ci.yml` | All tests run in CI on the 3 OSes | S1#31–36, S2#19–23, S3#22, S4#30, S5#24, S6#26–27, S7#22, F1.7.2#4–6, F1.7.4#4–10 | ⚠️ Code + CI builds/tests exist for all 3 OSes; Windows/Debian manual runs are still pending (P-03). Gap O-2. |
| BD-7 | Initial tests on macOS, portable code | Layering `src/core` / `src/platform` / `src/ui`; no `#ifdef` outside platform | CI (`ci.yml`) | – | 🟡 |
| BD-8 | Tabs "General", "Break Timer", "Auto Click", "Key Control", "Statistics" | `src/ui/MainWindow.cpp:47-59` | – | S0#3 | 🟡 |
| BD-9 | JSON parameter file read at start | `src/core/settings/SettingsStore`, `SettingsSchema`, `src/main.cpp:86-91` | `tst_settings::createsDefaultsWhenMissing`, `::saveIsDebouncedAndReloads` | S0#8, S0#21 | ✅ |
| BD-10 | Every parameter change saved to JSON | `SettingsStore::setValue` (debounced save) | `tst_settings::saveIsDebouncedAndReloads`, `::pendingChangesAreSavedOnDestruction` | S0#16–18, S3#3–4, S5#3 | ✅ |
| BD-11 | Logging module (errors, warnings, info, debug); errors never silently ignored | `src/core/Logger`, `LogLevel`, `LogCategories`, `CrashHandler` | `tst_logger::*` (9 functions) | S0#9, S0#16–19 | ✅ "Never silently ignored" is a code-review rule (A.5), not testable as a whole. |
| BD-12 | Minimize to tray where possible | `src/ui/TrayController`, `src/ui/MainWindow::closeEvent/changeEvent`, `AppController::hideMainWindowToTray` | – | S0#10–14, F1.1.2#1–7 | 🟡 |
| BD-13 | "Install" from the General tab to run at OS launch | `src/ui/InstallController`, `src/ui/tabs/InstallCard`, `src/platform/*/…Installer/Autostart` | `tst_locationclassifier::*` (location warning only) | S1#21–28, S1#31–34, F1.7.4#4–10 | 🟡 OS file/registry operations. |
| BD-14 | Exit button on the General tab | `src/ui/tabs/GeneralTab.cpp:116-121`, `AppController::exitApplication` | – | S0#20 | 🟡 |
| BD-15 | .md on building per OS, running, auto-execution "service" | `documentation/BUILD.md` §1–5 | – | – | ✅ Documented. "Service" ➖ by S-07 (per-user login item instead of a service). |
| BD-16 | Left click automatically when the pointer stops | `AutoClickEngine` | `tst_autoclick::armsAtDistanceAndClicksOnceAfterTriggerTime`, `::firstPositionIsTheReference` | S3#7–8 | ✅ |
| BD-17 | AutoClick has its own thread | `src/ui/autoclick/AutoClickController.cpp:24` (`AutoClickWorker` moved to a `QThread`) | – | – | 🟡 Thread wiring not unit-tested. |
| BD-18 | autoclick.triggerTimeMs 5–500 ms | `src/core/settings/AutoClickSettings.h:20-22` | `tst_settings::autoClickSettings` | S3#3 | ➖ Range changed to 100–3000 ms by 1.5 / C-04 (default 200 ms). |
| BD-19 | autoclick.triggerDistancePx 5–200 px | `AutoClickSettings.h:23-25` | `tst_settings::autoClickSettings`, `tst_autoclick::distanceIsStraightLine` | S3#3, S3#9 | ✅ |
| BD-20 | Auto Click tab: active/inactive indicator + activate button | `src/ui/tabs/AutoClickTab` (`StatusBadge`, button) | – | S3#2, S3#6 | 🟡 |
| BD-21 | Keys registered exclusively with the OS | `src/core/input/KeyCombo` (suppression registry), `src/platform/mac/MacInput.mm:332`, `src/platform/win/WinInput.cpp:90`, `src/platform/linux/LinuxX11Input.cpp` (XGrabKey), `LinuxEvdevInput.cpp` | `tst_keycombo::swallowDownRepeatAndUp`, `tst_keycontrol::addRegistersAndFiresOncePerPress` | S4#15, S4#18 | ✅ (core); OS swallowing manual only. |
| BD-22 | "Add Hotkey": press combo, choose action, then exclusively bound | `src/ui/keycontrol/HotkeyDialog`, `KeyControlController` | `tst_keycontrol::addRegistersAndFiresOncePerPress` | S4#3–9 | ✅ |
| BD-23 | List of actions with disable, modify (incl. key), reset binding | `src/ui/tabs/KeyControlTab`, `KeyControlEngine` | `tst_keycontrol::disableReleasesAndEnableRegistersAgain`, `::modifyChangesKeyAndAction`, `::resetUnregistersAndRegistersAgain` | S4#18–22 | ✅ |
| BD-24 | One thread per action, re-created on disable/reset | – | – | – | ➖ Replaced by one shared input-hook thread (1.4 REVIEW_RESULT, K-01); reset = unregister + re-register (K-02 a). |
| BD-25 | Action: left mouse click | `src/core/keycontrol/KeyAction` | `tst_keycontrol::allActionsDispatch` | S4#11 | ✅ |
| BD-26 | Action: right mouse click | `KeyAction` | `tst_keycontrol::allActionsDispatch` | S4#12 | ✅ |
| BD-27 | Action: double left click | `KeyAction`, `…ClickInjector::doubleClick` | `tst_keycontrol::allActionsDispatch` | S4#13 | ✅ |
| BD-28 | Drag lock: press holds the left button, pressing the key again releases | `src/core/keycontrol/DragLockEngine`, `src/ui/keycontrol/DragLockController` | `tst_draglock::hotkeyPressesAndHolds`, `::hotkeyAgainReleasesAtTheCurrentPosition` | S6#4–10 | ✅ |
| BD-29 | Drag lock released after 10 s or by any keyboard/mouse button | `DragLockEngine` | `tst_draglock::timeoutSinceTheLastMovement`, `::nonModifierKeyReleases`, `::anyMouseButtonReleases` | S6#11–15 | ✅ Refined by D-01 (modifiers alone do not release) and D-02 (10 s since the last movement). |
| BD-30 | AutoClick does not click during drag lock nor after release until the pointer moved autoclick.triggerDistancePx | `AutoClickEngine` (drag-lock suppression), `DragLockController` | `tst_autoclick::dragLockSuppressesAndResetsTheReferenceOnRelease`, `tst_draglock::autoClickDoesNotClickWhileHoldingNorAfterReleaseUntilTheDistanceIsMoved` | S6#21–25 | ✅ |
| BD-31 | Monitor input; INTERACTION_TIMER; break screen on top, closable only by its buttons | `BreakTimerEngine`, `src/ui/breaktimer/BreakWindow` (`closeEvent`/`keyPressEvent` ignore), `BreakWindowManager` | `tst_breaktimer::interactionTimerCountsOnlyWhileInputContinues`, `::breakAfterMaxWorkTime` | S5#5–6 | ✅ Timer definition changed to active work time (B.7, 1.6.1). |
| BD-32 | breaktimer.idleResetMinutes | `BreakTimerEngine`, `BreakTimerSettings.h:23` | `tst_breaktimer::idleResetCountsAsABreak`, `::idleResetChangeAppliesImmediately` | S5#16, F1.7.7#2 | ✅ Range now 2–15 min (1.7.7). |
| BD-33 | breaktimer.maxWorkMinutes | `BreakTimerEngine`, `BreakTimerSettings.h:24` | `tst_breaktimer::breakAfterMaxWorkTime`, `::maxWorkTimeChangeAppliesToTheCurrentCycle` | S5#5, S5#20, F1.7.7#2 | ✅ Range now 20–120 min (1.7.7). |
| BD-34 | breaktimer.breakMinutes | `BreakTimerEngine`, `BreakTimerSettings.h:25` | `tst_breaktimer::breakClosesAutomaticallyAndResets` | S5#8, F1.7.6#6 | ✅ Range 3–15, default 4 (1.6.1 / 1.7.6). |
| BD-35 | Show active working minutes at the bottom of the tab | `src/ui/tabs/BreakTimerTab` (status panel) | – | S5#4, F1.6.1#7 | ➖ Position changed: status panel is above the parameters (B.7, 1.6.1). Content 🟡. |
| BD-36 | Buttons Dismiss / Postpone on the Break Timer tab | `BreakWindow` | – | S5#5 | ➖ B-04: buttons only on the break window; no break-now/reset on the tab. |
| BD-37 | Dismiss closes and resets INTERACTION_TIMER | `BreakTimerEngine`, `BreakWindow` | `tst_breaktimer::dismissResetsImmediately` | S5#9 | ✅ |
| BD-38 | Postpone: close and reopen after 10 min | `BreakTimerEngine`, `BreakTimerSettings.h:26` | `tst_breaktimer::postponeReopensWhileTheTimerKeepsCounting`, `::postponeCountsFromThePostponeClick` | S5#10 | ✅ Duration configurable 3–20 min (B-08, 1.7.7). |
| BD-39 | Break window countdown; closes at zero | `BreakWindow`, `BreakTimerEngine` | `tst_breaktimer::breakClosesAutomaticallyAndResets` | S5#5, S5#8 | ✅ |

---

## Table B – ai_questions.md

### Part 1 – Critical topics (REVIEW_RESULT)

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| 1.1 | Wayland: best effort, root/permission where needed; unavailable features stated on their tab | `src/platform/linux/LinuxEvdevInput.cpp`, `LinuxPlatform.cpp:223-341` (pkexec fix), `src/ui/widgets/FeatureBanner` | `tst_featureavailability::*` | S1#35–36, S2#22–23 | 🟡 Not run on Debian yet (O-2). |
| 1.2 | macOS: permissions window at start; tabs show why unavailable; code-signing docs | `src/ui/PermissionsDialog`, `src/platform/mac/MacPermissions.mm`, `FeatureBanner`, `BUILD.md` §1.6 | `tst_featureavailability::*` | S1#6–15, F1.1.3#1–7, F1.1.4#1–5 | ✅ |
| 1.3 | Never-capturable combos rejected with reason; per OS | `src/core/keycontrol/KeyComboValidation.cpp:20-130` | `tst_keycontrol::reservedCombosPerOs`, `::modifierOnlyAndUnknownAreRejected` | S4#6, S4#30 | ✅ |
| 1.4 | Single hook thread; bindings must not get lost | `src/ui/InputService`, `src/core/input/HookWatchdog`, platform hooks | `tst_watchdog::*`, `tst_keycontrol::reapplyAfterHookReinstall` | S2#15–17, S4#27–28 | ✅ |
| 1.5 | Trigger range 100–3000 ms | `AutoClickSettings.h:20-22` | `tst_settings::autoClickSettings` | S3#3 | ✅ |
| 1.6 | AutoClick and hotkeys also work on the app's own windows incl. break window | No exclusion in `AutoClickEngine` / `KeyControlEngine` | `tst_autoclick::syntheticEventsAreIgnored` (tagging part) | S3#14, S4#17, S5#12 | 🟡 |

### Part 2 – Improvement suggestions (all accepted; S-06 and S-10 with changes)

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| S-01 | Qt 6.8+, CMake, C++20 | `CMakeLists.txt:1,19,64` | – | S0#1 | 🟡 |
| S-02 | core / platform / ui layers | `src/core`, `src/platform`, `src/ui` | all `tests/` link core only | – | ✅ |
| S-03 | Settings in AppConfigLocation | `src/main.cpp:88-90` | – | S0#8 | 🟡 |
| S-04 | schemaVersion, validation, defaults, corrupt backup | `SettingsStore.cpp:92,131,217`, `SettingsSchema` | `tst_settings::validation`, `::invalidValuesFallBackToDefaults`, `::corruptFileIsBackedUp`, `::nonObjectRootIsCorrupt` | S0#23–24 | ✅ |
| S-05 | Atomic write, debounce 300 ms | `SettingsStore.cpp:203,233` (QSaveFile) | `tst_settings::saveIsDebouncedAndReloads` | S0#18 | ✅ |
| S-06 | Logger with rotation; cumulative level selection + retention days on General tab | `Logger`, `LogLevel`, `GeneralTab::buildLoggingCard` | `tst_logger::selectionMatrix`, `::rollsOverWhenFileTooLarge`, `::retentionCleanupRemovesOldFiles` | S0#16–19 | ✅ |
| S-07 | Native per-user autostart instead of a service | `MacAutostart` (LaunchAgent), `WinPlatform.cpp:23` (HKCU Run), `LinuxPlatform.cpp:50` (`~/.config/autostart`) | – | S1#22–24, S1#31, S1#34 | 🟡 macOS uses LaunchAgent only (see B.1.3-b). |
| S-08 | Single instance, second launch activates first | `src/ui/SingleInstance`, `src/main.cpp:68-83` | – | S0#15 | 🟡 |
| S-09 | Tray menu with quick actions, icon state | `AppController::setupAutoClickTrayEntry/setupBreakTimerTrayEntries`, `TrayController` | – | S3#15, S5#14–15 | 🟡 "Pause all features" ➖ by W-04. |
| S-10 | Toggle AutoClick only as a Key Control action | `KeyAction.cpp:57` | `tst_keycontrol::allActionsDispatch` | S4#14 | ✅ |
| S-11 | Drag-lock timeout and postpone duration configurable | `KeyControlSettings.h:17-18`, `BreakTimerSettings.h:26` | `tst_draglock::timeoutChangeAppliesImmediately` | F1.6.1#5, S5#3 | ✅ |
| S-12 | Visual feedback before the click (optional) | `src/ui/autoclick/AutoClickOverlay`, `AutoClickEngine` feedback state | `tst_autoclick::feedbackWaitsForHalfTheTriggerTimeOfStillness`, `::anyMovementHidesTheFeedbackButKeepsTheClick`, `::countdownProgressAndRemainingTime` | F1.4.1#9–10, F1.4.2#2–5 | ✅ |
| S-13 | Logical pixels, HiDPI; break window on all screens | Platform hooks (logical coordinates), `BreakWindowManager.cpp:52,67-68` | – | S2#4, S2#19, S5#5, S5#22 | 🟡 |
| S-14 | Permissions / diagnostics section | `src/ui/tabs/DiagnosticsCard`, `InputDiagnosticsCard` | – | S1#9, S2#2–3 | 🟡 |
| S-15 | Unit tests + injectable clock | `src/core/Clock.h`, `tests/` (18 programs) | `tst_clock::*` | S0#1 | ✅ |
| S-16 | Staged implementation | `documentation/implementation_stages.md` | – | – | ✅ Process. |
| S-17 | CI on the 3 OSes | `.github/workflows/ci.yml` | ctest in each job (lines 47, 97, 160) | F1.1.1#1 | 🟡 |
| S-18 | UI texts in `tr()` | All `src/ui`, `src/platform` user texts | – | – | 🟡 Spot check: only symbols/numbers (e.g. "–", "⚠") outside `tr()`; CLI help in `src/main.cpp:36-45` is not translated (console only). |

### 3.1 General / Project

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| G-01 | Qt 6.8 LTS, LGPL, dynamic linking | `CMakeLists.txt:64`; deploy scripts bundle shared Qt | – | F1.7.2#4 | 🟡 |
| G-02 | Qt Widgets | `src/ui/*` (no QML) | – | – | 🟡 |
| G-03 | Win 11, macOS 26 universal, Debian 13 | `ci.yml:42` (`arm64;x86_64`), `BUILD.md` §1.2 | – | F1.7.2#1–6 | 🟡 |
| G-04 | X11 supported; Wayland warning | Superseded by 1.1 (best effort) – see 1.1 | – | S1#34–35 | ➖ by 1.1 REVIEW_RESULT. |
| G-05 | Personal use | – | – | – | ✅ No notarization needed. |
| G-06 | zip / .dmg / .deb | `scripts/package-macos.sh`, `package-windows.ps1`, `package-linux.sh`, `packaging/` | – | F1.7.2#1–6 | 🟡 Windows setup .exe added by 1.7.4 (B.1.3-j). |
| G-07 | Ad-hoc signing acceptable | `CMakeLists.txt:38`, `src/CMakeLists.txt:190-195` | – | F1.1.3#1–2 | 🟡 |
| G-08 | git used, AI never commits | Workflow rule A.7 | – | – | ✅ Process. |
| G-09 | English only | `tr()` everywhere, no translations shipped | – | – | ✅ |
| G-10 | Modern look and feel | `src/ui/Theme`, `resources/style.qss`, `src/ui/widgets/*` | – | S0#25–26, F1.4.1#8 | 🟡 |
| G-11 | Name ErgoPro2, id `com.techforge.ergopro2` | `CMakeLists.txt:40-41` | – | F1.1.3#2 | 🟡 |

### 3.2 Window, tray and General tab

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| W-01 | Close → tray (one-time hint); exit only via Exit button / tray | `MainWindow::closeEvent`, `AppController::hideMainWindowToTray` | – | S0#10–14, S0#20, S0#22 | 🟡 |
| W-02 | Start hidden at login, shown manually; user configurable | `src/core/StartupMode`, `AppController.cpp:122-130` | `tst_startupmode::decide` | S1#16–20 | ✅ |
| W-03 | Dock icon only while the window is visible | `MacDesktopIntegration.mm:18-22` | – | S0#10, S0#12 | 🟡 |
| W-04 | General tab: log level, folders, permissions, reset, version; no "pause all" | `src/ui/tabs/GeneralTab` | – | S1#2–5, S1#29–30 | 🟡 |
| W-05 | Exit without confirmation | `AppController::exitApplication` | – | S0#20 | 🟡 |
| W-06 | Install / Uninstall buttons, no admin where possible | `InstallController`, `InstallCard` | – | S1#21–27 | 🟡 |
| W-07 | Install copies to a permanent folder; else warn on temp/Downloads | `MacInstaller.cpp:15`, `WinPlatform.cpp:49-51`, `LinuxPlatform.cpp:97`, `src/core/LocationClassifier`, `InstallCard.cpp:72-80` | `tst_locationclassifier::*` | S1#22, S1#28, F1.1.1#3–4 | ✅ |
| W-08 | Statistics placeholder until implemented | – | – | – | ➖ Replaced by the real tab (stage 7, B.8). |
| W-09 | Collect nothing now; design for later | – | – | – | ➖ Statistics implemented in stage 7. |

### 3.3 Settings & Logging

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| C-01 | Settings path as S-03 | see S-03 | – | S0#8 | 🟡 |
| C-02 | Applied immediately, saved debounced | `SettingsStore`, `ParameterRow` | `tst_settings::saveIsDebouncedAndReloads` | S3#3, S5#3 | ✅ |
| C-03 | Slider + spin box with units | `src/ui/widgets/ParameterRow` | – | S0#18, S3#3 | 🟡 |
| C-04 | Defaults (trigger 200 ms …) | `src/core/settings/*Settings.h` | `tst_settings::autoClickSettings`, `tst_settings::integerRanges`, `tst_settings::booleanDefaults` | S3#2, S5#2, F1.7.7#3 | ✅ All defaults unit-tested since 1.8.0 (G-a1 fixed). |
| C-05 | Enabled states persist | `autoclick.enabled`, `breaktimer.enabled` in schema | `tst_settings::autoClickSettings` | S3#4 | 🟡 Break Timer enabled state manual only. |
| C-06 | Logs in AppDataLocation/logs, 5 MB files, level Info…, 1–180 days | `src/main.cpp:56-57`, `Logger.h:27` | `tst_logger::rollsOverWhenFileTooLarge`, `::retentionIsClamped` | S0#9, S0#18–19 | ✅ |
| C-07 | User-relevant errors also shown in the UI | `AppController::showError`, tray messages, banners, Key Control status column | – | S0#23–24, S4#24 | 🟡 |

### 3.4 Function 1 – AutoClick

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| A-01 | Arm at straight-line distance from last click; click once on stop | `AutoClickEngine` | `tst_autoclick::armsAtDistanceAndClicksOnceAfterTriggerTime`, `::distanceIsStraightLine` | S3#7–9 | ✅ |
| A-02 | Jitter tolerance (3 px, configurable) | `AutoClickEngine`, `AutoClickSettings.h:26-28` | `tst_autoclick::movementRestartsTheTimerButJitterDoesNot`, `::jitterSmallerThanToleranceNeverArms` | S3#10 | ✅ |
| A-03 | Manual click cancels and disarms | `AutoClickEngine` | `tst_autoclick::manualClickCancelsAndMovesTheReference` | S3#11 | ✅ |
| A-04 | No auto-click while a button is held | `AutoClickEngine` | `tst_autoclick::noClickWhileAButtonIsHeld`, `::manualDragLeavesTheReferenceAtTheDropPosition` | S3#12 | ✅ |
| A-05 | Suspend 500 ms after a key press | `AutoClickSettings.h:30` | `tst_autoclick::typingDelaysAPendingClick`, `::continuousTypingKeepsSuspending`, `::modifierKeysDoNotSuspend` | S3#13 | ✅ |
| A-06 | AutoClick also works on ErgoPro2's windows | No exclusion | – | S3#14, S5#12 | 🟡 |
| A-07 | No application exclusion list | – | – | – | ✅ Nothing to implement. |
| A-08 | Visual feedback nice to have; never sound | `AutoClickOverlay`; no audio API used anywhere in `src/` | `tst_autoclick::feedback*` | F1.4.1#9, F1.4.2#2–5 | ✅ |
| A-09 | Tray icon shows AutoClick active if easy | `TrayController.cpp:68-69`, `AppController.cpp:240-250` | – | S3#6 | 🟡 |

### 3.5 Function 2 – Key Control

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| K-01 | Single hook architecture accepted | `InputService`, platform `…InputHook` | `tst_dispatcher::*` | S2#1–6 | ✅ |
| K-02 | Reset = unregister + re-register (a) | `KeyControlEngine` | `tst_keycontrol::resetUnregistersAndRegistersAgain` | S4#21 | ✅ |
| K-03 | Disable releases the key to the OS | `KeyControlEngine` | `tst_keycontrol::disableReleasesAndEnableRegistersAgain` | S4#18 | ✅ |
| K-04 | Delete with confirmation | `KeyControlTab`, `KeyControlEngine` | `tst_keycontrol::removeReleases` | S4#22 | ✅ |
| K-05 | F-keys, numpad, modifier combos; warn for plain keys | `KeyComboValidation` | `tst_keycontrol::allowedCombos`, `::plainTypingKeysNeedConfirmation`, `tst_keycombo::keypadIsDistinct` | S4#7, S4#9 | ✅ |
| K-06 | Keyboard keys only | `HotkeyDialog` (keyboard capture only) | – | S4#3 | 🟡 |
| K-07 | Clear error when combo already used / registration fails | `KeyControlEngine`, `HotkeyDialog.cpp:274-283` | `tst_keycontrol::duplicatesAndReservedAreRejected`, `::registrationFailureIsReportedAndNotKept` | S4#8, S4#30 | ✅ |
| K-08 | Same key not for multiple actions | `KeyControlEngine` | `tst_keycontrol::duplicatesAndReservedAreRejected` | S4#8 | ✅ |
| K-09 | Click at the current pointer position | `…ClickInjector` | – | S4#11–13 | 🟡 |
| K-10 | Once per press, on key-down, no repeat | `KeyControlEngine`, `KeyCombo` | `tst_keycontrol::addRegistersAndFiresOncePerPress`, `tst_keycombo::swallowDownRepeatAndUp` | S4#11 | ✅ |
| K-11 | Double-click interval from the OS | `MacInput.mm:586-588`, `WinInput.cpp:400`, `LinuxX11Input.cpp:511` | – | S4#13 | 🟡 |
| K-12 | Pluggable action types; no extra actions now | `KeyAction` registry | `tst_keycontrol::actionRegistry` | – | ✅ Toggle AutoClick exists because of S-10. |
| K-13 | Hotkeys work while the break window is shown | No pause during breaks | – | S5#12 | 🟡 |
| K-14 | Hotkeys paused while the capture dialog is open | `HotkeyDialog.cpp:71,89`, `KeyControlController::setPaused` | `tst_keycontrol::pauseReleasesEverythingAndResumeRestores` | S4#10 | ✅ |
| K-15 | Add-Hotkey flow (press → confirm/retry → action → save → register) | `HotkeyDialog` | – | S4#3–5 | 🟡 |

### 3.6 Drag Lock

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| D-01 | Release conditions; modifiers alone do not release | `DragLockEngine` | `tst_draglock::nonModifierKeyReleases`, `::modifiersAloneDoNotRelease`, `::anyMouseButtonReleases` | S6#11–16 | ✅ |
| D-02 | Timeout from the last mouse movement | `DragLockEngine` | `tst_draglock::timeoutSinceTheLastMovement`, `::sameMovePositionIsNoMovement` | S6#11 | ✅ |
| D-03 | Releasing click is consumed | `KeySuppressionRegistry::shouldSwallowMouse`, `MacInput.mm:336`, `WinInput.cpp:130` | `tst_draglock::clickIsConsumedByTheInputLayer`, `::onlyOneClickIsConsumedEvenWhileStillHolding` | S6#14–15, S6#27 | ⚠️ Not possible on Linux (X11 and Wayland): `LinuxEvdevInput.cpp:161-163`, `PlatformInterfaces.h:48`. Gap O-1. |
| D-04 | Re-arm only after autoclick.triggerDistancePx from the release position | `AutoClickEngine`, `DragLockController` | `tst_draglock::autoClickDoesNotClickWhileHoldingNorAfterReleaseUntilTheDistanceIsMoved`, `::autoClickReferenceAfterAConsumedClickRelease` | S6#22–24 | ✅ |
| D-05 | Tray icon shows drag lock | `TrayController.cpp:71-72`, `AppController.cpp:242-249` | – | S6#4 | 🟡 |
| D-06 | Break window releases the drag | `AppController.cpp:145-146` | `tst_draglock::applicationReleases` | S6#17 | ✅ |

### 3.7 Function 3 – Break Timer

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| B-01 | (a) wall-clock time since first activity | `BreakTimerEngine` | `tst_breaktimer::interactionTimerCountsOnlyWhileInputContinues` | F1.6.1#9–10 | ➖ Changed to active work time (B.7, 1.6.1 owner request). |
| B-02 | All real input; synthetic excluded | `BreakTimerController.cpp:158` | – | S5#4 | 🟡 Synthetic filter not unit-tested in the Break Timer (it is in `tst_autoclick`/`tst_statistics`). |
| B-03 | Idle ≥ IDLE_RESET resets the timer; independent of BREAK_TIME | `BreakTimerEngine` | `tst_breaktimer::idleResetCountsAsABreak`, `::activityAfterALongGapWithoutUpdatesStartsANewCycle` | S5#16 | ✅ |
| B-04 | Dismiss/Postpone only on the break window; no tab buttons | `BreakWindow`, `BreakTimerTab` | – | S5#2 | 🟡 |
| B-05 | Full-screen semi-transparent overlay on all monitors; Esc does nothing; task switcher not blocked | `BreakWindow.cpp:50,182-196`, `BreakWindowManager` | – | S5#5–6, S5#13, S5#22, S5#24 | 🟡 |
| B-06 | Countdown end resets INTERACTION_TIMER | `BreakTimerEngine` | `tst_breaktimer::breakClosesAutomaticallyAndResets` | S5#8 | ✅ |
| B-07 | Dismiss resets, no confirmation/limit | `BreakTimerEngine` | `tst_breaktimer::dismissResetsImmediately` | S5#9 | ✅ |
| B-08 | Postpone configurable, unlimited, timer keeps counting | `BreakTimerEngine` | `tst_breaktimer::postponeReopensWhileTheTimerKeepsCounting` | S5#10 | ✅ |
| B-09 | Idle during postpone cancels it and resets | `BreakTimerEngine` | `tst_breaktimer::idleDuringPostponeCancelsIt` | S5#11 | ✅ |
| B-10 | Countdown, message, fixed stretching tips with images | `BreakWindow.cpp:27-37`, `resources/icons/stretch/*.svg` (5) | – | S5#5 | 🟡 |
| B-11 | No pre-warning | – | – | – | ✅ Nothing implemented, as decided. |
| B-12 | No sound | – | – | – | ✅ No audio API in `src/`. |
| B-13 | Activity during the break changes nothing | `BreakTimerEngine` | `tst_breaktimer::breakClosesAutomaticallyAndResets` | S5#7 | ✅ |
| B-14 | Enable/disable switch at the top of the tab | `BreakTimerTab` | `tst_breaktimer::disableEndsTheBreakAndResets` | S5#2, S5#15 | ✅ |
| B-15 | Live status every second: work time, next break, state | `BreakTimerTab`, `BreakTimerController` | `tst_breaktimer::workTimeIsReportedAsPausedWithoutInput`, `::cycleStartTimeIsKeptUntilTheCycleResets` | S5#4, F1.7.1#6–7, F1.7.5#2–5 | ✅ |
| B-16 | Sleep/lock count as idle | `BreakTimerEngine`, `IPowerEvents` | `tst_breaktimer::sleepCountsAsIdle`, `::shortSleepKeepsTheCycle`, `::lockCountsAsIdleAndIgnoresActivity`, `::noBreakStartsWhileLocked` | S5#17–19, F1.7.3#2–5 | ✅ |
| B-17 | Optional full-screen suppression, off by default | `BreakTimerEngine`, `MacDesktopIntegration.mm:109`, `WinPlatform.cpp:259`, Linux: `GenericDesktopIntegration` (unsupported → option disabled) | `tst_breaktimer::fullscreenDelaysTheBreakWhenEnabled`, `::fullscreenIgnoredWhenTheOptionIsOff` | S5#21, S5#24 | ✅ Linux unsupported is allowed by B.7 ("best effort"). |

### Part 4 – Stages

| ID | Answer (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|----------------|----------------|--------------|----------------|----------------|
| P-01 | Reorder stages | `implementation_stages.md` stage overview | – | – | ✅ |
| P-02 | Owner tests each stage before continuing | Process; `MANUAL_TESTS.md` | – | all | ✅ Process. |
| P-03 | Rely on CI for Windows/Linux; manual tests when available | `ci.yml` | ctest on 3 OSes | Windows/Debian rows | ⚠️ Windows/Debian manual rows not run yet (O-2). |

---

## Table C – implementation_stages.md Part B

### B.1.1 Window and look & feel

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.1.1-a | Tabs in fixed order | `MainWindow.cpp:47-59` | – | S0#3 | 🟡 |
| B.1.1-b | Fusion + qss, cards, toggle switches, badges, light/dark | `src/ui/Theme`, `resources/style.qss`, `CardWidget`, `ToggleSwitch`, `StatusBadge` | – | S0#25, S5#23, S7#20, F1.7.9#5 | 🟡 |
| B.1.1-c | Tab icons: accent, black when selected | `MainWindow::updateTabIcons`, `src/ui/TabIcons` | – | F1.4.1#8 | 🟡 |
| B.1.1-d | SVG app + tray icons; AutoClick / Drag Lock variants | `resources/icons/app`, `resources/icons/tray/*`, `TrayController::iconFor` | – | S0#6, S3#6, S6#4 | 🟡 |
| B.1.1-e | Statistics placeholder until stage 7 | (removed in 1.8.0) | – | S0#4 | ➖ Replaced by the Statistics tab (B.8). Dead class removed in 1.8.0 (G-a3). |
| B.1.1-f | Slider + spin box; mouse wheel never changes values or combo boxes | `ParameterRow`, `src/ui/widgets/WheelBlocker` (used in `ParameterRow`, `GeneralTab`, `HotkeyDialog`) | – | F1.7.5#6–7a | 🟡 |
| B.1.1-g | Resizable, no maximize/full screen; max 80 %, min 720×560, start 820×620; follows the screen | `src/core/WindowLimits`, `MainWindow.cpp:31-37,71-107,146-151` | `tst_windowlimits::*` | F1.7.8#9–11 | ✅ |
| B.1.1-h | Input diagnostics: live info only, no test buttons | `src/ui/tabs/InputDiagnosticsCard` | – | F1.7.5#8 | 🟡 S2#7, #9–11, #18 describe the removed buttons; marked historical in 1.8.0 (G-a2). |

### B.1.2 Tray, close and exit

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.1.2-a | Close / minimize hide to tray; first-time notification | `MainWindow::closeEvent/changeEvent`, `AppController.cpp:184-197` | – | S0#10–13, F1.1.2#1–7 | 🟡 |
| B.1.2-b | Exit only via Exit button / tray Exit; no confirmation | `AppController::exitApplication`, `MainWindow.cpp:119-138` | – | S0#14, S0#20, S0#22 | 🟡 |
| B.1.2-c | Tray menu: Show, AutoClick on/off, Break Timer on/off, Start break now, Exit | `TrayController`, `AppController.cpp:224-277` | – | S3#15, S5#14–15 | 🟡 |
| B.1.2-d | Click / double-click on tray icon shows the window | `TrayController.cpp:31-34` | – | S0#12 | 🟡 |
| B.1.2-e | macOS: Dock icon only while the window is visible | `MacDesktopIntegration.mm:18-22` | – | S0#10, S0#12, S1#17 | 🟡 |
| B.1.2-f | Single instance; second launch activates first | `SingleInstance`, `main.cpp:68-83` | – | S0#15 | 🟡 |

### B.1.3 General tab

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.1.3-a | Install copies to per-user folder and registers start at login (installed copy) | `InstallController`, `IInstaller` / `IAutostart` implementations | – | S1#22–23 | 🟡 |
| B.1.3-b | macOS: `~/Applications/ErgoPro2.app`; login item via SMAppService (fallback LaunchAgent) | `MacInstaller.cpp:15`, `MacAutostart` | – | S1#22–24 | ⚠️ Only the LaunchAgent is used, by design (`MacAutostart.h:10-13`: SMAppService cannot register the installed copy with `--autostart`). Spec text aligned in 1.8.0 (G-a4). |
| B.1.3-c | Windows: `%LOCALAPPDATA%\Programs\ErgoPro2`, HKCU Run, Start Menu shortcut | `WinPlatform.cpp:23,49-58,98-113` | – | S1#31–33 | 🟡 Not run on Windows yet (O-2). |
| B.1.3-d | Linux: `~/.local/opt/ErgoPro2`, autostart + applications .desktop | `LinuxPlatform.cpp:50-102,129-150` | – | S1#34 | 🟡 Not run on Debian yet (O-2). |
| B.1.3-e | Offer restart from the installed location | `InstallController.cpp:59-60`, `AppController::relaunch` | – | S1#23, S1#27 | 🟡 |
| B.1.3-f | Uninstall; deletion after exit if running copy; settings/logs kept | `InstallController.cpp:73-112`, `src/platform/common/FileOperations` (`scheduleDeletionAfterExit`) | – | S1#26–27, S1#33, F1.7.4#10 | 🟡 |
| B.1.3-g | Status label installed / not installed | `InstallCard` | – | S1#21, S1#23, S1#25 | 🟡 |
| B.1.3-h | Warning when running from temp / Downloads | `LocationClassifier`, `InstallCard.cpp:72-80` | `tst_locationclassifier::classify`, `::realTemporaryFolderIsDetected` | S1#28, F1.1.1#3–4, F1.7.2#2 | ✅ |
| B.1.3-i | Autostart passes `--autostart` | `main.cpp:39-41,50`, autostart writers | `tst_startupmode::decide` | S0#27, S1#17, S1#22 | ✅ |
| B.1.3-j | Windows setup program (Inno Setup), recognized as installed, uninstall via setup | `packaging/windows/ErgoPro2.iss`, `WinPlatform.cpp:63,124` | – | F1.7.4#4–10 | 🟡 Not run on Windows yet (O-2). |
| B.1.3-k | `--quit` option, wait 15 s, exit codes | `main.cpp:42-46,69-73`, `SingleInstance::quitRunningInstance` | – | F1.7.4#7, F1.7.4#11 | 🟡 |
| B.1.3-l | Startup behavior: login / manual options | `GeneralTab::buildStartupCard`, `StartupMode` | `tst_startupmode::decide`, `tst_settings::startupModeSettings` | S1#16–20 | ✅ |
| B.1.3-m | Logging: cumulative level, retention 1–180 days, open log / settings folder | `GeneralTab::buildLoggingCard` (also statistics folder) | `tst_logger::selectionMatrix`, `::retentionIsClamped` | S0#16–18, S1#4–5 | ✅ |
| B.1.3-n | Reset all settings (confirmation; hotkeys cleared and released) | `GeneralTab::resetSettings`, `SettingsStore::resetToDefaults`, `KeyControlController.cpp:129` | `tst_settings::resetToDefaults` | S1#29–30, S4#25, F1.6.1#8 | ✅ |
| B.1.3-o | Application version (window title, tab, tray tooltip) | `CMakeLists.txt:10`, `src/core/Version.h.in`, `MainWindow.cpp:27`, `GeneralTab.cpp:142`, `TrayController.cpp:29` | – | S0#2, S0#5–6, S0#28 | 🟡 |
| B.1.3-p | Permissions / diagnostics: OS, session, permissions with buttons, hook status | `DiagnosticsCard.cpp:74-133`, `PermissionActions` | – | S1#9, S2#2 | 🟡 |
| B.1.3-q | Exit button | `GeneralTab.cpp:116-121` | – | S0#20 | 🟡 |

### B.1.4 OS permissions and feature availability

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.1.4-a | Central FeatureAvailability: available / why / how to fix | `src/core/FeatureAvailability`, `src/ui/FeatureAvailabilityService` | `tst_featureavailability::*` | S1#8 | ✅ |
| B.1.4-b | Banner on each function tab; controls disabled | `FeatureBanner`, `FeaturePage`, tabs | `tst_featureavailability::missingPermissionOnlyAffectsItsFeatures` | S1#8, S3#5, S4#26, S7#21 | ✅ |
| B.1.4-c | macOS modal permissions window, Check again, auto re-check, Restart app | `PermissionsDialog` (`setModal`, `:54-55,122-127`), `FeatureAvailabilityService.cpp:25-62` | – | S1#6–15, F1.1.4#1–5, F1.7.8#2–4 | 🟡 |
| B.1.4-d | Linux X11 full support | `LinuxX11Input.cpp` (XRecord, XTest, XGrabKey) | – | S2#21, S1#34 | 🟡 Not run on Debian yet (O-2). |
| B.1.4-e | Wayland best effort: evdev/uinput, pkexec fix, log out/in hint | `LinuxEvdevInput.cpp`, `LinuxPlatform.cpp:223-341` | – | S1#35–36, S2#22–23 | 🟡 Not run on Debian yet (O-2). |
| B.1.4-f | Windows: no permissions; Administrator note | `WinPlatform.cpp:219`, `WinInput.cpp:368-369` | – | S1#31 | 🟡 |

### B.2 Settings (JSON)

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.2-a | AppConfigLocation/settings.json, org techforge, bundle id | `main.cpp:29-30,88-90`, `CMakeLists.txt:40-41` | – | S0#8 | 🟡 |
| B.2-b | schemaVersion; per-value validation; corrupt file renamed + user notified | `SettingsStore.cpp:92,131`, `AppController::reportSettingsLoad` | `tst_settings::validation`, `::rejectsInvalidAndUnknown`, `::invalidValuesFallBackToDefaults`, `::corruptFileIsBackedUp`, `::nonObjectRootIsCorrupt` | S0#23–24 | ✅ |
| B.2-c | Atomic write (QSaveFile), debounce 300 ms | `SettingsStore.cpp:203,232-233` | `tst_settings::saveIsDebouncedAndReloads`, `::pendingChangesAreSavedOnDestruction` | S0#18 | ✅ |
| B.2-d | All states persist | Schema keys below | `tst_settings::autoClickSettings`, `tst_keycontrol::storeRoundTripAndInvalidEntries` | S3#4, S4#23 | ✅ |
| B.2-e | Keys grouped by section | Dotted keys `section.key` in `src/core/settings/*Settings.h` | `tst_settings::schemaHasStage0Keys` | S0#8 | ✅ |
| B.2-f | `general.startupAutoMode` tray/window, default tray | `GeneralSettings.cpp:17` | `tst_settings::startupModeSettings` | S1#16, S1#20 | ✅ |
| B.2-g | `general.startupManualMode` window/tray, default window | `GeneralSettings.cpp:18` | `tst_settings::startupModeSettings` | S1#16, S1#20 | ✅ |
| B.2-h | `general.logLevel`, default info…critical | `GeneralSettings.cpp:11` | `tst_settings::schemaHasStage0Keys`, `tst_logger::selectionIdsRoundTrip` | S0#5, S0#16–17 | ✅ |
| B.2-i | `general.logRetentionDays` 1–180, default 30 | `GeneralSettings.cpp:13` | `tst_settings::schemaHasStage0Keys`, `tst_logger::retentionIsClamped` | S0#18, S0#24 | ✅ |
| B.2-j | `general.trayHintShown` bool | `GeneralSettings.cpp:15` | `tst_settings::schemaHasStage0Keys` | S0#10–11 | ✅ |
| B.2-k | `autoclick.enabled` default false | `AutoClickSettings.cpp:9` | `tst_settings::autoClickSettings` | S3#2, S3#4 | ✅ |
| B.2-l | `autoclick.triggerTimeMs` 100–3000, default 200 | `AutoClickSettings.h:20-22` | `tst_settings::autoClickSettings` | S3#2–3 | ✅ |
| B.2-m | `autoclick.triggerDistancePx` 5–200, default 15 | `AutoClickSettings.h:23-25` | `tst_settings::autoClickSettings` | S3#2–3 | ✅ |
| B.2-n | `autoclick.jitterTolerancePx` 1–10, default 3 | `AutoClickSettings.h:26-28` | `tst_settings::autoClickSettings` | S3#2–3 | ✅ |
| B.2-o | `autoclick.typingSuspendMs` fixed 500 (constant) | `AutoClickSettings.h:30` | `tst_autoclick::typingDelaysAPendingClick` | S3#13 | ✅ |
| B.2-p | `autoclick.showVisualFeedback` default false | `AutoClickSettings.cpp:13` | `tst_settings::autoClickSettings` | S3#2 | ✅ |
| B.2-q | `keycontrol.actions` list | `KeyControlSettings.cpp:11`, `KeyBinding.cpp:43-91` | `tst_keycontrol::storeRoundTripAndInvalidEntries`, `::loadShowsInvalidEntriesAsErrors` | S4#23–24 | ✅ |
| B.2-r | `draglock.timeoutSeconds` 1–60, default 10, system-only | `KeyControlSettings.h:17-18` | `tst_settings::integerRanges` (engine only: `tst_draglock::timeoutChangeAppliesImmediately`) | F1.6.1#5 | ✅ (G-a1 fixed in 1.8.0) |
| B.2-s | `breaktimer.enabled` default true | `BreakTimerSettings.cpp:9` | `tst_settings::booleanDefaults` | S5#2 | ✅ (G-a1 fixed in 1.8.0) |
| B.2-t | `breaktimer.idleResetMinutes` 2–15, default 5 | `BreakTimerSettings.h:23` | `tst_settings::integerRanges` | F1.7.7#2–4 | ✅ (G-a1 fixed in 1.8.0) |
| B.2-u | `breaktimer.maxWorkMinutes` 20–120, default 50 | `BreakTimerSettings.h:24` | `tst_settings::integerRanges` | F1.7.7#2–4 | ✅ (G-a1 fixed in 1.8.0) |
| B.2-v | `breaktimer.breakMinutes` 3–15, default 4 | `BreakTimerSettings.h:25` | `tst_settings::integerRanges` | F1.6.1#8, F1.7.6#6 | ✅ (G-a1 fixed in 1.8.0) |
| B.2-w | `breaktimer.postponeMinutes` 3–20, default 10 | `BreakTimerSettings.h:26` | `tst_settings::integerRanges` | F1.7.7#2–4 | ✅ (G-a1 fixed in 1.8.0) |
| B.2-x | `breaktimer.suppressWhenFullscreen` default false | `BreakTimerSettings.cpp:14` | `tst_settings::booleanDefaults` | S5#2 | ✅ (G-a1 fixed in 1.8.0) |
| B.2-y | `breaktimer.activityPauseSeconds` 5–60 s, default 20, system-only | `BreakTimerSettings.h:21,27` | `tst_settings::integerRanges` (engine only: `tst_breaktimer::pauseLimitIsConfigurable`) | F1.6.1#11 | ✅ (G-a1 fixed in 1.8.0) |
| B.2-z | `statistics.retentionDays` 1–360, default 90 | `StatisticsSettings.h:13-14` | `tst_settings::integerRanges` | F1.7.1#2–4 | ✅ (G-a1 fixed in 1.8.0) |

### B.3 Logging

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.3-a | Levels DEBUG…CRITICAL | `src/core/LogLevel` | `tst_logger::selectionMatrix` | S0#16 | ✅ |
| B.3-b | Cumulative selection, default level 4 | `LogLevel`, `GeneralTab` | `tst_logger::selectionMatrix`, `::selectionIdsRoundTrip` | S0#5, S0#16–17 | ✅ |
| B.3-c | Daily files, 5 MB roll-over; retention at startup and day change, never on setting change | `Logger` (`Logger.h:16,27`), `AppController.cpp:61-66,110-119` | `tst_logger::rollsOverWhenFileTooLarge`, `::switchesFileOnNewDay`, `::retentionCleanupRemovesOldFiles` | S0#19, F1.7.1#5 | ✅ "Never on setting change" is controller wiring, manual only (F1.7.1#5). |
| B.3-d | Line format | `Logger` | `tst_logger::writesFormattedLinesAndFilters` | S0#9 | ✅ |
| B.3-e | Qt messages via qInstallMessageHandler | `Logger::installQtMessageHandler`, `main.cpp:62` | `tst_logger::qtMessagesAreRouted` | S0#9 | ✅ |
| B.3-f | Thread-safe; console in debug builds | `Logger.h:81` (QMutex), `main.cpp:58-60` | – | – | 🟡 No concurrency test. |

### B.4 Function 1 – AutoClick

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.4-a | Own thread, events from the shared hook | `AutoClickController.cpp:24-26`, `AutoClickWorker` | – | S3#7 | 🟡 |
| B.4-b | Status badge + Activate/Deactivate | `AutoClickTab` | – | S3#2, S3#6 | 🟡 |
| B.4-c | 1. Armed at distance from the reference point | `AutoClickEngine` | `tst_autoclick::armsAtDistanceAndClicksOnceAfterTriggerTime`, `::firstPositionIsTheReference`, `::distanceChangeAppliesToTheNextArming` | S3#7, S3#9 | ✅ |
| B.4-d | 2. Movement > jitter restarts timer; smaller ignored | `AutoClickEngine` | `tst_autoclick::movementRestartsTheTimerButJitterDoesNot` | S3#10 | ✅ |
| B.4-e | 3. Click after TRIGGER_TIMER of stillness; disarm | `AutoClickEngine` | `tst_autoclick::armsAtDistanceAndClicksOnceAfterTriggerTime`, `::triggerTimeChangeWhileArmedAppliesImmediately` | S3#7–8 | ✅ |
| B.4-f | 4. Manual or Key Control click cancels, new reference | `AutoClickEngine` | `tst_autoclick::manualClickCancelsAndMovesTheReference`, `::keyControlClickBehavesLikeAManualClick` | S3#11, S4#16 | ✅ |
| B.4-g | 5. No click while a button is held | `AutoClickEngine` | `tst_autoclick::noClickWhileAButtonIsHeld` | S3#12 | ✅ |
| B.4-h | 6. No click within 500 ms after a key press | `AutoClickEngine` | `tst_autoclick::typingDelaysAPendingClick`, `::modifierKeysDoNotSuspend` | S3#13 | ✅ |
| B.4-i | 7. Drag Lock: no click; reference = release position | `AutoClickEngine` | `tst_autoclick::dragLockSuppressesAndResetsTheReferenceOnRelease` | S6#21–24 | ✅ |
| B.4-j | Works everywhere incl. own windows; no exclusion list | – | – | S3#14, S5#12 | 🟡 |
| B.4-k | Synthetic clicks tagged and ignored | `src/platform/common/Synthetic.h`, `AutoClickEngine` | `tst_autoclick::syntheticEventsAreIgnored` | S3#16 | ✅ |
| B.4-l | Visual feedback: orange X, after half the trigger time of no movement; click-through, no focus | `AutoClickOverlay.cpp:14-19`, `AutoClickEngine` | `tst_autoclick::feedbackWaitsForHalfTheTriggerTimeOfStillness`, `::slowMovementNeverShowsTheFeedback`, `::feedbackHiddenWhileSuppressed` | F1.4.1#9–10, F1.4.2#2–5 | ✅ Disabled with explanation on Wayland (`AutoClickController.cpp:93-102`). |
| B.4-m | Tray icon shows the active state | `AppController.cpp:240-250` | – | S3#6 | 🟡 |
| B.4-n | No sound | No audio API in `src/` | – | – | ✅ |

### B.5.1 Key Control – Bindings

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.5.1-a | Keyboard only | `HotkeyDialog` | – | S4#3 | 🟡 |
| B.5.1-b | F1–F24, numpad, modifier combos; plain key needs confirmation | `KeyComboValidation` | `tst_keycontrol::allowedCombos`, `::plainTypingKeysNeedConfirmation` | S4#7 | ✅ |
| B.5.1-c | Reserved combos per OS rejected with reason (one place); failing registrations rejected | `KeyComboValidation.cpp:20-130` | `tst_keycontrol::reservedCombosPerOs`, `::registrationFailureIsReportedAndNotKept` | S4#6, S4#30 | ✅ |
| B.5.1-d | One combo ↔ one action, one action ↔ one hotkey; duplicates shown as Error | `KeyControlEngine` | `tst_keycontrol::sameActionCannotBeAssignedTwice`, `::availableActionsExcludeAssignedOnes`, `::disabledHotkeyStillOccupiesItsAction`, `::deletingAHotkeyFreesItsAction`, `::modifyCannotTakeAnotherHotkeysAction`, `::storedDuplicateActionsAreErrors` | F1.4.1#2–7 | ✅ |
| B.5.1-e | Exclusive (swallowed by the hook) | `KeyCombo` registry, platform hooks | `tst_keycombo::swallowDownRepeatAndUp`, `::syntheticKeysAreNeverSwallowed` | S4#15 | ✅ |
| B.5.1-f | Once per press, key-down, no repeat | `KeyControlEngine` | `tst_keycontrol::addRegistersAndFiresOncePerPress`, `::modifiersMustMatchExactly` | S4#11 | ✅ |
| B.5.1-g | Works during break window and on ErgoPro2's window | – | – | S4#17, S5#12 | 🟡 |
| B.5.1-h | All actions paused while the Add/Modify dialog is open | `HotkeyDialog.cpp:71,89` | `tst_keycontrol::pauseReleasesEverythingAndResumeRestores` | S4#10 | ✅ |

### B.5.2 Actions list

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.5.2-a | Add flow; failure keeps dialog open | `HotkeyDialog.cpp:274-283` | `tst_keycontrol::registrationFailureIsReportedAndNotKept` | S4#3–9 | ✅ |
| B.5.2-b | Page scrolls; list full height; warning above list | `KeyControlTab` | – | F1.6.1#2–4 | 🟡 |
| B.5.2-c | Columns + Enable/Disable, Modify, Reset, Delete (confirm) | `KeyControlTab` | `tst_keycontrol::modifyChangesKeyAndAction`, `::failedModifyKeepsTheWorkingBinding`, `::removeReleases` | S4#18–22 | ✅ |
| B.5.2-d | Disable releases the key | `KeyControlEngine` | `tst_keycontrol::disableReleasesAndEnableRegistersAgain` | S4#18 | ✅ |
| B.5.2-e | Reset binding = unregister + re-register | `KeyControlEngine` | `tst_keycontrol::resetUnregistersAndRegistersAgain` | S4#21 | ✅ |
| B.5.2-f | Pluggable registry: left, right, double, drag lock, toggle AutoClick | `KeyAction.cpp:46-65` | `tst_keycontrol::actionRegistry`, `::allActionsDispatch`, `::allActionsCanHaveAHotkeyIncludingDragLock` | S4#9, S6#2 | ✅ |
| B.5.2-g | Clicks at the current pointer position | `…ClickInjector` | – | S4#11–13 | 🟡 |
| B.5.2-h | Stored as `{id, keyCombo, nativeInfo (optional), action, enabled}` | `KeyBinding.cpp:24,43-91` | `tst_keycontrol::storeRoundTripAndInvalidEntries`, `::portableTextRoundTrip` | S4#23–24 | ✅ Optional `nativeInfo` is not written (not needed). |

### B.5.3 Drag Lock

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.5.3-a | Hotkey → left button down at current position | `DragLockEngine`, `DragLockController` | `tst_draglock::hotkeyPressesAndHolds`, `::toggleUsesThePointerWhenNoEventWasSeen`, `::failedPressLeavesIdle` | S6#4, S6#6–10 | ✅ |
| B.5.3-b | Released by the hotkey again (swallowed) | `DragLockEngine` | `tst_draglock::hotkeyAgainReleasesAtTheCurrentPosition`, `tst_keycontrol::dragLockHotkeyTogglesWithoutReleasingFirst` | S6#5 | ✅ |
| B.5.3-c | Released after draglock.timeoutSeconds without movement | `DragLockEngine` | `tst_draglock::timeoutSinceTheLastMovement`, `::wheelDoesNotReleaseNorResetTheTimeout` | S6#11 | ✅ |
| B.5.3-d | Non-modifier key releases (passed on); modifiers do not | `DragLockEngine` | `tst_draglock::nonModifierKeyReleases`, `::modifiersAloneDoNotRelease`, `::swallowedHotkeysAndRepeatsAreLeftToTheirAction`, `tst_keycontrol::otherHotkeysReleaseTheDragLockBeforeTheirAction` | S6#12–13, S6#16, S6#18 | ✅ |
| B.5.3-e | Any mouse click releases and is consumed | `DragLockEngine`, `MacInput.mm:336`, `WinInput.cpp:130` | `tst_draglock::anyMouseButtonReleases`, `::clickIsConsumedByTheInputLayer` | S6#14–15, S6#27 | ⚠️ Not consumed on Linux (O-1). |
| B.5.3-f | Break window releases | `AppController.cpp:145-146` | `tst_draglock::applicationReleases` | S6#17 | ✅ |
| B.5.3-g | App exit, AutoClick / Key Control disabled release; never stuck | `DragLockController.cpp:91-104`, `KeyControlController.cpp:178,279`, `src/platform/common/EmergencyRelease` (signals) | `tst_draglock::applicationReleases`, `::failedReleaseIsReported` | S6#19–20 | ✅ Signal path manual only. |
| B.5.3-h | No AutoClick while held; reference = release position | `AutoClickEngine` | `tst_draglock::autoClickDoesNotClickWhileHoldingNorAfterReleaseUntilTheDistanceIsMoved`, `::autoClickReferenceAfterAConsumedClickRelease` | S6#21–25 | ✅ |
| B.5.3-i | Tray icon shows drag lock | `TrayController.cpp:71-72` | – | S6#4–5 | 🟡 |
| B.5.3-j | draglock.timeoutSeconds system-only, no card on the tab | `KeyControlTab` (no card), `KeyControlSettings.h:17` | – | F1.6.1#5 | 🟡 S6#3 describes the removed card; marked historical in 1.8.0 (G-a2). |

### B.5.4 Binding robustness

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.5.4-a | One shared hook, no thread per action | `InputService`, platform hooks | `tst_dispatcher::*` | S2#1–6 | ✅ |
| B.5.4-b | macOS: re-enable tap after timeout/user-input disable | `MacInput.mm:262-270` | – | S2#17, S4#28 | 🟡 |
| B.5.4-c | macOS: active tap never without Accessibility; checked every second; hook restarts | `MacInput.mm:249-252,416` (trust timer), `MacInput.mm:478`, `InputService.cpp:25` | – | F1.7.8#2–4 | 🟡 |
| B.5.4-d | Windows: fast callback; watchdog reinstalls a dead hook | `WinInput.cpp` (hook thread, `:245`), `HookWatchdog` | `tst_watchdog::unhealthyHookIsReinstalled` | S2#19–20 | ✅ (core); Windows manual pending (O-2). |
| B.5.4-e | Linux: re-grab on MappingNotify; evdev hotplug | `LinuxX11Input.cpp:352-371`, `LinuxEvdevInput.cpp:187-221` | – | S2#21–22 | 🟡 Debian manual pending (O-2). |
| B.5.4-f | All OSes: reinstall after resume and unlock, logged | `InputService.cpp:160-168`, `HookWatchdog` | `tst_watchdog::resumeAndUnlockReinstallButNotTwiceInARow`, `tst_keycontrol::reapplyAfterHookReinstall` | S2#15–16, S4#27, F1.7.3#4 | ✅ |
| B.5.4-g | Health check every 30 s; failures logged + shown | `HookWatchdog.h:20`, `InputService.cpp:145` | `tst_watchdog::persistentFailureAfterRepeatedFailedReinstalls`, `::healthyCheckClearsFailures` | S2#17 | ✅ |

### B.6 Shared input layer

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.6-a | One hook thread, normalized events with monotonic timestamp | `src/core/input/InputEvent`, `MacInput.mm:385`, `WinInput.cpp:245`, `LinuxX11Input.cpp:325`, `LinuxEvdevInput.cpp:136` | `tst_inputevent::*`, `tst_clock::systemClockIsMonotonic` | S2#4–6 | ✅ |
| B.6-b | Queued distribution; synchronous swallow decision | `src/core/input/InputDispatcher`, `KeyCombo` registry | `tst_dispatcher::deliversInOrderFromAnotherThread`, `::deliveryIsAsynchronous`, `::dropsWhenTheReceiverCannotKeepUp`, `tst_keycombo::concurrentUseIsSafe` | S2#5 | ✅ |
| B.6-c | Click injector incl. double click with OS interval | `…ClickInjector` (mac/win/linux) | – | S4#11–13 | 🟡 |
| B.6-d | Synthetic tagging per OS | `Synthetic.h`, `MacInput.mm:279,525`, `WinInput.cpp:81,107,363`, `LinuxX11Input.cpp:271` | `tst_keycombo::syntheticKeysAreNeverSwallowed`, `tst_autoclick::syntheticEventsAreIgnored`, `tst_statistics::syntheticEventsAreNotActivity` | S3#16 | ✅ |
| B.6-e | Logical pixels, multi-monitor; power/session events | Platform hooks, `IPowerEvents` (`LinuxPowerEvents.cpp`, mac/win) | `tst_macpowerevents::notificationsAfterStartReturnedReachTheCallback`, `::stopRemovesTheObservers` (macOS) | S2#4, S2#15–16 | ✅ |

### B.7 Function 3 – Break Timer

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.7-a | Tab: toggle + badge, status panel every second (work time, Work started at, next break, state incl. Paused); no break-now/reset buttons | `BreakTimerTab`, `BreakTimerController` | `tst_breaktimer::cycleStartTimeIsKeptUntilTheCycleResets`, `::workTimeIsReportedAsPausedWithoutInput`, `::startBreakNow` (tray only) | S5#2, S5#4, F1.6.1#7, F1.7.1#6–7, F1.7.5#2–5 | ✅ |
| B.7-b | INTERACTION_TIMER = active work time; stops after ACTIVITY_PAUSE; nothing counts during break | `BreakTimerEngine` | `tst_breaktimer::interactionTimerCountsOnlyWhileInputContinues`, `::gapsShorterThanThePauseLimitCountFully`, `::pauseLimitIsConfigurable`, `::noBreakWhileTheWorkTimeIsPaused`, `::postponeCountsFromThePostponeClick` | F1.6.1#9–11 | ✅ |
| B.7-c | Activity = real input; synthetic excluded | `BreakTimerController.cpp:158` | – | S5#4 | 🟡 |
| B.7-d | Idle reset; sleep and lock count as idle | `BreakTimerEngine` | `tst_breaktimer::idleResetCountsAsABreak`, `::sleepCountsAsIdle`, `::lockCountsAsIdleAndIgnoresActivity` | S5#16–18 | ✅ |
| B.7-e | Break at MAX_WORK_TIME; half time while usage > 100 % | `BreakTimerEngine`, `AppController.cpp:149-150` | `tst_breaktimer::breakAfterMaxWorkTime`, `::halfWorkTimeWhileUsageIsOverHundredPercent`, `::goingOverHundredPercentPastHalfTimeShowsTheBreakAtOnce` | S5#5, F1.7.6#5, F1.7.8#8 | ✅ |
| B.7-f | Descriptive texts only, no capital variable names | `BreakTimerTab`, `AutoClickTab` | – | F1.7.6#7 | 🟡 |
| B.7-g | Overlay on all monitors, on top, not closable (Esc/Alt+F4/Cmd+W); task switcher not blocked | `BreakWindow.cpp:50,182-200`, `BreakWindowManager.cpp:52-68` | – | S5#5–6, S5#13, S5#22, S5#24 | 🟡 |
| B.7-h | Message, countdown, 4–6 stretching SVGs, Dismiss / Postpone (N min) | `BreakWindow.cpp:27-37`, `resources/icons/stretch/*.svg` (5) | – | S5#5 | 🟡 |
| B.7-i | Countdown zero → closes, resets | `BreakTimerEngine` | `tst_breaktimer::breakClosesAutomaticallyAndResets` | S5#8 | ✅ |
| B.7-j | Dismiss → closes, resets | `BreakTimerEngine` | `tst_breaktimer::dismissResetsImmediately` | S5#9 | ✅ |
| B.7-k | Postpone; unlimited; idle during postpone cancels | `BreakTimerEngine` | `tst_breaktimer::postponeReopensWhileTheTimerKeepsCounting`, `::idleDuringPostponeCancelsIt` | S5#10–11, F1.7.1#7 | ✅ |
| B.7-l | Activity during break does nothing | `BreakTimerEngine` | `tst_breaktimer::breakClosesAutomaticallyAndResets` | S5#7 | ✅ |
| B.7-m | AutoClick / hotkeys on the break window; drag lock released | `AppController.cpp:145-146` | `tst_draglock::applicationReleases` | S5#12, S6#17 | ✅ |
| B.7-n | No pre-warning, no sound | – | – | – | ✅ |
| B.7-o | Optional full-screen suppression (best effort; disabled with explanation when unsupported) | `BreakTimerEngine`, `isFullscreenAppActive` (mac, win; Linux unsupported) | `tst_breaktimer::fullscreenDelaysTheBreakWhenEnabled`, `::fullscreenIgnoredWhenTheOptionIsOff` | S5#21, S5#24 | ✅ |
| B.7-p | Disabling closes the window and resets | `BreakTimerEngine` | `tst_breaktimer::disableEndsTheBreakAndResets` | S5#15, F1.7.5#5 | ✅ |
| B.7-q | MAX_WORK_TIME change applies to the current cycle | `BreakTimerEngine` | `tst_breaktimer::maxWorkTimeChangeAppliesToTheCurrentCycle` | S5#20, F1.7.6#3 | ✅ |

### B.8.1 Statistics – what is recorded

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.8.1-a | Real input only; nothing while not running / asleep / locked | `StatisticsCollector.cpp:147` | `tst_statistics::syntheticEventsAreNotActivity`, `::sleepAndLockCountAsNoInput` | S7#11 | ✅ |
| B.8.1-b | Privacy: counts and durations only | `StatisticsTypes`, `StatisticsStore` (CSV columns) | `tst_statistics::countersAndPrivacy`, `tst_statisticsstore::rowFormatAndRoundTrip` | S7#13 | ✅ |
| B.8.1-c | Mouse / keyboard / active seconds | `StatisticsCollector` | `tst_statistics::secondsAreClassifiedAsMouseKeyboardOrBoth` | S7#3 | ✅ |
| B.8.1-d | Mouse clicks left/right/other | `StatisticsCollector.cpp:174-181` | `tst_statistics::countersAndPrivacy` | S7#8 | ✅ |
| B.8.1-e | Clicks done for you (double = 2, drag press = 1) | `StatisticsController.cpp:91-93` | `tst_statistics::countersAndPrivacy` | S7#8 | ✅ |
| B.8.1-f | Key presses without repeat; modifiers and hotkeys count | `StatisticsCollector.cpp:190-193` | `tst_statistics::countersAndPrivacy` | S7#9 | ✅ Hotkey presses not separately unit-tested. |
| B.8.1-g | Wheel events (totals only) | `StatisticsCollector.cpp:186-188` | `tst_statistics::countersAndPrivacy` | S7#3 | ✅ |
| B.8.1-h | Mouse distance in px and cm (screen DPI, fallback 96) | `StatisticsCollector.cpp:164-170`, `StatisticsController.cpp:26-34` | `tst_statistics::mouseDistance` | S7#10 | ✅ |
| B.8.1-i | Day boundary local midnight; percent not reset | `StatisticsCollector` | `tst_statistics::midnightStartsANewDayButKeepsThePercent` | S7#19 | ✅ |

### B.8.2 Computer usage percent

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.8.2-a | Usage % = used time (like Break Timer work time) ÷ MAX_WORK_TIME | `StatisticsCollector` | `tst_statistics::hundredPercentAtMaxWorkTimeOfContinuousUse`, `::pausesCountUpToTheActivityPauseLikeTheWorkTime` | S7#5, F1.7.8#5 | ✅ |
| B.8.2-b | Reset after BREAK_TIME without input and when the break completes | `StatisticsCollector` | `tst_statistics::resetsOnlyAfterTheBreakTimeWithoutInput`, `::anEventOnlyMinuteAfterTheRestShowsZero`, `::completedBreakResetsTheUsageEvenWithInputDuringTheBreak` | S7#7, F1.7.6#4 | ✅ |
| B.8.2-c | Dismiss / Postpone do not reset | `StatisticsCollector` | `tst_statistics::growsBeyondHundredAndBreakButtonsDoNotReset`, `::dismissAndPostponeDoNotReset` | S7#6 | ✅ |
| B.8.2-d | Max 500 % → reset to 0, new cycle | `StatisticsCollector.h:44` | `tst_statistics::aboveFiveHundredPercentResetsToZero` | F1.7.8#8 | ✅ |
| B.8.2-e | Calculated also when the Break Timer is disabled | `StatisticsController` | – | S7#12 | 🟡 |
| B.8.2-f | New MAX_WORK_TIME applies at a new work period (or at once if 0) | `StatisticsCollector` | `tst_statistics::maxWorkChangeKeepsThePercentUntilTheUsageRestarts`, `::maxWorkChangeAppliesAtOnceWhileTheUsageIsZero`, `::workCycleResetAppliesTheNewScale` | F1.7.6#2, F1.7.8#6–7 | ✅ |
| B.8.2-g | Recorded per minute; empty minutes not 0 | `StatisticsCollector` | `tst_statistics::onlyMinutesWithDataAreReported` | S7#3 | ✅ |

### B.8.3 Statistics tab

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.8.3-a | Layout 1.7.9: fixed header, one day card with dividers, no descriptions | `src/ui/tabs/StatisticsTab` | – | F1.7.9#2–3, F1.7.9#6 | 🟡 |
| B.8.3-b | Header date + slim navigation | `StatisticsTab` | – | F1.7.9#2 | 🟡 |
| B.8.3-c | Chart 1: usage line, hour range, 0–100…500 % axis, 100 % line, gaps, event markers, tooltip | `src/ui/statistics/StatisticsCharts`, `src/core/statistics/ChartAxis` | `tst_statistics::hourRangeFromFirstToLastData`, `::percentAxisScaling`, `::niceCeilings`, `::breakEventsAreRecordedInTheirMinute` | S7#3–7 | ✅ (axis logic); drawing manual. |
| B.8.3-d | Chart 2: 3 bar charts, 5-min buckets, tooltips | `StatisticsCharts`, `ChartAxis` | `tst_statistics::fiveMinuteBucketing` | S7#3–4 | ✅ |
| B.8.3-e | Daily totals tiles | `StatisticsTab`, `StatisticsTypes` | `tst_statistics::totals` | S7#8–10 | ✅ |
| B.8.3-f | Navigation skips days without file; buttons disabled; live per minute | `StatisticsController`, `StatisticsStore` | `tst_statisticsstore::navigationSkipsDaysWithoutAFile` | S7#17, F1.7.9#4 | ✅ |
| B.8.3-g | Options card: folder + Keep statistics 1–360 days | `StatisticsTab.cpp:182-186` | – | F1.7.1#2–4 | 🟡 |
| B.8.3-h | No on/off; banner when the hook does not run | `FeaturePage(Feature::Statistics)` in `MainWindow.cpp:57` | `tst_featureavailability::missingPermissionOnlyAffectsItsFeatures` | S7#21 | ✅ |
| B.8.3-i | Charts drawn with QPainter (no Qt Charts) | `StatisticsCharts`, `CMakeLists.txt:64` (no Charts module) | – | – | 🟡 |

### B.8.4 Files

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.8.4-a | Folder AppDataLocation/statistics; shown on General tab | `StatisticsController`, `GeneralTab.cpp:194,204` | – | S7#13 | 🟡 |
| B.8.4-b | One CSV per day, header and columns as specified | `src/core/statistics/StatisticsStore` | `tst_statisticsstore::fileNames`, `::rowFormatAndRoundTrip`, `::writeHeaderOnceAndReadBack` | S7#13 | ✅ |
| B.8.4-c | Written per minute and on exit; partial line repaired; bad lines skipped | `StatisticsStore`, `AppController::exitApplication` | `tst_statistics::flushReportsThePartialMinute`, `tst_statisticsstore::partialLastLineIsRepaired`, `::partialLineIsRepairedBeforeAppending`, `::invalidLinesAreSkipped` | S7#14–16 | ✅ |
| B.8.4-d | Restart the same day continues the totals | `StatisticsCollector`, `StatisticsStore` | `tst_statistics::restoreContinuesTheDayAndThePercent`, `::dayMergesRecordsOfTheSameMinute`, `tst_statisticsstore::sameMinuteTwiceIsMerged` | S7#14 | ✅ |
| B.8.4-e | Retention at startup and midnight only, never on setting change | `StatisticsController.cpp:112-115,276` | `tst_statisticsstore::retentionCleanup` | F1.7.1#2–4 | ✅ S7#18 expects immediate deletion; marked historical in 1.8.0 (G-a2). |

### B.9 Releases

| ID | Rule (short) | Implementation | Unit test(s) | Manual test(s) | Status / notes |
|----|--------------|----------------|--------------|----------------|----------------|
| B.9-a | CI creates release `ErgoPro2_v<version>` once per version | `.github/workflows/ci.yml:180-243` | – | F1.7.4#2–3 | 🟡 |
| B.9-b | Release text = CHANGELOG section + download table | `ci.yml` (notes.md step) | – | F1.7.4#2 | 🟡 |
| B.9-c | Files: .dmg, setup .exe, .deb (zip stays an artifact) | `ci.yml:243` | – | F1.7.4#2 | 🟡 |

---

## Gaps

> **Status after the 1.8.0 fixes:** G-a1 (settings tests `tst_settings::integerRanges` /
> `::booleanDefaults`), G-a2 (historical-rows note at the top of `MANUAL_TESTS.md` + Stage 8 checklist),
> G-a3 (`PlaceholderPage` deleted) and G-a4 (B.1.3 text aligned) are **fixed**. `USER_GUIDE.md` and
> `ARCHITECTURE.md` now exist. O-1 and O-2 remain open for the owner, together with the review items listed
> in the 1.8.0 final report (CHANGELOG 1.8.0).

Row counts: Table A 39 rows, Table B 101 rows, Table C 171 rows.

### (a) Small fixes within the existing scope

| ID | Rows | What is missing | Suggested fix |
|----|------|-----------------|---------------|
| G-a1 | C-04, B.2-r … B.2-z | `tests/tst_settings.cpp` only checks the General and Auto Click schema entries. The ranges/defaults of `breaktimer.*` (incl. the 1.7.6/1.7.7 range changes and break default 4), `draglock.timeoutSeconds` and `statistics.retentionDays` (`src/core/settings/BreakTimerSettings.h:23-27`, `KeyControlSettings.h:18`, `StatisticsSettings.h:14`) have no unit test; an old value outside the new range (F1.7.7#4) is only checked manually. | Add `tst_settings::breakTimerSettings`, `::keyControlAndDragLockSettings`, `::statisticsSettings`: default, min, max, out-of-range value falls back to the default with a warning. |
| G-a2 | B.1.1-h, B.5.3-j, B.8.4-e | `MANUAL_TESTS.md` rows that no longer match the app: S2#7, S2#9–11, S2#18 (test buttons removed in 1.7.5); S3#17–19 ("ring", now a cross since 1.4.1); S5#2 (break default 5, now 4) and the S5 short-value tip (impossible since 1.7.7); S6#3 (Drag lock card removed in 1.6.1); S7#18 (immediate deletion, changed in 1.7.1). | Mark them as historical in the stage sections and cover the current behavior in the Stage 8 full regression checklist. |
| G-a3 | B.1.1-e | `src/ui/widgets/PlaceholderPage.{h,cpp}` is compiled (`src/CMakeLists.txt:157`) but no longer used since the Statistics tab exists. | Delete the class and its CMake entry. |
| G-a4 | B.1.3-b | Part B says the macOS login item uses SMAppService with a LaunchAgent fallback; the code uses only the LaunchAgent, with a documented reason (`src/platform/mac/MacAutostart.h:10-13`). Behavior is correct and tested manually (S1#22–24). | With the owner's OK, align the B.1.3 macOS bullet (`documentation/implementation_stages.md:135`) with the implementation. |

### (b) Larger items for the owner

| ID | Rows | What is missing | Suggested fix / decision needed |
|----|------|-----------------|--------------------------------|
| O-1 | D-03, B.5.3-e | On Linux the click that releases a drag lock cannot be consumed: X11 (XRecord) can only observe pointer events, and on Wayland the pointer devices are deliberately not grabbed (`src/platform/linux/LinuxEvdevInput.cpp:161-163`, `src/platform/PlatformInterfaces.h:48`). Documented as a known limitation in S6#27. | Accept as a documented limitation (USER_GUIDE), or approve new work: an XInput2 pointer grab on X11 / an evdev pointer grab with uinput forwarding on Wayland (risk: losing the mouse if forwarding breaks). |
| O-2 | BD-6, P-03, and all rows marked "manual pending" | The Windows 11 and Debian (X11/Wayland) manual rows have never been run: S1#31–36, S2#19–23, S3#22, S4#30, S5#24, S6#26–27, S7#22, F1.7.2#4–6, F1.7.4#4–10. Those platforms are only compiled and unit-tested by CI. | The owner runs the Stage 8 regression checklist on Windows 11 and Debian 13 (X11 and Wayland) when machines are available. |

Note (not a requirement row): the other Stage 8 documents are `documentation/USER_GUIDE.md` and
`documentation/ARCHITECTURE.md`.
