#include "core/input/HookWatchdog.h"

namespace ergo {

HookWatchdog::HookWatchdog(const IClock &clock)
    : HookWatchdog(clock, Config{})
{
}

HookWatchdog::HookWatchdog(const IClock &clock, const Config &config)
    : m_clock(clock)
    , m_config(config)
{
}

HookWatchdog::Action HookWatchdog::onHealthCheck(bool healthy)
{
    if (healthy) {
        m_failures = 0;
        return Action::None;
    }
    return requestReinstall();
}

HookWatchdog::Action HookWatchdog::onResume()
{
    return requestReinstall();
}

HookWatchdog::Action HookWatchdog::onUnlock()
{
    return requestReinstall();
}

HookWatchdog::Action HookWatchdog::requestReinstall()
{
    const qint64 now = m_clock.monotonicMs();
    if (m_lastReinstallMs >= 0 && now - m_lastReinstallMs < m_config.minReinstallSpacingMs)
        return Action::None;
    m_lastReinstallMs = now;
    ++m_reinstalls;
    return Action::Reinstall;
}

void HookWatchdog::onReinstallResult(bool ok)
{
    m_failures = ok ? 0 : m_failures + 1;
}

} // namespace ergo
