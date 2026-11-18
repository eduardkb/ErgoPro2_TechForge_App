#include "core/keepactive/KeepScreenActiveEngine.h"

#include <algorithm>

namespace ergo {

KeepScreenActiveEngine::KeepScreenActiveEngine(const IClock &clock)
    : m_clock(clock)
{
    m_lastInputMs = now();
}

void KeepScreenActiveEngine::setConfig(const Config &config)
{
    m_config = config;
    m_config.startAfterMs = std::max<qint64>(m_config.startAfterMs, 1000);
    m_config.intervalMs = std::max<qint64>(m_config.intervalMs, 1000);
}

void KeepScreenActiveEngine::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    m_working = false;
    m_cycles = 0;
    m_lastInputMs = now(); // the hotkey press itself is input: count from now
}

void KeepScreenActiveEngine::onRealInput()
{
    m_lastInputMs = now();
    m_working = false;
    m_cycles = 0;
}

void KeepScreenActiveEngine::setPaused(PauseReason reason, bool paused)
{
    if (paused)
        m_pause |= reason;
    else
        m_pause &= quint8(~reason);
    if (m_pause != None)
        m_working = false; // resuming starts with an immediate cycle
}

bool KeepScreenActiveEngine::update()
{
    if (!m_enabled || m_pause != None)
        return false;
    const qint64 t = now();
    if (t - m_lastInputMs < m_config.startAfterMs) {
        m_working = false;
        return false;
    }
    if (!m_working || t >= m_nextCycleMs) {
        m_working = true;
        m_nextCycleMs = t + m_config.intervalMs;
        ++m_cycles;
        return true;
    }
    return false;
}

KeepScreenActiveEngine::State KeepScreenActiveEngine::state() const
{
    if (!m_enabled)
        return State::Disabled;
    return m_working && m_pause == None ? State::Working : State::Idle;
}

qint64 KeepScreenActiveEngine::idleMs() const
{
    return std::max<qint64>(0, now() - m_lastInputMs);
}

bool keepActiveClickPoint(const QRectF &frame, QPointF *point)
{
    // 1.9.1 (owner request): the click goes exactly to the window's top-left
    // corner, where the pointer shows the resize arrows. A click without a drag
    // there changes nothing (no resize, no control is pressed). The previous
    // spot in the title bar (1.9.0) hit Teams controls on macOS.
    if (frame.isEmpty())
        return false;
    *point = frame.topLeft();
    return true;
}

bool keepActiveHotkeyBecameActive(const QString &oldId, bool oldEnabled, const QString &newId, bool newEnabled)
{
    if (newId.isEmpty() || !newEnabled)
        return false;
    return newId != oldId || !oldEnabled;
}

} // namespace ergo
