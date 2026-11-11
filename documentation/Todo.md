# Problems
- tested in MacOS. BREAKTIMER_MAX_WORK_TIME was at 50 and computer usage at around 75%. I modified MAX_WORK_TIME to 35. things to correct:
    - the percentage abruptly grew to 120%. when the slider changes, do not modify the percentage. not up, neither down. maintain it the same and only scale to the new minutes and percentage after a new work cycle begins. this way the pause window can be shown even when the percentage does not reach 100%.
    - when the MAX_WORK_TIME gets modified to below what the active work time currently is display the pause window immediatelly.
    - Break window was correctly shown after countdown elapsed. but the computer usage percentage was not reset to 0. If the break window closes by itlsef (user paused for the configured amount of minutes) the usage percentage should always reset to 0. no matter if there were inputs during the break while the break window was shown.
- modified MAX_WORK_TIME again to 45 (after break fully taken). the graphic dropped to 91%. as explained on topic above, percentage should not change immediatelly. The new scale (percentage vs. time to pause) should only reflect on the next work cycle.
- whenever the computer usage percentage is above 100% make the pause window show at half the time configured in BREAKTIMER_MAX_WORK_TIME.
- BREAKTIMER_BREAK_TIME should have a minimum of 3, maximum of 15 and default of 4 when app is fists started and has no settings.
- on all tabs containing variable names written in capital letters (example: BREAKTIMER_BREAK_TIME) remove the variable name leaving only the description text.

====
- Modify the maximum, minumum and default as below:
break timer tab - break after working: min: 20, max 120, default 50
break timer tab - reset after a pause of: min: 2, max: 15, default: 5
break timer tab - postpone by: min: 3, max: 20, default: 10

====
- when installing on windows. try with the app running and without. (strange exit message right before it starts installing)