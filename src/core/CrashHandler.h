#pragma once

#include <exception>

namespace ergo {

// Installs a std::terminate handler that logs the uncaught exception as
// CRITICAL (through the Qt message handler / ergo::Logger) before aborting.
void installTerminateHandler();

// Logs an exception that escaped to a top-level boundary (e.g. an event handler)
// as CRITICAL. 'where' describes the boundary.
void logUncaughtException(const char *where, std::exception_ptr exception);

} // namespace ergo
