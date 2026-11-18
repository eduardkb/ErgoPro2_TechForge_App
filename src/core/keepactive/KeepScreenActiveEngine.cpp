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

bool keepActiveTitleBarPoint(const QRectF &frame, KeepActiveApp app, QPointF *point)
{
    // Free area of the top bar: Teams has the window/navigation controls on the
    // left and the search box in the middle; Zoom has its tabs in the middle.
    // A spot at about a quarter of the width, at least 140 px from the left
    // edge, is empty in both on Windows and macOS (to be confirmed by the
    // manual tests; adjust here if an app changes its layout).
    constexpr double kMinLeft = 140.0;
    constexpr double kTopOffset = 10.0;
    const double fraction = app == KeepActiveApp::Teams ? 0.22 : 0.25;
    if (frame.width() < 2 * kMinLeft + 80 || frame.height() < 100)
        return false;
    const double x = frame.left() + std::max(kMinLeft, frame.width() * fraction);
    *point = QPointF(x, frame.top() + kTopOffset);
    return true;
}

} // namespace ergo
