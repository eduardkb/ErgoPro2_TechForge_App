# ErgoPro2 app

This application is intended to help persons with wrist pain to reduce mouse and keyboard usage with its features and suggested break timers.

## Basic functionality
- Function 1 - AutoClick: if enabled, left mouse button is clicked wheneve the pointer stops moving.
- Function 2 - Key Control: allows customer to add function key combinations to automate activities (like mouse clicking)
- Function 3 - Break Timer: periodically suggest user to stop and rest for some minutes.
- Function 4 - Statistics: will be implemented last. no need to worry about this funciton now.

## Base application and Function details:

### Basics
- The application's backend will be written in C++ and its interface in Qt.
- The same application code will have to run in Windwos 11, MacOS Tahoe and linux Debian.
- all initial tests will be made in MacOS. but write the code so it works on all operating systems mentioned above.
- The interface will have tabs in the top named: "General", "Break Timer", "Auto Click", "Key Control" and "Statistics".
- The parameter file in JSON format will be kept somewhere on the operating system to read saved parameters when the applicatoin starts.
- any time user changes a parameter it must be saved on JSON parameters file so that the application starts as it was last configured.
- the application will have a logging module to log erros, warnings, informations and debug messages. All erros or exceptions should be logged and not silently ignored.
- On operating systems where possible, applicataion should minimize to a tray instead of remaining on main applications bar.
- The application executable through its interface (general tab) should allow user to "install" this application to be run when the OS launches.
- on general tab, insert a button to exit the application.
- write a .md file on how to generate the executable for each of the three operating systems and how to execute the file and how to put it in a service for auto-execution.

### Function 1
- auto-click's main function is to left click the mouse button automatically whenever the mouse pointer stops moving.
- this function should have a thread of its own to work separately from the main application's interface.
- On the Auto Click tab, user can change parameters: 
    - AUTOCLICK_TRIGGER_TIMER: in milisseconds the time Auto Click waits before clicking the left mouse button after the cursor has stopped moving (ranges from 5ms to 500ms)
    - AUTOCLICK_TRIGGER_DISTANCE: in pixels the minimum ammount of pixels the mouse cursor has to move to trigger a auto-click (ranges from 5px to 200px)
- on the Auto Click tab, clearly indicate in the top if the functionality is active or not and include a button to activate or deactivate it.

### Function 2
- Key Control lets the user configure keys or key combinations that will register exclusively to this application with the operating system and when pressed do its assigned action.
- When the user clicks a button (Add Hotkey) he can press a keyboard key or combination of keys. when pressed he is then asked to select what that key will do. after configuration confirmed, the key or key combination is exclusively bound to this application (other OS actions that would be performed with this keys will be ignored).
- when user configures each action, it appears on a list of actions and he has the options to disable this action, modify it (including key or key combination) and reset the key binding which does reset the OS level binding for the key.
- each action configured should start a new thread on the processor. if the action is disabled or reset, the thread should be removed and re-created from scratch associating the key exclusively to this applciation again from a OS level.
- Possible actions:
    - Left mouse click
    - Right mouse click
    - double left mouse click
    - Drag lock (left mouse click) - This function will click and hold the left mouse button. then if the key associated to it is pressed again, the hold is released. Hold will also be released afte 10 seconds or if any keyboard or mouse button is clicked. Since the autoclick feature can cancel this action make auto click feature not click the left mouse button ducing drag lock action and not click after its release. Autoclick should only click again after mouse is moved the distance specified in "AUTOCLICK_TRIGGER_DISTANCE" parameter after the drag lock key is released (when released the movement count is reset again).

### Function 3
Break timer funciton actively monitors keyboard key presses and mouse movements and clicks. The application maintains a timer (from now on called INTERACTION_TIMER) for how long the user is interacting with the PC. If this "INTERACTION_TIMER" gets bnigger than "BREAKTIMER_MAX_WORK_TIME" minutes, a screen appears reminding user to take a break. This screan appears on top of all the others and cannot be clicked away unless a button on its screen is pressed.
- On the Break Timer tab, user can change parameters: 
    - BREAKTIMER_IDLE_RESET: how many minutes of inactivity is needed to reset the "interaction timer"
    - BREAKTIMER_MAX_WORK_TIME: how many minutes of active work until the break screen reminder is shown.
    - BREAKTIMER_BREAK_TIME: for how many minutes the break window is shown before automatically closing.
- Display actively working minutes on the bottom of the "break timer" tab.
- Buttons on "Break Timer" tab: Dismiss, postpone
    - Dismiss: instantly closes the berak window and resets the INTERACTION_TIMER starting a new cycle.
    - postopone: sleeps the break window for 10 minutes closing it and re-opening it after 10 minutes.
- the break window should show a countdown timer to allow user to know how many break minutes are left. and the break window automatically closes when it reaches zero.
    