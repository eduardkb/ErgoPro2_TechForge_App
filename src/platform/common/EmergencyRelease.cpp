#include "platform/common/EmergencyRelease.h"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <initializer_list>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace ergo {

namespace {

std::atomic<EmergencyReleaseFn> g_fn{nullptr};
std::atomic<void *> g_context{nullptr};
std::atomic<bool> g_installed{false};
std::terminate_handler g_previousTerminate = nullptr;

void writeStderr(const char *text)
{
#if defined(_WIN32)
    std::fputs(text, stderr);
#else
    size_t length = 0;
    while (text[length])
        ++length;
    (void)!::write(2, text, length); // async-signal-safe
#endif
}

// Runs at most once per arming: the function pointer is taken atomically.
void runEmergencyRelease()
{
    void *context = g_context.load();
    if (EmergencyReleaseFn fn = g_fn.exchange(nullptr)) {
        fn(context);
        writeStderr("ErgoPro2: emergency release of the drag-lock mouse button\n");
    }
}

#if defined(_WIN32)

LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *)
{
    runEmergencyRelease();
    return EXCEPTION_CONTINUE_SEARCH; // let Windows report the crash as before
}

void signalHandler(int signal)
{
    runEmergencyRelease();
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

void installSignalHandlers()
{
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
    for (int s : {SIGSEGV, SIGILL, SIGFPE, SIGABRT, SIGTERM, SIGINT})
        std::signal(s, signalHandler);
}

#else

void signalHandler(int signal)
{
    runEmergencyRelease();
    // SA_RESETHAND restored the default action: raise again to die as before.
    std::raise(signal);
}

void installSignalHandlers()
{
    struct sigaction action = {};
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESETHAND | SA_NODEFER;
    for (int s : {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTERM, SIGINT, SIGHUP}) {
        struct sigaction previous = {};
        // Keep a handler someone else installed on purpose (e.g. a debugger
        // helper or SIG_IGN for SIGHUP when started with nohup).
        if (sigaction(s, nullptr, &previous) == 0 && previous.sa_handler != SIG_DFL)
            continue;
        sigaction(s, &action, nullptr);
    }
}

#endif

[[noreturn]] void terminateHandler()
{
    runEmergencyRelease();
    if (g_previousTerminate)
        g_previousTerminate();
    std::abort();
}

} // namespace

void installEmergencyHandlers()
{
    if (g_installed.exchange(true))
        return;
    installSignalHandlers();
    g_previousTerminate = std::set_terminate(terminateHandler);
}

void armEmergencyRelease(EmergencyReleaseFn fn, void *context)
{
    g_context.store(context);
    g_fn.store(fn);
}

void disarmEmergencyRelease()
{
    g_fn.store(nullptr);
}

} // namespace ergo
