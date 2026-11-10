#include "core/keycontrol/DragLockEngine.h"

namespace ergo {

DragLockEngine::DragLockEngine(const IClock &clock, IDragLockDevice &device)
    : m_clock(clock)
    , m_device(device)
{
}

OpResult DragLockEngine::toggle(const QPointF &pointer)
{
    if (m_state == State::Holding) {
        release(ReleaseReason::Hotkey);
        return OpResult::success();
    }
    if (!m_positionSeen)
        m_position = pointer;
    // Prepare everything (AutoClick suppressed, click consumption armed) before
    // the button goes down, so nothing can click in between.
    m_device.setHolding(true);
    m_state = State::Holding;
    m_lastMoveMs = now();
    if (m_callback)
        m_callback(true, m_position);
    const OpResult r = m_device.pressLeft();
    if (!r.ok) {
        m_state = State::Idle;
        m_lastReason = ReleaseReason::Error;
        m_device.setHolding(false);
        if (m_callback)
            m_callback(false, m_position);
    }
    return r;
}

bool DragLockEngine::release(ReleaseReason reason)
{
    if (m_state != State::Holding)
        return false;
    m_state = State::Idle;
    m_lastReason = reason;
    m_lastReleaseResult = m_device.releaseLeft();
    m_device.setHolding(false);
    // AutoClick: reference point = release position (D-04).
    if (m_callback)
        m_callback(false, m_position);
    return true;
}

void DragLockEngine::onInputEvent(const InputEvent &event)
{
    if (event.synthetic)
        return; // our own button down/up and ErgoPro2 clicks
    switch (event.type) {
    case InputEventType::MouseMove:
        if (m_state == State::Holding && (!m_positionSeen || event.position != m_position))
            m_lastMoveMs = now(); // D-02: the timeout counts from the last movement
        m_position = event.position;
        m_positionSeen = true;
        break;
    case InputEventType::MouseDown:
        m_position = event.position;
        m_positionSeen = true;
        release(ReleaseReason::MouseClick); // the input layer consumed the click (D-03)
        break;
    case InputEventType::MouseUp:
        m_position = event.position;
        m_positionSeen = true;
        break;
    case InputEventType::KeyDown:
        // Swallowed keys are hotkeys: they release through their action (or
        // toggle, for the drag-lock hotkey). Auto-repeats follow a press that
        // was already handled (e.g. the drag-lock hotkey held a bit longer).
        // Modifiers alone never release (D-01): Shift/Option change drag-and-drop.
        if (event.swallowed || event.autoRepeat || isModifierKey(event.key))
            break;
        release(ReleaseReason::KeyPressed); // the key itself reaches the OS normally
        break;
    case InputEventType::KeyUp:
    case InputEventType::Wheel:
        break; // scrolling while dragging is allowed and is not a movement
    }
}

qint64 DragLockEngine::nextDeadlineMs() const
{
    return m_state == State::Holding ? m_lastMoveMs + m_timeoutMs : -1;
}

bool DragLockEngine::poll()
{
    if (m_state != State::Holding || now() < nextDeadlineMs())
        return false;
    return release(ReleaseReason::Timeout);
}

QString DragLockEngine::reasonText(ReleaseReason reason)
{
    switch (reason) {
    case ReleaseReason::Hotkey: return QStringLiteral("hotkey pressed again");
    case ReleaseReason::Timeout: return QStringLiteral("timeout without mouse movement");
    case ReleaseReason::KeyPressed: return QStringLiteral("key pressed");
    case ReleaseReason::MouseClick: return QStringLiteral("mouse click (consumed)");
    case ReleaseReason::BreakWindow: return QStringLiteral("break window shown");
    case ReleaseReason::AppExit: return QStringLiteral("application exit");
    case ReleaseReason::KeyControlOff: return QStringLiteral("Key Control paused or unavailable");
    case ReleaseReason::BindingChanged: return QStringLiteral("drag-lock hotkey disabled, deleted or modified");
    case ReleaseReason::InputLost: return QStringLiteral("input hook not running");
    case ReleaseReason::AutoClickOff: return QStringLiteral("AutoClick switched off");
    case ReleaseReason::Error: return QStringLiteral("error");
    }
    return QStringLiteral("unknown");
}

} // namespace ergo
