#include "core/breaktimer/BreakReminder.h"
#include "core/settings/BreakTimerSettings.h"
#include "core/settings/SettingsRegistry.h"
#include "core/settings/SettingsSchema.h"

#include <QtTest>

using namespace ergo;
using State = BreakTimerEngine::State;

namespace {
BreakReminder::Input working(qint64 untilMs, bool paused = false)
{
    BreakReminder::Input in;
    in.state = State::Working;
    in.timeUntilBreakMs = untilMs;
    in.workTimePaused = paused;
    in.leadMs = 60000;
    return in;
}
} // namespace

// 2.3.0 (owner request): reminder with a countdown before the break window.
class TestBreakReminder : public QObject {
    Q_OBJECT

private slots:
    void shownOnlyWithinTheLeadTime()
    {
        BreakReminder r;
        QVERIFY(!r.update(working(61000)).show);
        const BreakReminder::Output out = r.update(working(60000));
        QVERIFY(out.show);
        QCOMPARE(out.remainingMs, 60000);
        QVERIFY(r.update(working(1000)).show);
        QVERIFY(!r.update(working(0)).show); // break due (e.g. delayed by a full-screen app)
    }

    void pausedFreezesTheCountdown()
    {
        BreakReminder r;
        const BreakReminder::Output out = r.update(working(42000, true));
        QVERIFY(out.show);
        QVERIFY(out.paused);
        QCOMPARE(out.remainingMs, 42000);
    }

    void dismissedForTheRestOfThePeriod()
    {
        BreakReminder r;
        QVERIFY(r.update(working(50000)).show);
        r.dismiss();
        QVERIFY(!r.update(working(40000)).show);
        QVERIFY(!r.update(working(70000)).show);
        QVERIFY(!r.update(working(30000)).show); // still the same period
        // Break window, then a new work cycle: shown again.
        BreakReminder::Input brk;
        brk.state = State::BreakShown;
        brk.timeUntilBreakMs = 0;
        QVERIFY(!r.update(brk).show);
        BreakReminder::Input waiting;
        waiting.state = State::WaitingForActivity;
        QVERIFY(!r.update(waiting).show);
        QVERIFY(r.update(working(50000)).show);
    }

    void postponedBreakGetsItsOwnReminder()
    {
        BreakReminder r;
        r.update(working(50000));
        r.dismiss();
        BreakReminder::Input brk;
        brk.state = State::BreakShown;
        brk.timeUntilBreakMs = 0;
        r.update(brk);
        BreakReminder::Input postponed;
        postponed.state = State::Postponed;
        postponed.timeUntilBreakMs = 10 * 60000;
        QVERIFY(!r.update(postponed).show); // not yet
        postponed.timeUntilBreakMs = 59000;
        QVERIFY(r.update(postponed).show);
        r.dismiss();
        QVERIFY(!r.update(postponed).show);
    }

    void dismissIsResetWhenTheCycleResetsWithoutABreak()
    {
        BreakReminder r;
        r.update(working(50000));
        r.dismiss();
        BreakReminder::Input waiting; // long pause: the work period was reset
        waiting.state = State::WaitingForActivity;
        r.update(waiting);
        QVERIFY(r.update(working(50000)).show);
    }

    void cycleResetSignalStartsANewPeriod()
    {
        BreakReminder r;
        r.update(working(50000));
        r.dismiss();
        r.startNewPeriod(); // BreakTimerController::workCycleReset
        QVERIFY(r.update(working(50000)).show);
    }

    void switchedOffOrDisabledTimer()
    {
        BreakReminder r;
        BreakReminder::Input in = working(30000);
        in.enabled = false;
        QVERIFY(!r.update(in).show);
        BreakReminder::Input off;
        off.state = State::Disabled;
        QVERIFY(!r.update(off).show);
        in.leadMs = 15000; // shorter lead time
        in.enabled = true;
        QVERIFY(!r.update(in).show);
        in.timeUntilBreakMs = 15000;
        QVERIFY(r.update(in).show);
    }

    void positionNextToTheTray()
    {
        const QSize w(260, 80);
        const QRect screen(0, 0, 1920, 1080);
        // macOS: menu bar at the top (25 px), Dock at the bottom (70 px), tray icon in the menu bar: top right.
        QCOMPARE(breakReminderPosition(screen, QRect(0, 25, 1920, 985), w, 12, QRect(1700, 2, 22, 22)),
                 QPoint(1920 - 260 - 12, 25 + 12));
        // Windows: taskbar at the bottom.
        QCOMPARE(breakReminderPosition(screen, QRect(0, 0, 1920, 1032), w, 12), QPoint(1920 - 260 - 12, 1032 - 80 - 12));
        // macOS without a visible Dock / GNOME: panel at the top.
        QCOMPARE(breakReminderPosition(screen, QRect(0, 25, 1920, 1055), w, 12), QPoint(1920 - 260 - 12, 25 + 12));
        // Second screen to the left (negative coordinates), no panel: bottom right.
        QCOMPARE(breakReminderPosition(QRect(-1512, 0, 1512, 982), QRect(-1512, 0, 1512, 982), w, 12),
                 QPoint(-12 - 260, 982 - 80 - 12));
    }

    void settings()
    {
        SettingsSchema schema;
        registerAllSettings(schema);
        namespace BT = BreakTimerSettings;
        const SettingDefinition *enabled = schema.find(BT::ReminderEnabled);
        const SettingDefinition *seconds = schema.find(BT::ReminderSeconds);
        QVERIFY(enabled && seconds);
        QCOMPARE(enabled->defaultValue.toBool(), true);
        QCOMPARE(seconds->defaultValue.toInt(), 60);
        QCOMPARE(seconds->minimum, 15);
        QCOMPARE(seconds->maximum, 300);
    }
};

QTEST_GUILESS_MAIN(TestBreakReminder)
#include "tst_breakreminder.moc"
