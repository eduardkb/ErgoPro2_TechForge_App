# Todo Items

## Open:
- Implement new hidden feature - keep screen active. Description:
    - New function description: If enabled and active it waits for the inputs to get still (ammount of time configured on variable explained below) then immediatelly after this time runs out and after each cycle do this in order:
        1. verify if teams is a open process. if yes, left click on its title bar and finish interaction.
        2. verify if zoom is a open process. if yes, left click on its title bar and finish interaction.
        3. if none of above is present, minimize all open windows showing the desktop and click the desktop with one left click in any empty space.
    - on tab key control, add a new option on the "actions" menu. this action if added as a hotkey will enable the functionality. The configured hotkey will toggle this functionality between enabled/disabled. clicking disable on the hotkey table will also disable the functionality. 
    - add a card on the bottom of the Auto Click tab for this feature (it should only show if configured on the key control tab). on top it should say if the feature is enabled or disabled, what key is configured to do this action and its status ("idle" = active but not doing anything, "working" = active and working when input has stopped, "disabled" when disabled"). no additional items are necessary on this card.
    - create a new variable on the .json config file. it will control how many minutes the app leaves betwen left clicks to maintain the screen active. This variable will only be changeable on the .json file itself if needed. the interface will not mention it.
    - create a new variable on the .json config file. it will control how many minutes after the inputs went still before starting clicking to maintain the screen active. This variable will only be changeable on the .json file itself if needed. the interface will not mention it.
    - the mouse clicks and movements of this feature should not count as mouse movement or clicks for statistics or break timer functions




## Completed to verify:
- statystics tab - Day text. when today format "Today - <date>" is ok. But any other date should display format "<day of the week> - <date>"
- standardize all variable names on the .json config file. all upper case variables there should be modified to match the standard used on all other variables. Make sure to not introduce any problems on the app itself by changing the variables. make sure they have changed everywhere.

## Improvement and new features suggestions


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
