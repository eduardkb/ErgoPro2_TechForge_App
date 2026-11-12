#pragma once

#include "core/Clock.h"
#include "core/input/InputEvent.h"
#include "core/statistics/StatisticsTypes.h"

#include <QDate>
#include <QList>
#include <QPointF>

namespace ergo {

// Records the statistics of B.8.1 and the computer usage percent of B.8.2.
// Pure logic: driven by input events, break events and the injectable clock
// (wall clock, local time). The caller calls tick() about once per second and
// stores the finished minutes returned by takeFinished().
//
// Time grid: one second = one wall-clock second. A second is a mouse second
// if it had mouse input, a keyboard second if it had key input, an active
// second if it had either. A minute is finished when the clock has passed it;
// only minutes with data are reported.
//
// Usage percent = used time since the last reset / maxWorkMs x 100 (1.7.8).
// The used time is counted like the Break Timer's active work time: a pause
// between two inputs counts up to pauseAfterMs, then the time stands still until
// the next input; nothing counts while the break window is shown. So 100 % is
// reached when the work time reaches "Break after working".
// Reset to 0 after resetAfterMs without input (sleep, lock and the app not
// running count as no input because the wall clock keeps running), when a
// break completes by itself (1.7.6) and when it goes above kMaxPercent (500 %,
// 1.7.8). Dismiss/Postpone do not reset it, so it can grow beyond 100 %.
// Midnight does not reset it. The maxWorkMs of the 100 % scale is taken when the
// usage restarts at 0 and when the Break Timer's work cycle resets
// (onWorkCycleReset(), 1.7.8).
class StatisticsCollector {
public:
    struct Config {
        qint64 maxWorkMs = 50 * 60000;   // BREAKTIMER_MAX_WORK_TIME
        qint64 resetAfterMs = 4 * 60000; // BREAKTIMER_BREAK_TIME
        qint64 pauseAfterMs = 20000;     // BREAKTIMER_ACTIVITY_PAUSE
    };

    // Above this the usage is set back to 0 % (1.7.8, owner request).
    static constexpr double kMaxPercent = 500.0;

    struct FinishedMinute {
        QDate date;
        MinuteRecord record;
    };

    explicit StatisticsCollector(const IClock &clock);

    // resetAfterMs and pauseAfterMs apply at once; maxWorkMs from the next usage
    // reset or work cycle reset (or at once while the usage is 0). Recorded
    // minutes are not recalculated.
    void setConfig(const Config &config);
    const Config &config() const { return m_config; }

    // Logical pixels per centimetre of the screen under the pointer.
    void setPixelsPerCm(double pixelsPerCm);

    // Continues a day after a restart: 'day' = minutes already stored today.
    // lastDataEndMs / lastPercent: end (epoch ms) and usage of the last stored
    // minute (today or the day before); the percent continues if that is less
    // than resetAfterMs ago.
    void restore(const DayStats &day, qint64 lastDataEndMs, double lastPercent);

    // Real user input; synthetic events are ignored. Ignored while locked.
    void onInputEvent(const InputEvent &event);
    // Clicks made by ErgoPro2 (Auto Click, Key Control, Drag Lock press).
    void onClicksDoneForYou(int count);
    void onBreakEvent(BreakEvent event);
    // The Break Timer's work cycle was reset (Active work time back to 0: idle
    // reset, break over, Dismiss) or is 0 when the setting changes: the current
    // maxWorkMs becomes the 100 % scale (1.7.8). The used time is kept, so the
    // percent can change once.
    void onWorkCycleReset();
    // True once after the usage went above kMaxPercent and was reset to 0.
    bool takeCapReset();
    // Sleep / screen lock: input is ignored until unlocked / resumed.
    void setLocked(bool locked) { m_locked = locked; }

    // Closes seconds and minutes the clock has passed.
    void tick();
    // Closes the current (partial) minute, e.g. on exit.
    void flush();

    QList<FinishedMinute> takeFinished();

    // Finished minutes of the current day (the open minute is not included).
    const DayStats &today() const { return m_today; }
    QDate currentDate() const { return m_today.date; }
    double usagePercent() const { return percentAt(now()); }
    // maxWorkMs the current percent is measured against.
    qint64 scaleMaxWorkMs() const { return m_scaleMaxWorkMs; }

private:
    qint64 now() const { return m_clock.now().toMSecsSinceEpoch(); }
    void advanceTo(qint64 t);
    void commitSecond();
    void finishMinute();
    void openMinuteAt(qint64 t);
    double percentAt(qint64 t) const;
    void resetUsage();
    qint64 usedAt(qint64 t) const;
    void checkCap(qint64 t);

    const IClock &m_clock;
    Config m_config;
    double m_pixelsPerCm = 96.0 / 2.54;
    bool m_locked = false;

    DayStats m_today;
    QList<FinishedMinute> m_finished;

    // Open minute.
    QDate m_minuteDate;
    qint64 m_minuteStartMs = 0;
    qint64 m_minuteEndMs = 0;
    MinuteRecord m_minute;

    // Open second.
    qint64 m_second = -1; // epoch seconds
    bool m_secondMouse = false;
    bool m_secondKeyboard = false;

    // Usage percent.
    qint64 m_usedMs = 0;  // used time up to m_markMs
    qint64 m_markMs = 0;  // last counted input; 0 = not counting (after a reset, on break)
    qint64 m_scaleMaxWorkMs = 50 * 60000;
    qint64 m_lastInputMs = 0; // for the reset after resetAfterMs
    bool m_onBreak = false;
    bool m_capReset = false;

    QPointF m_lastPosition;
    bool m_hasPosition = false;
};

} // namespace ergo
