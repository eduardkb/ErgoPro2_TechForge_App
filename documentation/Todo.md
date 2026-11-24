# Todo Items

## Open:


## Completed to verify:
- Basic Description:
    1. **Micro-breaks**: a second, smaller timer (e.g. 20–30 s pause every 5–10 min of active use), with a small non-blocking overlay.
    2. new tab "Micro-breaks" contains all configuration variables to be configured and on top a toggle button to activate/deactivate the feature. try to maintain all tabs on the minimum width size. if not possible, allow user to scroll horizontally when the screen is too small.
    3. the two timer values should be configurable thourgh the tab or the .json file. Both the "pause every x seconds" and "maintain the screen displayed for y seconds". 
    4. It should display a small window on a corner (preferably close to the tray icon). it should be a non-blocking screen with a suggestion of a small break. clicking it dismisses it. it should be on top of everything always until it is dismissed or its pause time elapses. also should show a small timer on its bottom right corner counting the time for it to disappear.

-  if "Keep screen active" maintains the screen active for more than 4 hours non-stop, disable the feature and let the screen sleep. create a new variable that is only on the .json config file where user can set this time in hours. Minimum is 4, maximum is 25. if 25 or more is set, it never disables this feature.

- in MacOS, if accessibility permission is removed in settings while the app is running, the mouse is blocked from taking any actions (left clicks do not work.). make sure the app does not block this if it looses its permissions.
- maintain the tray icon colors as they are now (transparent if auto click not active and colored if it is arctive) and add one more status for the "keep screen active" functionality. If it is active, a small related colored symbol should show on the bottom right corner of the icon. if it is unactive, it should disappear. this will probably need to be implemented with several icons because each feature can be enabled/disabled independently.

- from "warnings and errors in the log" card inside "general" tab remove warnings. only errors and critical errors should be shown there.
- After implementing new functions below, bump minor version up making new version 2.3.0.
    1. Base description: implement a tray notification about 1 min before a break ("Break in 60 s"). the small reminder window shown should contain a countdown timer until the break window shows.
    2. create necessary variables in the .json file and allow changing them on the "Break Timer" tab. The show notification x minutes before the break window should be configurable in seconds (from minimum 15 seconds to maximum 5 minutes). default 1 minte. This reminder should always be only a reminder. non-blocking. if clicked it is dismissed and does not show anymore for the same period.

- when installing the .dmg file double clicking it. There was no clear indication I should drag the app's icon to the application folder and no message on the install screen to do so. also, although the app moved to the application folder when I drag and dropped it there was no indication that it got installed. there must be some sort of confirmation. and, when successfully installed display a message offering to delete the .dmg (it should delete it and un-mount the .dmg if the user confirms deletion.). this is also currently not happneing. make a new minor version

## Improvement and new features suggestions

| # | Feature | Need | Effort | Why / how it fits ErgoPro2 |
|---|---|---|---|---|
| 1 | **Micro-breaks**: a second, smaller timer (e.g. 20–30 s pause every 5–10 min of active use), with a small non-blocking overlay | High | Low–Med | Workrave's main feature. Reuses the engine's active-time logic and the break window. |
| 2 | **Guided wrist/hand exercises in the break window**: rotating text and illustrations (wrist flexor/extensor stretch, tendon glides, shoulder rolls) | High | Low | Turns idle break time into active recovery. Static content only, no new logic. |
| 3 | **IMPLEMENTED - Advance warning + gentle mode**: a tray notification about 1 min before a break ("Break in 60 s") and an option for a non-blocking reminder instead of a fullscreen one | High | Low | People abandon apps that interrupt mid-sentence. A small timer and settings change. |
| 4 | **Pause from the tray**: "Pause breaks for 30 min / 1 h / until tomorrow" (today the tray only has Show and Exit) | High | Low | Meetings and presentations are the main reason people quit these apps. |
| 5 | **Break history in Statistics**: breaks shown, completed, dismissed and postponed per day, plus a "breaks taken" percentage tile | Med | Very low | The events are already in the CSV. This is only new totals and one tile. |
| 6 | **Daily usage limit**: a warning (and optional long break) after X hours of active mouse/keyboard time per day | Med–High | Low | Statistics already counts active seconds. Workrave has this; most others don't. |
| 7 | **Pain/discomfort log**: a quick 0–10 rating (prompted once a day or after breaks), shown alongside usage in Statistics | High for tendinitis | Low–Med | Shows which days or work patterns hurt. Very few competitors offer it, so it would set ErgoPro2 apart. |
| 8 | **More Key Control actions**: one key sends a shortcut (Ctrl+C, Ctrl+V, Ctrl+Z, Alt+Tab…), scroll up/down, middle click | High for wrists | Medium | Replaces painful multi-finger shortcuts with single keys. Needs key injection on each OS. |
| 9 | **Auto-pause during fullscreen apps and video calls** | Med–High | Medium | Different detection code for each OS (`#ifdef` platform layer). Builds on #4. |
| 10 | **Intensity-aware breaks**: the break comes sooner when clicks and key presses per minute are high | Med | Medium | RSIGuard and Workpace do this. The per-minute counts already exist. |
| 11 | **Weekly/monthly trends**: active time, clicks, breaks taken and pain over weeks, plus CSV export of a date range | Med | Medium | The data is already in the daily CSVs. Needs new chart views. |
| 12 | **Auto Click click-type toolbar**: a small floating bar to pick the next dwell click (right / double / drag) | High for heavy mouse users | Med–High | Like OS dwell control. An always-on-top overlay plus focus handling on 3 OSes. |
| 13 | **Eye and posture reminders**: 20-20-20 eye rule, "check your posture", water | Low–Med | Low | Easy, but not focused on wrists. Better as an option added to #1. |
| 14 | **Profiles** (Work / Home / Gaming) switching all settings | Low–Med | Medium | Useful but rarely asked for. Means reworking the settings file. |
| 15 | **Voice commands/dictation** | High value | Very high | Leave it to the OS (Windows Voice Access, macOS Dictation/Voice Control). At most add a "Tips" link to them in the General tab. |
