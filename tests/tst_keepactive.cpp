#include "core/keepactive/KeepScreenActiveEngine.h"
#include "core/settings/KeepScreenActiveSettings.h"
#include "core/settings/SettingsRegistry.h"
#include "core/settings/SettingsSchema.h"

#include <QTest>

using namespace ergo;

namespace {
constexpr qint64 kSec = 1000;
constexpr qint64 kMin = 60 * kSec;
using E = KeepScreenActiveEngine;
} // namespace

// Keep Screen Active (B.10, stage 9): when the cycles are due.
class TestKeepActive : public QObject {
    Q_OBJECT

    struct Fixture {
        FakeClock clock;
        E engine{clock};
        int cycles = 0;
        Fixture()
        {
            engine.setConfig({3 * kMin, 3 * kMin});
            engine.setEnabled(true);
        }
        // Advances second by second, counting due cycles.
        void advance(qint64 ms)
        {
            for (qint64 t = 0; t < ms; t += kSec) {
                clock.advanceMs(kSec);
                if (engine.update())
                    ++cycles;
            }
        }
    };

private slots:
    void disabledNeverCycles()
    {
        FakeClock clock;
        E e(clock);
        e.setConfig({kMin, kMin});
        clock.advanceMs(30 * kMin);
        QVERIFY(!e.update());
        QCOMPARE(e.state(), E::State::Disabled);
    }

    void firstCycleExactlyAfterTheIdleTimeThenEveryInterval()
    {
        Fixture f;
        f.advance(3 * kMin - kSec);
        QCOMPARE(f.cycles, 0);
        QCOMPARE(f.engine.state(), E::State::Idle);
        f.advance(kSec);
        QCOMPARE(f.cycles, 1); // immediately when the idle time is reached
        QCOMPARE(f.engine.state(), E::State::Working);
        f.advance(3 * kMin - kSec);
        QCOMPARE(f.cycles, 1);
        f.advance(kSec);
        QCOMPARE(f.cycles, 2); // then every interval
        f.advance(9 * kMin);
        QCOMPARE(f.cycles, 5);
    }

    void realInputStopsWorkingAndRestartsTheIdleTime()
    {
        Fixture f;
        f.advance(4 * kMin);
        QCOMPARE(f.engine.state(), E::State::Working);
        f.engine.onRealInput();
        QCOMPARE(f.engine.state(), E::State::Idle);
        QCOMPARE(f.engine.idleMs(), 0);
        const int before = f.cycles;
        f.advance(3 * kMin - kSec);
        QCOMPARE(f.cycles, before);
        f.advance(kSec);
        QCOMPARE(f.cycles, before + 1);
    }

    void pausesStopCyclesAndResumeWithAnImmediateCycle_data()
    {
        QTest::addColumn<int>("reason");
        QTest::newRow("locked") << int(E::Locked);
        QTest::newRow("break window") << int(E::BreakWindow);
        QTest::newRow("drag lock") << int(E::DragLock);
    }

    void pausesStopCyclesAndResumeWithAnImmediateCycle()
    {
        QFETCH(int, reason);
        Fixture f;
        f.advance(3 * kMin);
        QCOMPARE(f.cycles, 1);
        f.engine.setPaused(E::PauseReason(reason), true);
        QCOMPARE(f.engine.state(), E::State::Idle);
        f.advance(10 * kMin);
        QCOMPARE(f.cycles, 1); // nothing while paused
        f.engine.setPaused(E::PauseReason(reason), false);
        QVERIFY(f.engine.update()); // still idle: immediate cycle
        QCOMPARE(f.engine.state(), E::State::Working);
    }

    void twoPauseReasonsMustBothEnd()
    {
        Fixture f;
        f.engine.setPaused(E::Locked, true);
        f.engine.setPaused(E::BreakWindow, true);
        f.advance(5 * kMin);
        f.engine.setPaused(E::Locked, false);
        QVERIFY(!f.engine.update());
        f.engine.setPaused(E::BreakWindow, false);
        QVERIFY(f.engine.update());
    }

    void sleepCountsAsIdleTime()
    {
        Fixture f;
        f.engine.setPaused(E::Locked, true);
        f.clock.advanceMs(60 * kMin); // asleep, no ticks
        f.engine.setPaused(E::Locked, false);
        QVERIFY(f.engine.update());
    }

    void enablingCountsFromNowAndDisablingStops()
    {
        FakeClock clock;
        E e(clock);
        e.setConfig({3 * kMin, 3 * kMin});
        clock.advanceMs(30 * kMin); // idle while disabled
        e.setEnabled(true);         // the hotkey press is input
        QVERIFY(!e.update());
        clock.advanceMs(3 * kMin);
        QVERIFY(e.update());
        e.setEnabled(false);
        clock.advanceMs(10 * kMin);
        QVERIFY(!e.update());
        QCOMPARE(e.state(), E::State::Disabled);
    }

    void titleBarPoint()
    {
        QPointF p;
        QVERIFY(keepActiveTitleBarPoint(QRectF(100, 50, 1200, 800), KeepActiveApp::Teams, &p));
        QCOMPARE(p, QPointF(100 + 1200 * 0.22, 60));
        QVERIFY(keepActiveTitleBarPoint(QRectF(0, 0, 400, 300), KeepActiveApp::Zoom, &p));
        QCOMPARE(p, QPointF(140, 10)); // at least 140 px from the left edge
        QVERIFY(!keepActiveTitleBarPoint(QRectF(0, 0, 300, 300), KeepActiveApp::Teams, &p)); // too small
    }

    void settings()
    {
        SettingsSchema schema;
        registerAllSettings(schema);
        namespace KS = KeepScreenActiveSettings;
        const SettingDefinition *enabled = schema.find(KS::Enabled);
        const SettingDefinition *start = schema.find(KS::StartAfterIdleMinutes);
        const SettingDefinition *interval = schema.find(KS::IntervalMinutes);
        QVERIFY(enabled && start && interval);
        QCOMPARE(enabled->defaultValue.toBool(), false);
        QCOMPARE(start->defaultValue.toInt(), 3);
        QCOMPARE(interval->defaultValue.toInt(), 3);
        QVERIFY(SettingsSchema::validate(*start, 1) && SettingsSchema::validate(*start, 60));
        QVERIFY(!SettingsSchema::validate(*start, 0) && !SettingsSchema::validate(*start, 61));
        QVERIFY(SettingsSchema::validate(*interval, 1) && SettingsSchema::validate(*interval, 60));
        QVERIFY(!SettingsSchema::validate(*interval, 0) && !SettingsSchema::validate(*interval, 61));
    }
};

QTEST_GUILESS_MAIN(TestKeepActive)
#include "tst_keepactive.moc"
