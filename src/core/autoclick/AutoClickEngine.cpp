#include "core/autoclick/AutoClickEngine.h"

#include <QLineF>

#include <algorithm>

namespace ergo {

namespace {

double distance(const QPointF &a, const QPointF &b)
{
    return QLineF(a, b).length();
}

} // namespace

AutoClickEngine::AutoClickEngine(const IClock &clock)
    : m_clock(clock)
{
}

void AutoClickEngine::setConfig(const Config &config)
{
    // Takes effect immediately: a changed trigger time moves the pending
    // deadline, a changed distance applies to the next arming decision.
    m_config = config;
}

void AutoClickEngine::setEnabled(bool enabled)
{
    m_enabled = enabled;
    m_armed = false;
    m_hasReference = false; // the first position seen becomes the reference
    m_heldButtons.clear();
    m_suspendUntilMs = 0;
}

void AutoClickEngine::resetMotion(const QPointF &position)
{
    m_armed = false;
    m_hasReference = true;
    m_reference = position;
    m_position = position;
    m_lastSignificant = position;
    m_lastMoveMs = now();
    m_lastAnyMoveMs = m_lastMoveMs;
}

void AutoClickEngine::resetReferencePoint(const QPointF &position)
{
    resetMotion(position);
}

void AutoClickEngine::onInputEvent(const InputEvent &event)
{
    if (event.synthetic)
        return; // ErgoPro2's own clicks never count as user input
    switch (event.type) {
    case InputEventType::MouseMove:
        onMove(event.position);
        break;
    case InputEventType::MouseDown:
    case InputEventType::MouseUp:
        onButton(event);
        break;
    case InputEventType::KeyDown:
        onKey(event);
        break;
    case InputEventType::KeyUp:
    case InputEventType::Wheel:
        break;
    }
}

void AutoClickEngine::onMove(const QPointF &position)
{
    if (position != m_position)
        m_lastAnyMoveMs = now(); // any movement hides the visual feedback
    m_position = position;
    if (!m_hasReference) {
        resetMotion(position);
        return;
    }
    // Arming is checked on every movement, so a small final step across the
    // distance threshold still arms; the countdown then starts at that moment.
    if (!m_armed && distance(position, m_reference) >= m_config.distancePx) {
        m_armed = true;
        m_lastSignificant = position;
        m_lastMoveMs = now();
        return;
    }
    if (distance(position, m_lastSignificant) <= m_config.jitterPx)
        return; // tremor: does not restart the stop timer
    m_lastSignificant = position;
    m_lastMoveMs = now();
}

void AutoClickEngine::onButton(const InputEvent &event)
{
    const int button = int(event.button);
    if (event.type == InputEventType::MouseDown)
        m_heldButtons.insert(button);
    else
        m_heldButtons.remove(button);
    // Manual click: cancel, disarm, reference = click (and release) position.
    resetMotion(event.position);
}

void AutoClickEngine::onKey(const InputEvent &event)
{
    if (isModifierKey(event.key))
        return;
    m_suspendUntilMs = now() + m_config.typingSuspendMs;
}

void AutoClickEngine::onExternalClick(const QPointF &position)
{
    resetMotion(position);
}

void AutoClickEngine::setDragLockActive(bool active, const QPointF &position)
{
    m_dragLock = active;
    if (!active)
        resetMotion(position); // the pointer must travel the distance again
}

AutoClickEngine::SuppressReason AutoClickEngine::suppressReason() const
{
    if (m_dragLock)
        return SuppressReason::DragLock;
    if (!m_heldButtons.isEmpty())
        return SuppressReason::ButtonHeld;
    if (now() < m_suspendUntilMs)
        return SuppressReason::Typing;
    return SuppressReason::None;
}

qint64 AutoClickEngine::nextDeadlineMs() const
{
    if (!m_enabled || !m_armed || m_dragLock || !m_heldButtons.isEmpty())
        return -1;
    return std::max(m_lastMoveMs + m_config.triggerMs, m_suspendUntilMs);
}

bool AutoClickEngine::poll()
{
    const qint64 deadline = nextDeadlineMs();
    if (deadline < 0 || now() < deadline)
        return false;
    resetMotion(m_position); // one click, then disarmed at the click position
    return true;
}

AutoClickEngine::State AutoClickEngine::state() const
{
    if (!m_enabled)
        return State::Disabled;
    if (!m_armed)
        return State::WaitingForMovement;
    if (suppressReason() != SuppressReason::None)
        return State::Suppressed;
    if (now() - m_lastMoveMs < kMovingWindowMs)
        return State::Armed;
    return State::Countdown;
}

double AutoClickEngine::countdownProgress() const
{
    if (state() != State::Countdown || m_config.triggerMs <= 0)
        return 0.0;
    return std::clamp(double(now() - m_lastMoveMs) / m_config.triggerMs, 0.0, 1.0);
}

qint64 AutoClickEngine::feedbackShowAtMs() const
{
    const qint64 deadline = nextDeadlineMs();
    if (deadline < 0)
        return -1;
    const qint64 showAt = m_lastAnyMoveMs + m_config.triggerMs / 2;
    return showAt < deadline ? showAt : -1;
}

bool AutoClickEngine::isFeedbackVisible() const
{
    const qint64 showAt = feedbackShowAtMs();
    return showAt >= 0 && now() >= showAt;
}

int AutoClickEngine::remainingMs() const
{
    const qint64 deadline = nextDeadlineMs();
    if (deadline < 0)
        return -1;
    return int(std::max<qint64>(0, deadline - now()));
}

} // namespace ergo
