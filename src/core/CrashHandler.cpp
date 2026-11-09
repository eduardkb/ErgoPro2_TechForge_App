#include "core/CrashHandler.h"

#include "core/Logger.h"

#include <cstdlib>
#include <stdexcept>

namespace ergo {

namespace {

void terminateHandler()
{
    if (std::exception_ptr current = std::current_exception())
        logUncaughtException("std::terminate", current);
    else
        logCritical(u"app", QStringLiteral("std::terminate called without an active exception - aborting"));
    std::abort();
}

} // namespace

void installTerminateHandler()
{
    std::set_terminate(terminateHandler);
}

void logUncaughtException(const char *where, std::exception_ptr exception)
{
    try {
        std::rethrow_exception(exception);
    } catch (const std::exception &e) {
        logCritical(u"app", QStringLiteral("Uncaught exception in %1: %2")
                                .arg(QString::fromLatin1(where), QString::fromLocal8Bit(e.what())));
    } catch (...) {
        logCritical(u"app", QStringLiteral("Uncaught unknown exception in %1").arg(QString::fromLatin1(where)));
    }
}

} // namespace ergo
