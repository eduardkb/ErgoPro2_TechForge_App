#include "core/statistics/ChartAxis.h"
#include "core/statistics/StatisticsCollector.h"

#include <QTest>

using namespace ergo;

namespace {

constexpr qint64 kSec = 1000;
constexpr qint64 kMin = 60 * kSec;

InputEvent move(double x, double y)
{
    InputEvent e;
    e.type = InputEventType::MouseMove;
    e.position = QPointF(x, y);
    return e;
}

InputEvent click(MouseButtonId button, bool synthetic = false)
{
    InputEvent e;
    e.type = InputEventType::MouseDown;
    e.button = button;
    e.synthetic = synthetic;
    return e;
}

InputEvent key(int qtKey, bool repeat = false, InputEventType type = InputEventType::KeyDown)
{
    InputEvent e;
    e.type = type;
    e.key = qtKey;
    e.autoRepeat = repeat;
    return e;
}

// Clock starts at 09:00:00 local time; max work 5 min, reset after 1 min.
// The usage counts like the Break Timer's active work time (1.7.8): a pause
// counts up to pauseAfterMs. The fixture uses 1 s so that "input every second"
// gives round numbers (n seconds of use = n seconds counted).
struct Fixture {
    FakeClock clock{QDateTime(QDate(2026, 3, 10), QTime(9, 0))};
    StatisticsCollector collector{clock};
    QList<StatisticsCollector::FinishedMinute> finished;

    explicit Fixture(qint64 maxWorkMs = 5 * kMin, qint64 resetAfterMs = kMin, qint64 pauseAfterMs = kSec)
    {
        collector.setConfig({maxWorkMs, resetAfterMs, pauseAfterMs});
    }
    void advance(qint64 ms)
    {
        clock.advanceMs(ms);
        collector.tick();
        finished += collector.takeFinished();
    }
    // Input every second for 'ms' (one mouse move per second).
    void use(qint64 ms)
    {
        for (qint64 t = 0; t < ms; t += kSec) {
            collector.onInputEvent(move(double(t % 7), 0));
            advance(kSec);
        }
    }
    void idle(qint64 ms)
    {
        for (qint64 t = 0; t < ms; t += kSec)
            advance(kSec);
    }
    double lastPercent() const { return finished.isEmpty() ? -1 : finished.last().record.usagePercent; }
};

DayStats dayWith(std::initializer_list<int> minutes)
{
    DayStats d;
    d.date = QDate(2026, 3, 10);
    for (int m : minutes) {
        MinuteRecord r;
        r.minute = m;
        r.activeSeconds = 1;
        d.add(r);
    }
    return d;
}

} // namespace

class TestStatistics : public QObject {
    Q_OBJECT
private slots:
    // ---------------------------------------------------------------- per second
    void secondsAreClassifiedAsMouseKeyboardOrBoth()
    {
        Fixture f;
        f.clock.advanceMs(100);
        f.collector.onInputEvent(move(1, 1)); // second 0: mouse + keyboard
        f.collector.onInputEvent(key(Qt::Key_A));
        f.advance(kSec);
        f.collector.onInputEvent(move(2, 2)); // second 1: mouse only
        f.advance(kSec);
        f.collector.onInputEvent(key(Qt::Key_B)); // second 2: keyboard only
        f.advance(kMin);
        QCOMPARE(f.finished.size(), 1);
        const MinuteRecord &r = f.finished[0].record;
        QCOMPARE(r.minute, 9 * 60);
        QCOMPARE(r.mouseSeconds, 2);
        QCOMPARE(r.keyboardSeconds, 2);
        QCOMPARE(r.activeSeconds, 3);
        QCOMPARE(f.finished[0].date, QDate(2026, 3, 10));
    }

    void countersAndPrivacy()
    {
        Fixture f;
        f.collector.onInputEvent(click(MouseButtonId::Left));
        f.collector.onInputEvent(click(MouseButtonId::Left));
        f.collector.onInputEvent(click(MouseButtonId::Right));
        f.collector.onInputEvent(click(MouseButtonId::Middle));
        f.collector.onInputEvent(click(MouseButtonId::Left, true)); // ErgoPro2's own click: not real
        f.collector.onClicksDoneForYou(2); // e.g. a Key Control double click
        f.collector.onClicksDoneForYou(1);
        f.collector.onInputEvent(key(Qt::Key_A));
        f.collector.onInputEvent(key(Qt::Key_A, true)); // auto-repeat: no extra press
        f.collector.onInputEvent(key(Qt::Key_A, false, InputEventType::KeyUp));
        f.collector.onInputEvent(key(Qt::Key_Shift)); // modifiers count
        InputEvent wheel;
        wheel.type = InputEventType::Wheel;
        f.collector.onInputEvent(wheel);
        f.advance(kMin);
        const MinuteRecord &r = f.finished.at(0).record;
        QCOMPARE(r.clicksLeft, 2);
        QCOMPARE(r.clicksRight, 1);
        QCOMPARE(r.clicksOther, 1);
        QCOMPARE(r.clicksForYou, 3);
        QCOMPARE(r.keyPresses, 2);
        QCOMPARE(r.wheel, 1);
        QCOMPARE(r.clicks(), 4);
    }

    void syntheticEventsAreNotActivity()
    {
        Fixture f;
        InputEvent e = move(5, 5);
        e.synthetic = true;
        f.collector.onInputEvent(e);
        f.collector.onInputEvent(click(MouseButtonId::Left, true));
        f.advance(kMin);
        QVERIFY(f.finished.isEmpty()); // nothing recorded
    }

    void mouseDistance()
    {
        Fixture f;
        f.collector.setPixelsPerCm(10);
        f.collector.onInputEvent(move(0, 0));
        f.collector.onInputEvent(move(30, 40)); // 50 px
        f.collector.onInputEvent(move(30, 50)); // 10 px
        f.advance(kMin);
        QCOMPARE(f.finished.at(0).record.distancePx, 60.0);
        QCOMPARE(f.finished.at(0).record.distanceCm, 6.0);
    }

    void onlyMinutesWithDataAreReported()
    {
        Fixture f;
        f.collector.onInputEvent(move(1, 1));
        f.idle(10 * kMin);
        f.collector.onInputEvent(move(2, 2));
        f.advance(kMin);
        QCOMPARE(f.finished.size(), 2);
        QCOMPARE(f.finished[0].record.minute, 540);
        QCOMPARE(f.finished[1].record.minute, 550);
        QCOMPARE(f.collector.today().minutes.size(), 2);
    }

    void breakEventsAreRecordedInTheirMinute()
    {
        Fixture f;
        f.collector.onBreakEvent(BreakEvent::Shown); // no input needed
        f.advance(kMin);
        f.collector.onBreakEvent(BreakEvent::Postponed);
        f.collector.onBreakEvent(BreakEvent::Dismissed);
        f.advance(kMin);
        QCOMPARE(f.finished.size(), 2);
        QCOMPARE(f.finished[0].record.events, QStringList{QStringLiteral("break_shown")});
        QCOMPARE(f.finished[1].record.events,
                 (QStringList{QStringLiteral("break_postponed"), QStringLiteral("break_dismissed")}));
    }

    void flushReportsThePartialMinute()
    {
        Fixture f;
        f.collector.onInputEvent(key(Qt::Key_A));
        f.clock.advanceMs(20 * kSec);
        f.collector.flush();
        const auto list = f.collector.takeFinished();
        QCOMPARE(list.size(), 1);
        QCOMPARE(list[0].record.keyPresses, 1);
        QCOMPARE(list[0].record.activeSeconds, 1);
    }

    // ---------------------------------------------------------------- usage percent (B.8.2)
    void hundredPercentAtMaxWorkTimeOfContinuousUse()
    {
        Fixture f; // max work 5 min
        f.use(5 * kMin);
        QCOMPARE(f.finished.size(), 5);
        QCOMPARE(f.finished[0].record.usagePercent, 20.0);
        QCOMPARE(f.lastPercent(), 100.0);
    }

    void growsBeyondHundredAndBreakButtonsDoNotReset()
    {
        Fixture f;
        f.use(5 * kMin);
        f.collector.onBreakEvent(BreakEvent::Shown);
        f.collector.onBreakEvent(BreakEvent::Dismissed);
        f.use(kMin);
        QCOMPARE(f.lastPercent(), 120.0);
        f.collector.onBreakEvent(BreakEvent::Shown);
        f.collector.onBreakEvent(BreakEvent::Postponed);
        f.use(kMin);
        QCOMPARE(f.lastPercent(), 140.0); // (a completed break resets it since 1.7.6)
    }

    void resetsOnlyAfterTheBreakTimeWithoutInput()
    {
        Fixture f; // reset after 1 min
        f.use(2 * kMin);
        QCOMPARE(f.lastPercent(), 40.0);
        f.idle(58 * kSec); // shorter pause (59 s incl. the second after the last input): keeps the value
        f.use(kMin);
        QVERIFY(f.lastPercent() > 40.0);
        const double before = f.lastPercent();
        f.idle(kMin); // a full minute without input
        f.use(3 * kSec);
        f.idle(kMin);
        QVERIFY(f.lastPercent() < before);
        QCOMPARE(f.lastPercent(), 1.0); // 3 s of 300 s
    }

    void anEventOnlyMinuteAfterTheRestShowsZero()
    {
        Fixture f;
        f.use(2 * kMin);
        f.idle(2 * kMin);
        f.collector.onBreakEvent(BreakEvent::Completed);
        f.advance(kMin);
        QCOMPARE(f.lastPercent(), 0.0);
    }

    void sleepAndLockCountAsNoInput()
    {
        Fixture f;
        f.use(2 * kMin);
        f.collector.setLocked(true);
        for (int i = 0; i < 70; ++i) { // typing on the lock screen does not count
            f.collector.onInputEvent(key(Qt::Key_A));
            f.advance(kSec);
        }
        f.collector.setLocked(false);
        f.use(3 * kSec);
        f.idle(kMin);
        QCOMPARE(f.lastPercent(), 1.0); // reset: the lock lasted longer than the break time
        // Sleep: the clock jumps without ticks.
        f.use(kMin);
        f.collector.setLocked(true);
        f.clock.advanceMs(30 * kMin);
        f.collector.setLocked(false);
        f.use(3 * kSec);
        f.idle(kMin);
        QCOMPARE(f.lastPercent(), 1.0);
    }

    void midnightStartsANewDayButKeepsThePercent()
    {
        Fixture f;
        f.clock.setNow(QDateTime(QDate(2026, 3, 10), QTime(23, 58)));
        f.collector.tick();
        f.use(4 * kMin); // 23:58 - 00:02
        QCOMPARE(f.finished.size(), 4);
        QCOMPARE(f.finished[1].date, QDate(2026, 3, 10));
        QCOMPARE(f.finished[1].record.minute, 23 * 60 + 59);
        QCOMPARE(f.finished[2].date, QDate(2026, 3, 11));
        QCOMPARE(f.finished[2].record.minute, 0);
        QCOMPARE(f.finished[3].record.usagePercent, 80.0); // not reset at midnight
        QCOMPARE(f.collector.currentDate(), QDate(2026, 3, 11));
        QCOMPARE(f.collector.today().minutes.size(), 2);
    }

    void maxWorkChangeKeepsThePercentUntilTheUsageRestarts()
    {
        Fixture f; // max work 5 min, reset after 1 min
        f.use(kMin);
        QCOMPARE(f.lastPercent(), 20.0);
        f.collector.setConfig({10 * kMin, kMin, kSec}); // "Break after working" changed to 10 min
        QCOMPARE(f.collector.usagePercent(), 20.0); // no jump (1.7.6)
        f.use(kMin);
        QCOMPARE(f.finished[0].record.usagePercent, 20.0);
        QCOMPARE(f.lastPercent(), 40.0); // still the old 5-minute scale
        f.collector.setConfig({kMin, kMin, kSec}); // lowered: no jump either
        QCOMPARE(f.collector.usagePercent(), 40.0);
        // Usage restarts at 0 (pause of the break length): the new scale applies.
        f.idle(kMin);
        f.use(6 * kSec);
        f.idle(kMin);
        QCOMPARE(f.lastPercent(), 10.0); // 6 s of 60 s
    }

    void maxWorkChangeAppliesAtOnceWhileTheUsageIsZero()
    {
        Fixture f;
        f.collector.setConfig({10 * kMin, kMin, kSec});
        f.use(kMin);
        QCOMPARE(f.lastPercent(), 10.0);
    }

    void completedBreakResetsTheUsageEvenWithInputDuringTheBreak()
    {
        Fixture f;
        f.use(4 * kMin);
        f.collector.onBreakEvent(BreakEvent::Shown);
        f.use(kMin); // input while the break window is shown does not count (1.7.8, like the work time)
        QCOMPARE(f.lastPercent(), 80.0);
        f.collector.onBreakEvent(BreakEvent::Completed); // the break window closed by itself
        QCOMPARE(f.collector.usagePercent(), 0.0);
        f.use(3 * kSec);
        f.idle(kMin);
        QCOMPARE(f.lastPercent(), 1.0); // counting starts again from 0 (3 s of 300 s)
    }

    // 1.7.8: the usage counts time like the Break Timer's active work time.
    void pausesCountUpToTheActivityPauseLikeTheWorkTime()
    {
        Fixture f(5 * kMin, 5 * kMin, 20 * kSec); // pause after 20 s
        // One key press every 10 s for 5 minutes: the gaps count completely.
        for (int i = 0; i < 30; ++i) {
            f.collector.onInputEvent(key(Qt::Key_A));
            f.advance(10 * kSec);
        }
        QCOMPARE(f.lastPercent(), 100.0); // 5 min of work time = 100 % (was 10 % before 1.7.8)
        // A longer pause counts only 20 s, then the usage stands still.
        f.collector.onInputEvent(key(Qt::Key_A));
        f.advance(50 * kSec);
        QCOMPARE(f.collector.usagePercent(), 100.0 + 20.0 * 100 / 300);
        f.advance(60 * kSec);
        QCOMPARE(f.collector.usagePercent(), 100.0 + 20.0 * 100 / 300);
    }

    void workCycleResetAppliesTheNewScale()
    {
        Fixture f; // 5 min
        f.use(kMin);
        f.collector.setConfig({10 * kMin, kMin, kSec});
        QCOMPARE(f.collector.usagePercent(), 20.0); // no jump while the work cycle runs
        f.collector.onWorkCycleReset(); // e.g. Dismiss or the Break Timer's idle reset
        QCOMPARE(f.collector.scaleMaxWorkMs(), 10 * kMin);
        QCOMPARE(f.collector.usagePercent(), 10.0); // same used time on the new scale
        f.use(kMin);
        QCOMPARE(f.lastPercent(), 20.0);
    }

    void aboveFiveHundredPercentResetsToZero()
    {
        Fixture f(kMin, 10 * kMin); // 100 % = 1 min
        f.use(5 * kMin);
        QCOMPARE(f.lastPercent(), 500.0); // exactly 500 %: kept
        QVERIFY(!f.collector.takeCapReset());
        f.use(2 * kSec);
        QVERIFY(f.collector.takeCapReset()); // reported once
        QVERIFY(!f.collector.takeCapReset());
        QVERIFY(f.collector.usagePercent() < 5.0);
        f.use(30 * kSec);
        f.idle(kMin);
        QVERIFY(f.lastPercent() <= 500.0);
        QVERIFY(f.collector.usagePercent() < 60.0);
        for (const auto &m : std::as_const(f.finished))
            QVERIFY(m.record.usagePercent <= 500.0);
    }

    void dismissAndPostponeDoNotReset()
    {
        Fixture f;
        f.use(kMin);
        f.collector.onBreakEvent(BreakEvent::Dismissed);
        f.collector.onBreakEvent(BreakEvent::Postponed);
        QCOMPARE(f.collector.usagePercent(), 20.0);
    }

    void restoreContinuesTheDayAndThePercent()
    {
        Fixture f;
        DayStats day = dayWith({500, 530});
        const qint64 lastEnd = QDateTime(QDate(2026, 3, 10), QTime(8, 51)).toMSecsSinceEpoch();
        f.clock.setNow(QDateTime(QDate(2026, 3, 10), QTime(8, 51, 30)));
        f.collector.restore(day, lastEnd, 40.0); // 30 s ago < reset time
        QCOMPARE(f.collector.today().minutes.size(), 2);
        f.use(kMin);
        QVERIFY(f.lastPercent() > 40.0);

        Fixture g;
        g.clock.setNow(QDateTime(QDate(2026, 3, 10), QTime(9, 30)));
        g.collector.restore(day, lastEnd, 40.0); // long ago: starts at 0
        g.use(3 * kSec);
        g.idle(kMin);
        QCOMPARE(g.lastPercent(), 1.0);
    }

    // ---------------------------------------------------------------- types
    void dayMergesRecordsOfTheSameMinute()
    {
        DayStats d;
        MinuteRecord a;
        a.minute = 600;
        a.keyPresses = 3;
        a.activeSeconds = 40;
        a.usagePercent = 10;
        MinuteRecord b = a;
        b.keyPresses = 2;
        b.usagePercent = 12;
        b.events << QStringLiteral("break_shown");
        MinuteRecord c;
        c.minute = 300;
        d.add(a);
        d.add(b);
        d.add(c);
        QCOMPARE(d.minutes.size(), 2);
        QCOMPARE(d.minutes[0].minute, 300); // sorted
        QCOMPARE(d.find(600)->keyPresses, 5);
        QCOMPARE(d.find(600)->activeSeconds, 60); // capped at 60
        QCOMPARE(d.find(600)->usagePercent, 12.0);
        QVERIFY(!d.find(301));
    }

    void totals()
    {
        DayStats d;
        MinuteRecord a;
        a.minute = 1;
        a.clicksLeft = 2;
        a.clicksForYou = 1;
        a.mouseSeconds = 30;
        a.keyboardSeconds = 10;
        a.activeSeconds = 35;
        a.distanceCm = 12.5;
        a.usagePercent = 80;
        MinuteRecord b = a;
        b.minute = 2;
        b.usagePercent = 110;
        d.add(a);
        d.add(b);
        const DayTotals t = computeTotals(d);
        QCOMPARE(t.clicks(), 4);
        QCOMPARE(t.clicksForYou, 2);
        QCOMPARE(t.mouseSeconds, qint64(60));
        QCOMPARE(t.activeSeconds, qint64(70));
        QCOMPARE(t.distanceCm, 25.0);
        QCOMPARE(t.maxUsagePercent, 110.0);
    }

    // ---------------------------------------------------------------- chart axes
    void hourRangeFromFirstToLastData()
    {
        QCOMPARE(hourRange(DayStats()).startHour, 8); // empty day
        QCOMPARE(hourRange(DayStats()).endHour, 18);
        const HourRange r = hourRange(dayWith({9 * 60 + 15, 13 * 60 + 59}));
        QCOMPARE(r.startHour, 9);
        QCOMPARE(r.endHour, 14);
        QCOMPARE(r.minutes(), 300);
        const HourRange late = hourRange(dayWith({23 * 60 + 59}));
        QCOMPARE(late.startHour, 23);
        QCOMPARE(late.endHour, 24);
    }

    void percentAxisScaling()
    {
        double step = 0;
        QCOMPARE(percentAxisMax(0, &step), 100.0);
        QCOMPARE(step, 25.0);
        QCOMPARE(percentAxisMax(80), 100.0);
        QCOMPARE(percentAxisMax(130), 150.0);
        QCOMPARE(percentAxisMax(260, &step), 300.0);
        QCOMPARE(step, 50.0);
        QCOMPARE(percentAxisMax(480, &step), 500.0);
        QCOMPARE(step, 50.0);
        QCOMPARE(percentAxisMax(612, &step), 500.0); // the usage never exceeds 500 % (1.7.8)
        QCOMPARE(step, 50.0);
    }

    void niceCeilings()
    {
        QCOMPARE(niceCeiling(0), 1.0);
        QCOMPARE(niceCeiling(7), 10.0);
        QCOMPARE(niceCeiling(13), 20.0);
        QCOMPARE(niceCeiling(41), 50.0);
        QCOMPARE(niceCeiling(100), 100.0);
        QCOMPARE(niceCeiling(101), 200.0);
    }

    void fiveMinuteBucketing()
    {
        DayStats d;
        MinuteRecord a;
        a.minute = 542;
        a.clicksLeft = 2;
        a.keyPresses = 10;
        a.distanceCm = 3;
        MinuteRecord b = a;
        b.minute = 544;
        MinuteRecord c = a;
        c.minute = 545;
        d.add(a);
        d.add(b);
        d.add(c);
        const QList<ActivityBucket> buckets = fiveMinuteBuckets(d);
        QCOMPARE(buckets.size(), 288);
        QCOMPARE(buckets[108].startMinute, 540);
        QCOMPARE(buckets[108].clicks, 4);
        QCOMPARE(buckets[108].keyPresses, 20);
        QCOMPARE(buckets[108].distanceCm, 6.0);
        QCOMPARE(buckets[109].clicks, 2);
        QCOMPARE(buckets[0].clicks, 0);
    }
};

QTEST_GUILESS_MAIN(TestStatistics)
#include "tst_statistics.moc"
