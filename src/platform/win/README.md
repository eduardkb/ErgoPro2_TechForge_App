# Windows platform implementations

Windows-specific implementations of the interfaces in `../PlatformInterfaces.h` go here
(`.cpp` files using Win32). They are added in stage 1 (autostart, installer, permissions)
and stage 2 (low-level hooks, `SendInput`, power events). Until then
`../stub/StubPlatform.*` is used.
