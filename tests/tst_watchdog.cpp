#include "core/input/HookWatchdog.h"

#include <QTest>

using namespace ergo;

class TestWatchdog : public QObject {
    Q_OBJECT
private slots:
    void healthyHookNeedsNothing()
    {
        FakeClock clock;
        HookWatchdog w(clock);
        QCOMPARE(w.config().checkIntervalMs, 30000);
        QCOMPARE(w.onHealthCheck(true), HookWatchdog::Action::None);
        QCOMPARE(w.reinstallCount(), 0);
    }

    void unhealthyHookIsReinstalled()
    {
        FakeClock clock;
        HookWatchdog w(clock);
        QCOMPARE(w.onHealthCheck(false), HookWatchdog::Action::Reinstall);
        QCOMPARE(w.reinstallCount(), 1);
        w.onReinstallResult(true);
        QCOMPARE(w.consecutiveFailures(), 0);
    }

    void resumeAndUnlockReinstallButNotTwiceInARow()
    {
        FakeClock clock;
        HookWatchdog w(clock);
        QCOMPARE(w.onResume(), HookWatchdog::Action::Reinstall);
        clock.advanceMs(1000); // unlock right after resume
        QCOMPARE(w.onUnlock(), HookWatchdog::Action::None);
        clock.advanceMs(w.config().minReinstallSpacingMs);
        QCOMPARE(w.onUnlock(), HookWatchdog::Action::Reinstall);
        QCOMPARE(w.reinstallCount(), 2);
    }

    void persistentFailureAfterRepeatedFailedReinstalls()
    {
        FakeClock clock;
        HookWatchdog w(clock);
        for (int i = 0; i < w.config().maxConsecutiveFailures; ++i) {
            QVERIFY(!w.persistentFailure());
            QCOMPARE(w.onHealthCheck(false), HookWatchdog::Action::Reinstall);
            w.onReinstallResult(false);
            clock.advanceMs(w.config().checkIntervalMs);
        }
        QVERIFY(w.persistentFailure());
        // Attempts continue; one success clears the failure.
        QCOMPARE(w.onHealthCheck(false), HookWatchdog::Action::Reinstall);
        w.onReinstallResult(true);
        QVERIFY(!w.persistentFailure());
    }

    void healthyCheckClearsFailures()
    {
        FakeClock clock;
        HookWatchdog w(clock);
        w.onHealthCheck(false);
        w.onReinstallResult(false);
        QCOMPARE(w.consecutiveFailures(), 1);
        clock.advanceMs(30000);
        QCOMPARE(w.onHealthCheck(true), HookWatchdog::Action::None);
        QCOMPARE(w.consecutiveFailures(), 0);
    }
};

QTEST_GUILESS_MAIN(TestWatchdog)
#include "tst_watchdog.moc"
