#pragma once

#include "core/Clock.h"

#include <QtGlobal>

namespace ergo {

// Break reminder logic (implementation_stages.md B.7). Pure: driven by
// activity, power/lock events and an injectable clock; the caller calls
// update() every second and shows/hides the break window when state() changes.
//
// All durations are WALL-CLOCK time (IClock::now()), so time spent asleep
// counts as idle time.
//
// Rules:
//  - INTERACTION_TIMER = active work time of the current cycle, which starts
//    with the first activity (WaitingForActivity -> Working). Time counts
//    while input continues; a pause counts only up to pauseAfterMs (20 s)
//    after the last input, then the timer stops until the next input (1.6.1).
//  - No activity for idleResetMs -> the cycle resets (counts as a break taken).
//    Screen lock / sleep: activity is ignored until unlock/resume, the time is
//    idle, and no break starts while locked (it is decided again afterwards).
//  - INTERACTION_TIMER >= maxWorkMs -> BreakShown (delayed while a full-screen
//    application is active, if that option is on).
//  - BreakShown: auto-closes after breakMs and resets; dismiss() resets
//    immediately; postpone() -> Postponed until postponeMs later, while the
//    INTERACTION_TIMER keeps counting; then BreakShown again.
//  - Postponed: idle >= idleResetMs cancels the postponed break and resets.
//  - Activity during a break changes nothing.
//  - Config changes apply immediately; disabling ends any break and resets.
class BreakTimerEngine {
public:
    struct Config {
        qint64 idleResetMs = 5 * 60000;
        qint64 maxWorkMs = 50 * 60000;
        qint64 breakMs = 4 * 60000;
        qint64 postponeMs = 10 * 60000;
        bool suppressWhenFullscreen = false;
        // The work time stops this long after the last input and resumes with
        // the next input (1.6.1, owner request): pauses only count up to it.
        qint64 pauseAfterMs = 20000;
    };

    enum class State { Disabled, WaitingForActivity, Working, BreakShown, Postponed };

    explicit BreakTimerEngine(const IClock &clock);

    void setConfig(const Config &config);
    const Config &config() const { return m_config; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_state != State::Disabled; }

    // Real user input (mouse move, click, wheel, key press). Synthetic events
    // must not be passed in (the caller filters them).
    void onActivity();

    void onSleep();
    void onResume();
    void onScreenLocked();
    void onScreenUnlocked();

    // A full-screen application is in the foreground (best-effort detection).
    void setFullscreenActive(bool active) { m_fullscreenActive = active; }

    // Evaluates the time-based rules. Call regularly (every second) and after events.
    void update();

    // Break window buttons and the tray's "Start break now".
    void dismiss();
    void postpone();
    void startBreakNow();

    State state() const { return m_state; }

    qint64 interactionMs() const;      // INTERACTION_TIMER (0 when no cycle runs)
    qint64 timeUntilBreakMs() const;   // -1 when no break is scheduled
    qint64 breakRemainingMs() const;   // BreakShown only, else -1
    qint64 idleMs() const;             // time since the last activity (0 without a cycle)
    bool isBreakDelayedByFullscreen() const;
    // Working, but the work time stands still: no input for pauseAfterMs (1.7.1:
    // shown as "Paused" on the tab). Always false in the other states.
    bool isWorkTimePaused() const;

private:
    qint64 now() const { return m_clock.now().toMSecsSinceEpoch(); }
    void resetCycle();
    void showBreak();
    // Work time at time t: accumulated + the current gap up to pauseAfterMs.
    qint64 workedAt(qint64 t) const;

    const IClock &m_clock;
    Config m_config;
    State m_state = State::WaitingForActivity;
    qint64 m_cycleStartMs = 0;
    qint64 m_lastActivityMs = 0;
    qint64 m_workedMs = 0;   // work time up to m_markMs
    qint64 m_markMs = 0;     // last input (or Postpone click) from which the gap counts
    qint64 m_breakWorkedMs = 0; // work time frozen while the break window is shown
    qint64 m_breakStartMs = 0;
    qint64 m_postponeUntilMs = 0;
    bool m_locked = false;
    bool m_fullscreenActive = false;
};

} // namespace ergo
