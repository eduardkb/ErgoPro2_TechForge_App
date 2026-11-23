#include "core/breaktimer/BreakReminder.h"

namespace ergo {

BreakReminder::Output BreakReminder::update(const Input &input)
{
    using State = BreakTimerEngine::State;
    const bool scheduled = input.state == State::Working || input.state == State::Postponed;
    // A new period starts with a new work cycle or a postponed break: the
    // reminder may appear again even if it was dismissed before.
    if (!scheduled || input.state != m_lastState)
        m_dismissed = false;
    m_lastState = input.state;

    Output out;
    if (!input.enabled || !scheduled || m_dismissed)
        return out;
    if (input.timeUntilBreakMs <= 0 || input.timeUntilBreakMs > input.leadMs)
        return out;
    out.show = true;
    out.remainingMs = input.timeUntilBreakMs;
    out.paused = input.state == State::Working && input.workTimePaused;
    return out;
}

void BreakReminder::dismiss()
{
    m_dismissed = true;
}

QPoint breakReminderPosition(const QRect &screen, const QRect &available, const QSize &window, int margin,
                             const QRect &trayIcon)
{
    bool top = false;
    if (trayIcon.isValid() && screen.contains(trayIcon.center()))
        top = trayIcon.center().y() < screen.center().y();
    else
        top = (available.top() - screen.top()) > (screen.bottom() - available.bottom());
    const int x = available.x() + available.width() - window.width() - margin;
    const int y = top ? available.y() + margin : available.y() + available.height() - window.height() - margin;
    return {x, y};
}

} // namespace ergo
