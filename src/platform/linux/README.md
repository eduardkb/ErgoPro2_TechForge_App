# Linux platform implementations

Linux-specific implementations of the interfaces in `../PlatformInterfaces.h` go here
(X11: XRecord/XTest/XGrabKey; Wayland best effort: libevdev + uinput). They are added in
stage 1 (autostart, installer, permissions) and stage 2 (input hook, click injection, power
events). Until then `../stub/StubPlatform.*` is used.
