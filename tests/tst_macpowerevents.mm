// macOS only: MacPowerEvents must keep working after start() has returned
// (1.7.3: a screen lock crashed the app with std::bad_function_call because the
// notification blocks referred to start()'s destroyed callback parameter).
//
// Notifications are posted to this process's own NSWorkspace notification
// center only - no other application sees them.

#include "platform/mac/MacInput.h"

#import <AppKit/AppKit.h>

#include <QCoreApplication>
#include <QList>
#include <QTest>

#include <cstring>

using namespace ergo;

namespace {

// Overwrites the stack area that start()'s parameters used, so a dangling
// reference to them no longer finds the old values.
__attribute__((noinline)) void clobberStack()
{
    volatile char buffer[64 * 1024];
    std::memset(const_cast<char *>(buffer), 0, sizeof buffer);
}

__attribute__((noinline)) void startWith(MacPowerEvents &events, QList<PowerEvent> *received)
{
    events.start([received](PowerEvent e) { received->append(e); }); // temporary std::function
}

void post(NSString *name)
{
    [NSWorkspace.sharedWorkspace.notificationCenter postNotificationName:name object:nil];
}

} // namespace

class TestMacPowerEvents : public QObject {
    Q_OBJECT
private slots:
    void notificationsAfterStartReturnedReachTheCallback()
    {
        MacPowerEvents events;
        QList<PowerEvent> received;
        startWith(events, &received);
        clobberStack();

        post(NSWorkspaceSessionDidResignActiveNotification); // screen locked
        post(NSWorkspaceSessionDidBecomeActiveNotification); // unlocked
        post(NSWorkspaceWillSleepNotification);
        post(NSWorkspaceDidWakeNotification);
        QTRY_COMPARE(received.size(), 4);
        QCOMPARE(received, (QList<PowerEvent>{PowerEvent::ScreenLocked, PowerEvent::ScreenUnlocked,
                                              PowerEvent::Sleep, PowerEvent::Resume}));
    }

    void stopRemovesTheObservers()
    {
        MacPowerEvents events;
        QList<PowerEvent> received;
        startWith(events, &received);
        events.stop();
        post(NSWorkspaceSessionDidResignActiveNotification);
        QTest::qWait(100);
        QVERIFY(received.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestMacPowerEvents)
#include "tst_macpowerevents.moc"
