#include "core/breaktimer/MicroBreakEngine.h"
#include "core/settings/MicroBreakSettings.h"
#include "core/settings/SettingsRegistry.h"
#include "core/settings/SettingsSchema.h"

#include <QtTest>

using namespace ergo;

namespace {
constexpr qint64 kSec = 1000;
constexpr qint64 kMin = 60 * kSec;
using E = MicroBreakEngine;
using State = E::State;

struct Fixture {
    FakeClock clock;
    E engine{clock};
    Fixture(qint64 intervalMs = 10 * kMin, qint64 showMs = 20 * kSec)
    {
        E::Config c;
        c.intervalMs = intervalMs;
        c.showMs = showMs;
        c.pauseAfterMs = 20 * kSec;
        engine.setConfig(c);
        engine.setEnabled(true);
    }
    // Input once per second for 'ms', the last input at the end.
    void work(qint64 ms)
    {
        for (qint64 t = 0; t < ms; t += kSec) {
            engine.onActivity();
            clock.advanceMs(kSec);
            engine.update();
        }
        engine.onActivity();
    }
    // No input for 'ms'.
    void idle(qint64 ms)
    {
        for (qint64 t = 0; t < ms; t += kSec) {
            clock.advanceMs(kSec);
            engine.update();
        }
    }
};
} // namespace

// 2.5.0 (owner request): micro-breaks, a short pause every few minutes of active use.
class TestMicroBreak : public QObject {
    Q_OBJECT

private slots:
    void shownAfterTheIntervalOfActiveUse()
    {
        Fixture f;
        QCOMPARE(f.engine.state(), State::Waiting);
        QCOMPARE(f.engine.timeUntilMs(), qint64(-1));
        f.work(10 * kMin - kSec);
        QCOMPARE(f.engine.state(), State::Working);
        QCOMPARE(f.engine.timeUntilMs(), kSec);
        f.work(kSec);
        QCOMPARE(f.engine.state(), State::Shown);
        QCOMPARE(f.engine.shownRemainingMs(), 20 * kSec);
    }

    void shortPausesCountOnlyUpToPauseAfter()
    {
        Fixture f(10 * kMin, 60 * kSec); // micro-break 60 s: a 45 s pause is no rest
        f.work(kMin);
        f.idle(45 * kSec);
        QCOMPARE(f.engine.state(), State::Working);
        QVERIFY(f.engine.isPaused());
        QCOMPARE(f.engine.activeMs(), kMin + 20 * kSec); // the pause counts up to 20 s
        f.work(kSec);
        QVERIFY(!f.engine.isPaused());
    }

    void aPauseAsLongAsTheMicroBreakRestartsTheCount()
    {
        Fixture f;
        f.work(5 * kMin);
        f.idle(19 * kSec);
        QCOMPARE(f.engine.state(), State::Working);
        f.idle(kSec); // 20 s without input = the micro-break was taken
        QCOMPARE(f.engine.state(), State::Waiting);
        QCOMPARE(f.engine.activeMs(), qint64(0));
        f.work(10 * kMin - kSec);
        QCOMPARE(f.engine.state(), State::Working);
    }

    void closesAfterTheShowTimeAndInputDoesNotCount()
    {
        Fixture f;
        f.work(10 * kMin);
        QCOMPARE(f.engine.state(), State::Shown);
        f.work(19 * kSec); // working on during the micro-break: no effect
        QCOMPARE(f.engine.state(), State::Shown);
        QCOMPARE(f.engine.shownRemainingMs(), kSec);
        f.idle(kSec);
        QCOMPARE(f.engine.state(), State::Waiting);
        f.work(10 * kMin - kSec); // a full interval again
        QCOMPARE(f.engine.state(), State::Working);
        f.work(kSec);
        QCOMPARE(f.engine.state(), State::Shown);
    }

    void dismissClosesIt()
    {
        Fixture f;
        f.work(10 * kMin);
        f.engine.dismiss();
        QCOMPARE(f.engine.state(), State::Waiting);
        f.engine.dismiss(); // nothing shown: no change
        QCOMPARE(f.engine.state(), State::Waiting);
    }

    void givesWayToTheBreakTimer()
    {
        Fixture f;
        f.work(9 * kMin);
        f.engine.setBlocked(true); // break reminder / break window shown
        f.work(2 * kMin);
        QCOMPARE(f.engine.state(), State::Working); // due, but waits
        f.engine.setBlocked(false);
        f.engine.update();
        QCOMPARE(f.engine.state(), State::Shown);
        f.engine.setBlocked(true); // the break reminder appears: the micro-break closes
        QCOMPARE(f.engine.state(), State::Waiting);
    }

    void breakTimerResetRestartsTheCount()
    {
        Fixture f;
        f.work(9 * kMin);
        f.engine.restart();
        QCOMPARE(f.engine.state(), State::Waiting);
        f.work(10 * kMin - kSec);
        QCOMPARE(f.engine.state(), State::Working);
        f.work(kSec);
        f.engine.restart(); // a shown micro-break is not closed by it
        QCOMPARE(f.engine.state(), State::Shown);
    }

    void lockOrSleepRestartsTheCount()
    {
        Fixture f;
        f.work(9 * kMin);
        f.engine.setLocked(true);
        QCOMPARE(f.engine.state(), State::Waiting);
        f.work(kMin); // input while locked is ignored
        QCOMPARE(f.engine.state(), State::Waiting);
        f.engine.setLocked(false);
        f.work(10 * kMin);
        QCOMPARE(f.engine.state(), State::Shown);
        f.engine.setLocked(true);
        QCOMPARE(f.engine.state(), State::Waiting);
    }

    void disabledAndConfigChanges()
    {
        Fixture f;
        f.engine.setEnabled(false);
        f.work(20 * kMin);
        QCOMPARE(f.engine.state(), State::Disabled);
        f.engine.setEnabled(true);
        QCOMPARE(f.engine.state(), State::Waiting);
        f.work(5 * kMin);
        E::Config c = f.engine.config();
        c.intervalMs = 4 * kMin; // shorter interval: due at once
        f.engine.setConfig(c);
        QCOMPARE(f.engine.state(), State::Shown);
        f.work(20 * kSec);
        f.engine.setEnabled(false);
        QCOMPARE(f.engine.state(), State::Disabled);
    }

    void settings()
    {
        SettingsSchema schema;
        registerAllSettings(schema);
        namespace MB = MicroBreakSettings;
        const SettingDefinition *enabled = schema.find(MB::Enabled);
        const SettingDefinition *interval = schema.find(MB::IntervalMinutes);
        const SettingDefinition *show = schema.find(MB::ShowSeconds);
        QVERIFY(enabled && interval && show);
        QCOMPARE(enabled->defaultValue.toBool(), false);
        QCOMPARE(interval->defaultValue.toInt(), 10);
        QCOMPARE(interval->minimum, 1);
        QCOMPARE(interval->maximum, 60);
        QVERIFY(interval->clampToRange);
        QCOMPARE(show->defaultValue.toInt(), 20);
        QCOMPARE(show->minimum, 10);
        QCOMPARE(show->maximum, 120);
        QVERIFY(show->clampToRange);
    }
};

QTEST_GUILESS_MAIN(TestMicroBreak)
#include "tst_microbreak.moc"
