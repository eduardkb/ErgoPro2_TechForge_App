# Todo Items

## Open:

## Completed to verify:


## Improvement and new features suggestions
- when installing the .dmg file double clicking it. There was no clear indication I should drag the app's icon to the application folder and no message on the install screen to do so. also, although the app moved to the application folder when I drag and dropped it there was no indication that it got installed. there must be some sort of confirmation. and, when successfully installed display a message offering to delete the .dmg (it should delete it and un-mount the .dmg if the user confirms deletion.). this is also currently not happneing. make a new minor version

| # | Feature | Need | Effort | Why / how it fits ErgoPro2 |
|---|---|---|---|---|
| 1 | **Micro-breaks**: a second, smaller timer (e.g. 20–30 s pause every 5–10 min of active use), with a small non-blocking overlay | High | Low–Med | Workrave's main feature. Reuses the engine's active-time logic and the break window. |
| 2 | **Guided wrist/hand exercises in the break window**: rotating text and illustrations (wrist flexor/extensor stretch, tendon glides, shoulder rolls) | High | Low | Turns idle break time into active recovery. Static content only, no new logic. |
| 3 | **Advance warning + gentle mode**: a tray notification about 1 min before a break ("Break in 60 s") and an option for a non-blocking reminder instead of a fullscreen one | High | Low | People abandon apps that interrupt mid-sentence. A small timer and settings change. |
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
