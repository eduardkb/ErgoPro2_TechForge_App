You are fixing a bug in the ErgoPro2 desktop application (C++20 / Qt 6 Widgets / CMake; Windows 11, macOS 26 Tahoe, Debian 13).

FIRST read:
- documentation/implementation_stages.md – Part A (mandatory rules: architecture, versioning, coding, testing, workflow) and Part B (consolidated functional specification, which defines the CORRECT behavior).
- documentation/initial development docs/ai_questions.md (owner's "Answer:" and "REVIEW_RESULT:" decisions) and documentation/initial development docs/baseDescription.md when the behavior in question needs more context.
- CHANGELOG.md, to see the current version and recent changes.
Then study the code related to the bug.

PROCESS:
1. Understand: restate the bug in one or two sentences and identify the expected behavior from Part B. If the report describes behavior that actually matches the specification (i.e. it is a change request, not a bug), or if the expected behavior is ambiguous, STOP and ask the owner before changing anything.
2. Locate the root cause. Use the provided logs, code reading and, where possible, a reproduction. Explain the root cause clearly; do not just patch the symptom.
3. Write a failing unit test first whenever the bug is in testable logic (src/core with the fake clock, settings, validation, state machines). If it cannot be unit-tested (OS hook, UI, packaging), explain why and add precise manual test steps instead.
4. Fix it with the smallest clean change that respects the architecture (core / platform / ui separation, no #ifdef outside src/platform and the factory, no silent error handling, tr() for UI strings, no UI access from worker threads). If the same defect clearly exists in the other OS implementations, fix it there too and say so.
5. Do not refactor unrelated code, do not add features, and do not change behavior that the specification defines.
6. Make sure there is enough logging around the fixed area to diagnose a recurrence (correct log levels).
7. Versioning: bump ONLY the PATCH number of project(ErgoPro2 VERSION x.y.z) in the root CMakeLists.txt (e.g. 1.3.0 → 1.3.1). MAJOR stays 1, MINOR unchanged. Exactly one bump per run, even if several related issues are fixed.
8. Add a CHANGELOG.md entry at the top: "## [x.y.z] - YYYY-MM-DD" with a "### Fixed" list describing the bug from the user's point of view (and the root cause in one short sentence).
9. Add a regression check to documentation/initial development docs/MANUAL_TESTS.md under a heading "## Bug fix – vx.y.z" (steps + expected result).
10. Build the project and run ctest. All tests must pass.
11. Do NOT run git commit, git push or create branches.

FINAL REPLY:
- Root cause.
- Fix description.
- Files changed.
- New version number.
- Tests added and build/test results (relevant output).
- Manual test steps to verify the fix on the affected OS.
- Risks, side effects, or anything not verified (e.g. Windows/Linux only compile-checked by CI).

BUG REPORT (filled in by the owner):
- Break timer tab - status box: between active work time and next break in insert a new field that shows the hour (24h format), minute and second when the active work timer started last. it can show "-" if it was reset and user did not start any action yet. I want it to show the time the user last started a cycle and if the work cycle was reset it shows "-".
- change all sliders so they don't change their value with mouse scroll. mouse scroll only scrolls right throug the sliders. only mouse click or typing the value on the text box changes their value.
- on general tab - input diagnostics box: remove both buttons ("Test left click" and "Test: Bloxk F9...") at the end of the box