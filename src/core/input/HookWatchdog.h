#pragma once

#include "core/Clock.h"

namespace ergo {

// Decision logic of the input hook health watchdog (B.5.4). Pure: driven by
// the caller and an injectable clock, so it is unit-testable.
//
// - A periodic health check (every checkIntervalMs) reports whether the hook is
//   healthy; an unhealthy hook is reinstalled.
// - Resume from sleep and session unlock always trigger a reinstall.
// - Reinstalls are not repeated within minReinstallSpacingMs (e.g. resume and
//   unlock arriving together).
// - After maxConsecutiveFailures failed reinstalls the failure is persistent and
//   must be reported to the user; reinstalls continue to be attempted.
class HookWatchdog {
public:
    struct Config {
        int checkIntervalMs = 30000;
        int minReinstallSpacingMs = 5000;
        int maxConsecutiveFailures = 3;
    };

    enum class Action { None, Reinstall };

    explicit HookWatchdog(const IClock &clock);
    HookWatchdog(const IClock &clock, const Config &config);

    Action onHealthCheck(bool healthy);
    Action onResume();
    Action onUnlock();
    void onReinstallResult(bool ok);

    const Config &config() const { return m_config; }
    int reinstallCount() const { return m_reinstalls; }
    int consecutiveFailures() const { return m_failures; }
    bool persistentFailure() const { return m_failures >= m_config.maxConsecutiveFailures; }

private:
    Action requestReinstall();

    const IClock &m_clock;
    Config m_config;
    int m_reinstalls = 0;
    int m_failures = 0;
    qint64 m_lastReinstallMs = -1;
};

} // namespace ergo
