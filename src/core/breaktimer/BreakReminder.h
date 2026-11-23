#pragma once

#include "core/breaktimer/BreakTimerEngine.h"

#include <QRect>
#include <QSize>
#include <QtGlobal>

namespace ergo {

// Reminder shortly before a break (2.3.0, owner request; spec B.7): a small,
// non-blocking window with a countdown "Break in 0:58". Pure decision logic,
// driven by the Break Timer engine's state once per second
// (tests/tst_breakreminder.cpp).
//
// Rules:
//  - Shown while the reminder is enabled, a break is scheduled (Working or
//    Postponed) and 0 < time until the break <= leadMs.
//  - "Paused": the work time stands still (no input); the countdown freezes.
//  - Clicking it dismisses it for the current period. A period is one work
//    cycle up to its break, or one postponed break (owner: the postponed break
//    gets its own reminder).
//  - Hidden when the break window appears, the cycle resets or the timer is
//    switched off; also when the time until the break grows above leadMs again
//    (e.g. a longer "Break after working").
class BreakReminder {
public:
    struct Input {
        BreakTimerEngine::State state = BreakTimerEngine::State::Disabled;
        qint64 timeUntilBreakMs = -1;
        bool workTimePaused = false;
        bool enabled = true;
        qint64 leadMs = 60000;
    };
    struct Output {
        bool show = false;
        qint64 remainingMs = 0;
        bool paused = false;
    };

    Output update(const Input &input);
    // The user clicked the reminder: hidden until the next period.
    void dismiss();
    bool isDismissed() const { return m_dismissed; }
    // The work cycle was reset (also when the reset and the next input fall
    // into the same second, so update() never sees WaitingForActivity).
    void startNewPeriod() { m_dismissed = false; }

private:
    BreakTimerEngine::State m_lastState = BreakTimerEngine::State::Disabled;
    bool m_dismissed = false;
};

// Top-left position of the reminder window, in the right corner next to the
// tray, 'margin' pixels inside the available area: top right when the tray
// icon is in the upper half of the screen (macOS menu bar, GNOME), else bottom
// right (Windows taskbar). Without a known tray icon position: the side where
// a panel takes more space.
QPoint breakReminderPosition(const QRect &screen, const QRect &available, const QSize &window, int margin,
                             const QRect &trayIcon = {});

} // namespace ergo
