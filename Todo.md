# Problems
- git actions - adds a new github release with the name: "ErgoPro2_vx.x.x". The release should contain a summary of the changes made on its version and allow the user to download executable files of the application for all three operating systems. The same artifact being generated for linux and MacOS can be used. But the windows version will change as below.
- for windows make the git actions script bundle all files needed in one .exe installer. This installer already installs the app on the recommended location. User can choose the location if he wants. if a version is already being executed it is closed and then the new version is installed on its place. if it is not being executed but is installed, it should also be replaced on the same place where it was installed previously. Log, statistics and config files remain on the original location. On windows, if app gets intalled this way on general screen installation box the  app behaves as already installed.

=====
- Break timer tab - status box: between active work time and next break in insert a new field that shows the hour (24h format), minute and second when the active work timer started last. it can show "-" if it was reset and user did not start any action yet. I want it to show the time the user last started a cycle and if the work cycle was reset it shows "-".
- change all sliders so they don't change their value with mouse scroll. mouse scroll only scrolls right throug the sliders. only mouse click or typing the value on the text box changes their value.
- on general tab - input diagnostics box: remove both buttons ("Test left click" and "Test: Bloxk F9...") at the end of the box


====
- when installing on windows. try with the app and without. (strange exit message right before it starts installing)