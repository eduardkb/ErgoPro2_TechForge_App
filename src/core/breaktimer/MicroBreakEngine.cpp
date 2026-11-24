#include "core/breaktimer/MicroBreakEngine.h"

#include <algorithm>

namespace ergo {

MicroBreakEngine::MicroBreakEngine(const IClock &clock)
    : m_clock(clock)
{
}

void MicroBreakEngine::setConfig(const Config &config)
{
    m_config = config;
    m_config.intervalMs = std::max<qint64>(m_config.intervalMs, 1000);
    m_config.showMs = std::max<qint64>(m_config.showMs, 1000);
    m_config.pauseAfterMs = std::max<qint64>(m_config.pauseAfterMs, 0);
    update();
}

void MicroBreakEngine::setEnabled(bool enabled)
{
    if (enabled == isEnabled())
        return;
    reset();
    if (!enabled)
        m_state = State::Disabled;
}

void MicroBreakEngine::reset()
{
    m_state = State::Waiting;
    m_workedMs = m_markMs = m_lastActivityMs = m_shownAtMs = 0;
}

qint64 MicroBreakEngine::workedAt(qint64 t) const
{
    return m_workedMs + std::clamp<qint64>(t - m_markMs, 0, m_config.pauseAfterMs);
}

void MicroBreakEngine::onActivity()
{
    if (m_state == State::Disabled || m_state == State::Shown || m_locked)
        return;
    const qint64 t = now();
    if (m_state == State::Waiting || t - m_lastActivityMs >= m_config.showMs) {
        m_state = State::Working; // first input of a new count
        m_workedMs = 0;
    } else {
        m_workedMs = workedAt(t);
    }
    m_markMs = m_lastActivityMs = t;
}

void MicroBreakEngine::setLocked(bool locked)
{
    m_locked = locked;
    if (locked && isEnabled())
        reset();
}

void MicroBreakEngine::setBlocked(bool blocked)
{
    m_blocked = blocked;
    if (blocked && m_state == State::Shown)
        reset();
    update();
}

void MicroBreakEngine::restart()
{
    if (m_state == State::Working)
        reset();
}

void MicroBreakEngine::dismiss()
{
    if (m_state == State::Shown)
        reset();
}

void MicroBreakEngine::update()
{
    const qint64 t = now();
    switch (m_state) {
    case State::Disabled:
    case State::Waiting:
        return;
    case State::Working:
        if (t - m_lastActivityMs >= m_config.showMs)
            reset(); // rested as long as a micro-break
        else if (workedAt(t) >= m_config.intervalMs && !m_blocked && !m_locked) {
            m_state = State::Shown;
            m_shownAtMs = t;
        }
        return;
    case State::Shown:
        if (t - m_shownAtMs >= m_config.showMs)
            reset();
        return;
    }
}

qint64 MicroBreakEngine::activeMs() const
{
    switch (m_state) {
    case State::Working: return workedAt(now());
    case State::Shown: return m_config.intervalMs;
    case State::Disabled:
    case State::Waiting: break;
    }
    return 0;
}

qint64 MicroBreakEngine::timeUntilMs() const
{
    if (m_state != State::Working)
        return -1;
    return std::max<qint64>(0, m_config.intervalMs - workedAt(now()));
}

qint64 MicroBreakEngine::shownRemainingMs() const
{
    if (m_state != State::Shown)
        return -1;
    return std::max<qint64>(0, m_shownAtMs + m_config.showMs - now());
}

bool MicroBreakEngine::isPaused() const
{
    return m_state == State::Working && now() - m_markMs >= m_config.pauseAfterMs;
}

} // namespace ergo
