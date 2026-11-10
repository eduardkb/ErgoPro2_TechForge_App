#pragma once

namespace ergo {

// Last-resort safety for Drag Lock (B.5.3): a held mouse button must never
// stay down when ErgoPro2 crashes or is killed.
//
// installEmergencyHandlers() installs, once per process, handlers for fatal
// signals / unhandled exceptions (SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT,
// SIGTERM, SIGINT, SIGHUP; Windows: the unhandled exception filter) and
// std::terminate. While armed, such a handler calls the release function
// once, writes one line to stderr and then lets the process die as before
// (default action / next handler).
//
// The release function runs in a crash context: it must only post the
// button-up event (no logging, no Qt, no locks).
using EmergencyReleaseFn = void (*)(void *context);

void installEmergencyHandlers();
void armEmergencyRelease(EmergencyReleaseFn fn, void *context);
void disarmEmergencyRelease();

} // namespace ergo
