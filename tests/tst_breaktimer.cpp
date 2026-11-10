#include "core/breaktimer/BreakTimerEngine.h"

#include <QTest>

using namespace ergo;
using S = BreakTimerEngine::State;

namespace {

constexpr qint64 kMin = 60000;

// idle reset 5 min, max work 50 min, break 5 min, postpone 10 min (defaults).
struct Fixture {
    FakeClock clock;
    BreakTimerEngine engine{clock};
    Fixture() { engine.setConfig({5 * kMin, 50 * kMin, 5 * kMin, 10 * kMin, false}); }

    // Simulates continuous use: activity every 30 s for 'ms', updating every second.
    void work(qint64 ms)
    {
        for (qint64 t = 0; t < ms; t += 1000) {
            if (t % 30000 == 0)
                engine.onActivity();
            clock.advanceMs(1000);
            engine.update();
        }
    }
    // No activity for 'ms', updating every second.
    void idle(qint64 ms)
    {
        for (qint64 t = 0; t < ms; t += 1000) {
            clock.advanceMs(1000);
            engine.update();
        }
    }
};

} // namespace

class TestBreakTimer : public QObject {
    Q_OBJECT
private slots:
    void waitsForTheFirstActivity()
    {
        Fixture f;
        QCOMPARE(f.engine.state(), S::WaitingForActivity);
        f.idle(60 * kMin);
        QCOMPARE(f.engine.state(), S::WaitingForActivity); // no activity, no cycle
        QCOMPARE(f.engine.interactionMs(), qint64(0));
        f.engine.onActivity();
        QCOMPARE(f.engine.state(), S::Working);
    }

    void interactionTimerIsWallClockSinceFirstActivity()
    {
        Fixture f;
        f.work(10 * kMin);
        QCOMPARE(f.engine.interactionMs(), 10 * kMin);
        f.idle(4 * kMin); // short pause (< idle reset) still counts
        QCOMPARE(f.engine.interactionMs(), 14 * kMin);
        QCOMPARE(f.engine.state(), S::Working);
        QCOMPARE(f.engine.timeUntilBreakMs(), 36 * kMin);
    }

    void breakAfterMaxWorkTime()
    {
        Fixture f;
        f.work(50 * kMin - 1000);
        QCOMPARE(f.engine.state(), S::Working);
        f.work(1000);
        QCOMPARE(f.engine.state(), S::BreakShown);
        QCOMPARE(f.engine.breakRemainingMs(), 5 * kMin);
    }

    void idleResetCountsAsABreak()
    {
        Fixture f;
        f.work(30 * kMin);
        f.idle(5 * kMin); // idle reset reached
        QCOMPARE(f.engine.state(), S::WaitingForActivity);
        f.engine.onActivity();
        QCOMPARE(f.engine.interactionMs(), qint64(0)); // new cycle
        f.work(30 * kMin);
        QCOMPARE(f.engine.state(), S::Working); // 30 + 30 never adds up to 50
    }

    void activityAfterALongGapWithoutUpdatesStartsANewCycle()
    {
        Fixture f;
        f.work(30 * kMin);
        f.clock.advanceMs(10 * kMin); // no update() in between (e.g. app suspended)
        f.engine.onActivity();
        QCOMPARE(f.engine.interactionMs(), qint64(0));
    }

    void sleepCountsAsIdle()
    {
        Fixture f;
        f.work(40 * kMin);
        f.engine.onSleep();
        f.clock.advanceMs(60 * kMin); // asleep: no updates
        f.engine.onResume();
        QCOMPARE(f.engine.state(), S::WaitingForActivity); // slept longer than the idle reset
    }

    void shortSleepKeepsTheCycle()
    {
        Fixture f;
        f.work(20 * kMin);
        f.engine.onSleep();
        f.clock.advanceMs(2 * kMin);
        f.engine.onResume();
        QCOMPARE(f.engine.state(), S::Working);
        QCOMPARE(f.engine.interactionMs(), 22 * kMin); // wall-clock time keeps counting
    }

    void lockCountsAsIdleAndIgnoresActivity()
    {
        Fixture f;
        f.work(40 * kMin);
        f.engine.onScreenLocked();
        for (int i = 0; i < 6; ++i) { // typing on the lock screen does not count
            f.engine.onActivity();
            f.idle(kMin);
        }
        QCOMPARE(f.engine.state(), S::WaitingForActivity);
        f.engine.onScreenUnlocked();
        f.engine.onActivity();
        QCOMPARE(f.engine.interactionMs(), qint64(0));
    }

    void noBreakStartsWhileLocked()
    {
        Fixture f;
        f.work(49 * kMin);
        f.engine.onScreenLocked();
        f.idle(2 * kMin); // max work time passes while locked (but idle < reset)
        QCOMPARE(f.engine.state(), S::Working);
        f.engine.onScreenUnlocked();
        QCOMPARE(f.engine.state(), S::BreakShown); // decided after unlock
    }

    void breakClosesAutomaticallyAndResets()
    {
        Fixture f;
        f.work(50 * kMin);
        QCOMPARE(f.engine.state(), S::BreakShown);
        f.work(5 * kMin - 1000); // activity during the break changes nothing (B-13)
        QCOMPARE(f.engine.state(), S::BreakShown);
        f.idle(1000);
        QCOMPARE(f.engine.state(), S::WaitingForActivity); // B-06
        QCOMPARE(f.engine.interactionMs(), qint64(0));
    }

    void dismissResetsImmediately()
    {
        Fixture f;
        f.work(50 * kMin);
        f.engine.dismiss();
        QCOMPARE(f.engine.state(), S::WaitingForActivity); // B-07
        f.engine.onActivity();
        QCOMPARE(f.engine.state(), S::Working);
        QCOMPARE(f.engine.interactionMs(), qint64(0));
    }

    void postponeReopensWhileTheTimerKeepsCounting()
    {
        Fixture f;
        f.work(50 * kMin);
        f.engine.postpone();
        QCOMPARE(f.engine.state(), S::Postponed);
        QCOMPARE(f.engine.timeUntilBreakMs(), 10 * kMin);
        f.work(10 * kMin - 1000);
        QCOMPARE(f.engine.state(), S::Postponed);
        QCOMPARE(f.engine.interactionMs(), 60 * kMin - 1000); // keeps counting (B-08)
        f.work(1000);
        QCOMPARE(f.engine.state(), S::BreakShown);
        f.engine.postpone(); // unlimited postpones
        QCOMPARE(f.engine.state(), S::Postponed);
    }

    void idleDuringPostponeCancelsIt()
    {
        Fixture f;
        f.work(50 * kMin);
        f.engine.postpone();
        f.idle(5 * kMin); // rested during the postpone (B-09)
        QCOMPARE(f.engine.state(), S::WaitingForActivity);
        f.idle(10 * kMin);
        QCOMPARE(f.engine.state(), S::WaitingForActivity); // no break pops up later
    }

    void maxWorkTimeChangeAppliesToTheCurrentCycle()
    {
        Fixture f;
        f.work(20 * kMin);
        BreakTimerEngine::Config c = f.engine.config();
        c.maxWorkMs = 15 * kMin; // already exceeded
        f.engine.setConfig(c);
        QCOMPARE(f.engine.state(), S::BreakShown);
        f.engine.dismiss();
        c.maxWorkMs = 60 * kMin;
        f.engine.setConfig(c);
        f.work(55 * kMin);
        QCOMPARE(f.engine.state(), S::Working);
    }

    void idleResetChangeAppliesImmediately()
    {
        Fixture f;
        f.work(10 * kMin);
        f.idle(2 * kMin);
        BreakTimerEngine::Config c = f.engine.config();
        c.idleResetMs = kMin;
        f.engine.setConfig(c);
        QCOMPARE(f.engine.state(), S::WaitingForActivity);
    }

    void disableEndsTheBreakAndResets()
    {
        Fixture f;
        f.work(50 * kMin);
        f.engine.setEnabled(false);
        QCOMPARE(f.engine.state(), S::Disabled);
        f.work(60 * kMin);
        QCOMPARE(f.engine.state(), S::Disabled);
        f.engine.setEnabled(true);
        QCOMPARE(f.engine.state(), S::WaitingForActivity);
        QCOMPARE(f.engine.interactionMs(), qint64(0));
    }

    void startBreakNow()
    {
        Fixture f;
        f.engine.startBreakNow(); // also without a running cycle
        QCOMPARE(f.engine.state(), S::BreakShown);
        f.engine.postpone();
        QCOMPARE(f.engine.state(), S::Postponed);
        f.engine.setEnabled(false);
        f.engine.startBreakNow();
        QCOMPARE(f.engine.state(), S::Disabled); // not while disabled
    }

    void fullscreenDelaysTheBreakWhenEnabled()
    {
        Fixture f;
        BreakTimerEngine::Config c = f.engine.config();
        c.suppressWhenFullscreen = true;
        f.engine.setConfig(c);
        f.engine.setFullscreenActive(true);
        f.work(55 * kMin);
        QCOMPARE(f.engine.state(), S::Working);
        QVERIFY(f.engine.isBreakDelayedByFullscreen());
        f.engine.setFullscreenActive(false);
        f.engine.update();
        QCOMPARE(f.engine.state(), S::BreakShown);
    }

    void fullscreenIgnoredWhenTheOptionIsOff()
    {
        Fixture f;
        f.engine.setFullscreenActive(true);
        f.work(50 * kMin);
        QCOMPARE(f.engine.state(), S::BreakShown);
    }
};

QTEST_GUILESS_MAIN(TestBreakTimer)
#include "tst_breaktimer.moc"
