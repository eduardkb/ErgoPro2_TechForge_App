#include "core/autoclick/AutoClickEngine.h"

#include <QTest>

using namespace ergo;

namespace {

InputEvent move(double x, double y)
{
    InputEvent e;
    e.type = InputEventType::MouseMove;
    e.position = QPointF(x, y);
    return e;
}

InputEvent button(InputEventType type, double x, double y, MouseButtonId b = MouseButtonId::Left)
{
    InputEvent e;
    e.type = type;
    e.button = b;
    e.position = QPointF(x, y);
    return e;
}

InputEvent keyDown(int key)
{
    InputEvent e;
    e.type = InputEventType::KeyDown;
    e.key = key;
    return e;
}

// Engine with distance 15 px, jitter 3 px, trigger 200 ms, typing 500 ms.
struct Fixture {
    FakeClock clock;
    AutoClickEngine engine{clock};
    Fixture()
    {
        engine.setConfig({200, 15, 3, 500});
        engine.setEnabled(true);
        engine.onInputEvent(move(100, 100)); // first position = reference
    }
    // Advances time and returns how many clicks poll() reported.
    int runFor(qint64 ms)
    {
        int clicks = 0;
        for (qint64 t = 0; t < ms; t += 10) {
            clock.advanceMs(10);
            if (engine.poll())
                ++clicks;
        }
        return clicks;
    }
};

} // namespace

class TestAutoClick : public QObject {
    Q_OBJECT
private slots:
    void disabledNeverClicks()
    {
        FakeClock clock;
        AutoClickEngine engine(clock);
        QCOMPARE(engine.state(), AutoClickEngine::State::Disabled);
        engine.onInputEvent(move(0, 0));
        engine.onInputEvent(move(500, 500));
        clock.advanceMs(5000);
        QVERIFY(!engine.poll());
        QCOMPARE(engine.nextDeadlineMs(), qint64(-1));
    }

    void firstPositionIsTheReference()
    {
        Fixture f;
        QVERIFY(f.engine.hasReference());
        QCOMPARE(f.engine.referencePoint(), QPointF(100, 100));
        QCOMPARE(f.engine.state(), AutoClickEngine::State::WaitingForMovement);
        QCOMPARE(f.runFor(1000), 0); // resting at the start: no click
    }

    void armsAtDistanceAndClicksOnceAfterTriggerTime()
    {
        Fixture f;
        f.engine.onInputEvent(move(110, 100)); // 10 px: not armed yet
        QVERIFY(!f.engine.isArmed());
        f.engine.onInputEvent(move(115, 100)); // 15 px: armed
        QVERIFY(f.engine.isArmed());
        QCOMPARE(f.engine.state(), AutoClickEngine::State::Armed);

        f.clock.advanceMs(190);
        QVERIFY(!f.engine.poll()); // before the trigger time
        QCOMPARE(f.engine.state(), AutoClickEngine::State::Countdown);
        f.clock.advanceMs(10);
        QVERIFY(f.engine.poll()); // exactly one click after 200 ms
        QCOMPARE(f.engine.referencePoint(), QPointF(115, 100));
        QCOMPARE(f.engine.state(), AutoClickEngine::State::WaitingForMovement);
        QCOMPARE(f.runFor(3000), 0); // resting: no repeated clicks
    }

    void distanceIsStraightLine()
    {
        Fixture f;
        f.engine.onInputEvent(move(109, 109)); // 12.7 px diagonal
        QVERIFY(!f.engine.isArmed());
        f.engine.onInputEvent(move(111, 111)); // 15.6 px diagonal
        QVERIFY(f.engine.isArmed());
    }

    void movementRestartsTheTimerButJitterDoesNot()
    {
        Fixture f;
        f.engine.onInputEvent(move(130, 100)); // armed
        f.clock.advanceMs(150);
        f.engine.onInputEvent(move(150, 100)); // real movement: timer restarts
        f.clock.advanceMs(150);
        QVERIFY(!f.engine.poll()); // only 150 ms since the last movement
        f.engine.onInputEvent(move(152, 101)); // 2.2 px tremor: ignored
        f.clock.advanceMs(50);
        QVERIFY(f.engine.poll()); // 200 ms since the last REAL movement
    }

    void jitterSmallerThanToleranceNeverArms()
    {
        Fixture f;
        for (int i = 0; i < 50; ++i) {
            f.engine.onInputEvent(move(100 + (i % 2) * 3, 100)); // 3 px back and forth
            f.clock.advanceMs(20);
        }
        QVERIFY(!f.engine.isArmed());
        QCOMPARE(f.runFor(1000), 0);
    }

    void manualClickCancelsAndMovesTheReference()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100)); // armed
        f.clock.advanceMs(100);
        f.engine.onInputEvent(button(InputEventType::MouseDown, 140, 100, MouseButtonId::Right));
        f.engine.onInputEvent(button(InputEventType::MouseUp, 140, 100, MouseButtonId::Right));
        QVERIFY(!f.engine.isArmed());
        QCOMPARE(f.engine.referencePoint(), QPointF(140, 100));
        QCOMPARE(f.runFor(1000), 0);
        // The pointer must travel the distance again from the click position.
        f.engine.onInputEvent(move(150, 100));
        QVERIFY(!f.engine.isArmed());
        f.engine.onInputEvent(move(156, 100));
        QVERIFY(f.engine.isArmed());
    }

    void manualDragLeavesTheReferenceAtTheDropPosition()
    {
        Fixture f;
        f.engine.onInputEvent(button(InputEventType::MouseDown, 100, 100));
        f.engine.onInputEvent(move(300, 300));
        QCOMPARE(f.engine.state(), AutoClickEngine::State::Suppressed);
        f.engine.onInputEvent(button(InputEventType::MouseUp, 300, 300));
        QCOMPARE(f.engine.referencePoint(), QPointF(300, 300));
        QCOMPARE(f.runFor(1000), 0); // no click at the drop position
    }

    void noClickWhileAButtonIsHeld()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100)); // armed
        f.engine.onInputEvent(button(InputEventType::MouseDown, 140, 100));
        f.engine.onInputEvent(move(200, 100)); // moving with the button held arms again
        QVERIFY(f.engine.isArmed());
        QCOMPARE(f.engine.state(), AutoClickEngine::State::Suppressed);
        QCOMPARE(f.engine.suppressReason(), AutoClickEngine::SuppressReason::ButtonHeld);
        QCOMPARE(f.engine.nextDeadlineMs(), qint64(-1));
        QCOMPARE(f.runFor(1000), 0);
    }

    void typingDelaysAPendingClick()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100)); // armed, deadline in 200 ms
        f.clock.advanceMs(100);
        f.engine.onInputEvent(keyDown(Qt::Key_A)); // suspend until +500 ms
        QCOMPARE(f.engine.suppressReason(), AutoClickEngine::SuppressReason::Typing);
        QCOMPARE(f.runFor(490), 0);
        QCOMPARE(f.runFor(20), 1); // click right after the suspension ends
    }

    void continuousTypingKeepsSuspending()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100));
        int clicks = 0;
        for (int i = 0; i < 20; ++i) {
            f.engine.onInputEvent(keyDown(Qt::Key_A));
            clicks += f.runFor(300);
        }
        QCOMPARE(clicks, 0);
    }

    void modifierKeysDoNotSuspend()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100));
        f.engine.onInputEvent(keyDown(Qt::Key_Shift));
        f.engine.onInputEvent(keyDown(Qt::Key_Meta));
        QCOMPARE(f.engine.suppressReason(), AutoClickEngine::SuppressReason::None);
        QCOMPARE(f.runFor(210), 1);
    }

    void syntheticEventsAreIgnored()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100)); // armed
        InputEvent ownClick = button(InputEventType::MouseDown, 140, 100);
        ownClick.synthetic = true;
        f.engine.onInputEvent(ownClick); // must not count as a manual click
        QVERIFY(f.engine.isArmed());
        InputEvent ownMove = move(400, 400);
        ownMove.synthetic = true;
        f.engine.onInputEvent(ownMove);
        QCOMPARE(f.engine.position(), QPointF(140, 100));
    }

    void keyControlClickBehavesLikeAManualClick()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100)); // armed
        f.engine.onExternalClick(QPointF(140, 100));
        QVERIFY(!f.engine.isArmed());
        QCOMPARE(f.engine.referencePoint(), QPointF(140, 100));
        QCOMPARE(f.runFor(1000), 0);
    }

    void dragLockSuppressesAndResetsTheReferenceOnRelease()
    {
        Fixture f;
        f.engine.setDragLockActive(true, QPointF(100, 100));
        f.engine.onInputEvent(move(300, 100)); // armed while dragging
        QCOMPARE(f.engine.suppressReason(), AutoClickEngine::SuppressReason::DragLock);
        QCOMPARE(f.runFor(1000), 0); // never clicks during Drag Lock
        f.engine.setDragLockActive(false, QPointF(300, 100));
        QVERIFY(!f.engine.isArmed());
        QCOMPARE(f.engine.referencePoint(), QPointF(300, 100));
        QCOMPARE(f.runFor(1000), 0); // no click on release
        f.engine.onInputEvent(move(310, 100));
        QVERIFY(!f.engine.isArmed()); // must travel the full distance from the release position
        f.engine.onInputEvent(move(316, 100));
        QVERIFY(f.engine.isArmed());
    }

    void resetReferencePoint()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100)); // armed
        f.engine.resetReferencePoint(QPointF(500, 500));
        QVERIFY(!f.engine.isArmed());
        QCOMPARE(f.engine.referencePoint(), QPointF(500, 500));
    }

    void triggerTimeChangeWhileArmedAppliesImmediately()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100)); // armed at t0
        f.clock.advanceMs(150);
        f.engine.setConfig({1000, 15, 3, 500}); // longer trigger
        QCOMPARE(f.runFor(500), 0);
        f.engine.setConfig({100, 15, 3, 500}); // now shorter than the elapsed time
        f.clock.advanceMs(1);
        QVERIFY(f.engine.poll());
    }

    void distanceChangeAppliesToTheNextArming()
    {
        Fixture f;
        f.engine.setConfig({200, 50, 3, 500});
        f.engine.onInputEvent(move(140, 100)); // 40 px < 50
        QVERIFY(!f.engine.isArmed());
        f.engine.onInputEvent(move(151, 100)); // 51 px
        QVERIFY(f.engine.isArmed());
    }

    void disableCancelsAndReEnableUsesANewReference()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100)); // armed
        f.engine.setEnabled(false);
        QCOMPARE(f.runFor(1000), 0);
        f.engine.setEnabled(true);
        QVERIFY(!f.engine.hasReference());
        f.engine.onInputEvent(move(400, 400)); // first position after enabling
        QCOMPARE(f.engine.referencePoint(), QPointF(400, 400));
        QVERIFY(!f.engine.isArmed());
    }

    void countdownProgressAndRemainingTime()
    {
        Fixture f;
        f.engine.onInputEvent(move(140, 100));
        f.clock.advanceMs(100);
        QCOMPARE(f.engine.state(), AutoClickEngine::State::Countdown);
        QCOMPARE(f.engine.countdownProgress(), 0.5);
        QCOMPARE(f.engine.remainingMs(), 100);
    }
};

QTEST_GUILESS_MAIN(TestAutoClick)
#include "tst_autoclick.moc"
