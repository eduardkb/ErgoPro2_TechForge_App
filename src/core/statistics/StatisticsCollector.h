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
// Usage percent = active seconds since the last reset / (maxWorkMs / 1000) x 100.
// Reset to 0 after resetAfterMs without input (sleep, lock and the app not
// running count as no input because the wall clock keeps running) and when a
// break completes by itself (1.7.6). Dismiss/Postpone do not reset it, so it can
// grow beyond 100 %. Midnight does not reset it. The maxWorkMs of the 100 %
// scale is taken when the usage (re)starts at 0; a later change of the setting
// applies only from the next reset, so the percent never jumps (1.7.6).
class StatisticsCollector {
public:
    struct Config {
        qint64 maxWorkMs = 50 * 60000;   // BREAKTIMER_MAX_WORK_TIME
        qint64 resetAfterMs = 4 * 60000; // BREAKTIMER_BREAK_TIME
    };

    struct FinishedMinute {
        QDate date;
        MinuteRecord record;
    };

    explicit StatisticsCollector(const IClock &clock);

    // resetAfterMs applies at once; maxWorkMs from the next usage reset (or at
    // once while the usage is 0). Recorded minutes are not recalculated.
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
    qint64 m_usedSeconds = 0;
    qint64 m_scaleMaxWorkMs = 50 * 60000;
    qint64 m_lastInputMs = 0;

    QPointF m_lastPosition;
    bool m_hasPosition = false;
};

} // namespace ergo
