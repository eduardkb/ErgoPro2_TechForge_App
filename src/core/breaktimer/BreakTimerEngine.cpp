#include "core/breaktimer/BreakTimerEngine.h"

#include <algorithm>

namespace ergo {

BreakTimerEngine::BreakTimerEngine(const IClock &clock)
    : m_clock(clock)
{
}

void BreakTimerEngine::setConfig(const Config &config)
{
    m_config = config;
    update(); // e.g. a shorter max work time can start the break right away
}

void BreakTimerEngine::setEnabled(bool enabled)
{
    if (enabled == isEnabled())
        return;
    m_state = enabled ? State::WaitingForActivity : State::Disabled;
    m_cycleStartMs = m_lastActivityMs = m_breakStartMs = m_postponeUntilMs = 0;
    m_workedMs = m_markMs = m_breakWorkedMs = 0;
}

void BreakTimerEngine::resetCycle()
{
    m_state = State::WaitingForActivity;
    m_cycleStartMs = m_lastActivityMs = m_breakStartMs = m_postponeUntilMs = 0;
    m_workedMs = m_markMs = m_breakWorkedMs = 0;
}

qint64 BreakTimerEngine::workedAt(qint64 t) const
{
    if (m_cycleStartMs == 0)
        return 0;
    return m_workedMs + std::clamp<qint64>(t - m_markMs, 0, m_config.pauseAfterMs);
}

void BreakTimerEngine::showBreak()
{
    m_breakWorkedMs = workedAt(now()); // no counting while the break is shown (B-13)
    m_state = State::BreakShown;
    m_breakStartMs = now();
}

void BreakTimerEngine::onActivity()
{
    if (m_state == State::Disabled || m_state == State::BreakShown || m_locked)
        return; // activity during a break changes nothing (B-13); locked = idle
    const qint64 t = now();
    if (m_state == State::WaitingForActivity || t - m_lastActivityMs >= m_config.idleResetMs) {
        // First activity, or idle long enough since the last activity (e.g.
        // update() was not called in between): this activity starts a new cycle.
        m_state = State::Working;
        m_cycleStartMs = t;
        m_postponeUntilMs = 0;
        m_workedMs = 0;
    } else {
        m_workedMs = workedAt(t); // the gap counts up to pauseAfterMs
    }
    m_markMs = t;
    m_lastActivityMs = t;
}

void BreakTimerEngine::onSleep()
{
    // Sleep counts as idle: nothing to record, the wall clock keeps running and
    // update() after resume sees the long idle time.
    m_locked = true;
}

void BreakTimerEngine::onResume()
{
    m_locked = false;
    update();
}

void BreakTimerEngine::onScreenLocked()
{
    m_locked = true;
}

void BreakTimerEngine::onScreenUnlocked()
{
    m_locked = false;
    update();
}

bool BreakTimerEngine::isBreakDelayedByFullscreen() const
{
    return m_config.suppressWhenFullscreen && m_fullscreenActive;
}

void BreakTimerEngine::update()
{
    const qint64 t = now();
    switch (m_state) {
    case State::Disabled:
    case State::WaitingForActivity:
        return;
    case State::Working:
        if (t - m_lastActivityMs >= m_config.idleResetMs)
            resetCycle(); // natural break taken (B-03)
        else if (workedAt(t) >= m_config.maxWorkMs && !isBreakDelayedByFullscreen() && !m_locked)
            showBreak(); // never behind the lock screen; decided again after unlock
        return;
    case State::Postponed:
        if (t - m_lastActivityMs >= m_config.idleResetMs)
            resetCycle(); // rested during the postpone: cancel it (B-09)
        else if (t >= m_postponeUntilMs && !isBreakDelayedByFullscreen() && !m_locked)
            showBreak();
        return;
    case State::BreakShown:
        if (t - m_breakStartMs >= m_config.breakMs)
            resetCycle(); // break over: new cycle (B-06)
        return;
    }
}

void BreakTimerEngine::dismiss()
{
    if (m_state == State::BreakShown)
        resetCycle(); // B-07: no confirmation, no limit
}

void BreakTimerEngine::postpone()
{
    if (m_state != State::BreakShown)
        return;
    m_state = State::Postponed;
    m_postponeUntilMs = now() + m_config.postponeMs;
    if (m_cycleStartMs == 0) { // break started by "Start break now" without a cycle
        m_cycleStartMs = m_breakStartMs;
        m_lastActivityMs = m_breakStartMs;
    }
    // The work time continues from the break (B-08); the click on Postpone
    // counts as the last input.
    m_workedMs = m_breakWorkedMs;
    m_markMs = now();
}

void BreakTimerEngine::startBreakNow()
{
    if (m_state == State::Disabled || m_state == State::BreakShown)
        return;
    showBreak();
}

qint64 BreakTimerEngine::interactionMs() const
{
    if (m_cycleStartMs == 0 || m_state == State::WaitingForActivity || m_state == State::Disabled)
        return 0;
    if (m_state == State::BreakShown)
        return m_breakWorkedMs;
    return workedAt(now());
}

qint64 BreakTimerEngine::timeUntilBreakMs() const
{
    switch (m_state) {
    case State::Working:
        return std::max<qint64>(0, m_config.maxWorkMs - workedAt(now()));
    case State::Postponed:
        return std::max<qint64>(0, m_postponeUntilMs - now());
    case State::BreakShown:
        return 0;
    case State::Disabled:
    case State::WaitingForActivity:
        break;
    }
    return -1;
}

qint64 BreakTimerEngine::breakRemainingMs() const
{
    if (m_state != State::BreakShown)
        return -1;
    return std::max<qint64>(0, m_breakStartMs + m_config.breakMs - now());
}

qint64 BreakTimerEngine::idleMs() const
{
    if (m_lastActivityMs == 0)
        return 0;
    return std::max<qint64>(0, now() - m_lastActivityMs);
}

} // namespace ergo
