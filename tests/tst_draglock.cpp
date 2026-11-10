#include "core/autoclick/AutoClickEngine.h"
#include "core/input/KeyCombo.h"
#include "core/keycontrol/DragLockEngine.h"

#include <QTest>

using namespace ergo;

namespace {

using Reason = DragLockEngine::ReleaseReason;

// Records what the drag lock does to the "system".
struct FakeDevice : IDragLockDevice {
    QStringList calls;
    bool holdingFlag = false;
    bool failPress = false;
    bool failRelease = false;
    OpResult pressLeft() override
    {
        calls << QStringLiteral("down");
        return failPress ? OpResult::failure(QStringLiteral("no permission")) : OpResult::success();
    }
    OpResult releaseLeft() override
    {
        calls << QStringLiteral("up");
        return failRelease ? OpResult::failure(QStringLiteral("no permission")) : OpResult::success();
    }
    void setHolding(bool holding) override
    {
        holdingFlag = holding;
        calls << (holding ? QStringLiteral("hold") : QStringLiteral("unhold"));
    }
};

InputEvent move(double x, double y)
{
    InputEvent e;
    e.type = InputEventType::MouseMove;
    e.position = QPointF(x, y);
    return e;
}

InputEvent button(InputEventType type, MouseButtonId b, double x = 0, double y = 0, bool synthetic = false)
{
    InputEvent e;
    e.type = type;
    e.button = b;
    e.position = QPointF(x, y);
    e.synthetic = synthetic;
    return e;
}

InputEvent key(int qtKey, bool swallowed = false, bool repeat = false, InputEventType type = InputEventType::KeyDown)
{
    InputEvent e;
    e.type = type;
    e.key = qtKey;
    e.modifiers = modifierForKey(qtKey);
    e.swallowed = swallowed;
    e.autoRepeat = repeat;
    return e;
}

struct Fixture {
    FakeClock clock;
    FakeDevice device;
    DragLockEngine engine{clock, device};
    QList<QPair<bool, QPointF>> changes;
    Fixture()
    {
        engine.setTimeoutMs(10000);
        engine.setHoldingCallback([this](bool h, const QPointF &p) { changes.append({h, p}); });
    }
    void hold(const QPointF &at = QPointF(100, 100))
    {
        engine.onInputEvent(move(at.x(), at.y()));
        QVERIFY(engine.toggle(at).ok);
        QVERIFY(engine.isHolding());
    }
};

} // namespace

class TestDragLock : public QObject {
    Q_OBJECT
private slots:
    // ---------------------------------------------------------------- press / hotkey
    void hotkeyPressesAndHolds()
    {
        Fixture f;
        QCOMPARE(f.engine.state(), DragLockEngine::State::Idle);
        f.hold(QPointF(50, 60));
        // The input layer is prepared (consumption, AutoClick) BEFORE the button goes down.
        QCOMPARE(f.device.calls, (QStringList{QStringLiteral("hold"), QStringLiteral("down")}));
        QCOMPARE(f.changes.size(), 1);
        QCOMPARE(f.changes[0].first, true);
        QCOMPARE(f.changes[0].second, QPointF(50, 60));
    }

    void hotkeyAgainReleasesAtTheCurrentPosition()
    {
        Fixture f;
        f.hold();
        f.engine.onInputEvent(move(180, 140));
        QVERIFY(f.engine.toggle(QPointF()).ok);
        QVERIFY(!f.engine.isHolding());
        QCOMPARE(f.engine.lastReleaseReason(), Reason::Hotkey);
        QCOMPARE(f.device.calls.mid(2), (QStringList{QStringLiteral("up"), QStringLiteral("unhold")}));
        QCOMPARE(f.changes.last().first, false);
        QCOMPARE(f.changes.last().second, QPointF(180, 140));
    }

    void toggleUsesThePointerWhenNoEventWasSeen()
    {
        Fixture f;
        QVERIFY(f.engine.toggle(QPointF(7, 8)).ok);
        QCOMPARE(f.engine.position(), QPointF(7, 8));
    }

    void failedPressLeavesIdle()
    {
        Fixture f;
        f.device.failPress = true;
        QVERIFY(!f.engine.toggle(QPointF(1, 1)).ok);
        QVERIFY(!f.engine.isHolding());
        QVERIFY(!f.device.holdingFlag);
        QCOMPARE(f.changes.last().first, false); // AutoClick is re-armed again
        QVERIFY(!f.engine.release(Reason::AppExit)); // nothing to release
    }

    void onlyOneDragLockAtATime()
    {
        Fixture f;
        f.hold();
        f.engine.toggle(QPointF()); // second press releases instead of pressing again
        QCOMPARE(f.device.calls.count(QStringLiteral("down")), 1);
        QCOMPARE(f.device.calls.count(QStringLiteral("up")), 1);
    }

    // ---------------------------------------------------------------- timeout (D-02)
    void timeoutSinceTheLastMovement()
    {
        Fixture f;
        f.hold();
        f.clock.advanceMs(9000);
        QVERIFY(!f.engine.poll());
        f.engine.onInputEvent(move(101, 100)); // movement restarts the timeout
        QCOMPARE(f.engine.nextDeadlineMs(), f.clock.monotonicMs() + 10000);
        f.clock.advanceMs(9999);
        QVERIFY(!f.engine.poll());
        QVERIFY(f.engine.isHolding());
        f.clock.advanceMs(1);
        QVERIFY(f.engine.poll());
        QCOMPARE(f.engine.lastReleaseReason(), Reason::Timeout);
        QCOMPARE(f.changes.last().second, QPointF(101, 100));
    }

    void sameMovePositionIsNoMovement()
    {
        Fixture f;
        f.hold(QPointF(100, 100));
        f.clock.advanceMs(9000);
        f.engine.onInputEvent(move(100, 100)); // no actual movement
        f.clock.advanceMs(1000);
        QVERIFY(f.engine.poll());
    }

    void timeoutChangeAppliesImmediately()
    {
        Fixture f;
        f.hold();
        f.clock.advanceMs(3000);
        f.engine.setTimeoutMs(2000);
        QVERIFY(f.engine.poll());
    }

    void wheelDoesNotReleaseNorResetTheTimeout()
    {
        Fixture f;
        f.hold();
        InputEvent wheel;
        wheel.type = InputEventType::Wheel;
        wheel.wheelDeltaY = 120;
        f.clock.advanceMs(5000);
        f.engine.onInputEvent(wheel);
        QVERIFY(f.engine.isHolding());
        f.clock.advanceMs(5000);
        QVERIFY(f.engine.poll());
    }

    // ---------------------------------------------------------------- keys (D-01)
    void nonModifierKeyReleases()
    {
        Fixture f;
        f.hold();
        f.engine.onInputEvent(key(Qt::Key_A));
        QVERIFY(!f.engine.isHolding());
        QCOMPARE(f.engine.lastReleaseReason(), Reason::KeyPressed);
    }

    void modifiersAloneDoNotRelease()
    {
        Fixture f;
        f.hold();
        for (int k : {int(Qt::Key_Shift), int(Qt::Key_Control), int(Qt::Key_Alt), int(Qt::Key_Meta)}) {
            f.engine.onInputEvent(key(k));
            f.engine.onInputEvent(key(k, false, false, InputEventType::KeyUp));
        }
        QVERIFY(f.engine.isHolding());
    }

    void swallowedHotkeysAndRepeatsAreLeftToTheirAction()
    {
        Fixture f;
        f.hold();
        f.engine.onInputEvent(key(Qt::Key_F8, true));        // the drag-lock hotkey itself (swallowed)
        f.engine.onInputEvent(key(Qt::Key_F8, true, true));  // and its auto-repeat
        f.engine.onInputEvent(key(Qt::Key_A, false, true));  // repeat of a key held before
        f.engine.onInputEvent(key(Qt::Key_A, false, false, InputEventType::KeyUp));
        QVERIFY(f.engine.isHolding());
    }

    void syntheticEventsAreIgnored()
    {
        Fixture f;
        f.hold();
        f.engine.onInputEvent(button(InputEventType::MouseDown, MouseButtonId::Left, 0, 0, true)); // our own press
        InputEvent k = key(Qt::Key_A);
        k.synthetic = true;
        f.engine.onInputEvent(k);
        QVERIFY(f.engine.isHolding());
    }

    // ---------------------------------------------------------------- mouse click (D-03)
    void anyMouseButtonReleases()
    {
        for (MouseButtonId b : {MouseButtonId::Left, MouseButtonId::Right, MouseButtonId::Middle, MouseButtonId::Back}) {
            Fixture f;
            f.hold();
            f.engine.onInputEvent(button(InputEventType::MouseDown, b, 220, 230));
            QVERIFY(!f.engine.isHolding());
            QCOMPARE(f.engine.lastReleaseReason(), Reason::MouseClick);
            QCOMPARE(f.changes.last().second, QPointF(220, 230));
            f.engine.onInputEvent(button(InputEventType::MouseUp, b, 220, 230));
            QCOMPARE(f.device.calls.count(QStringLiteral("up")), 1); // the click only released the drag
        }
    }

    void clickIsConsumedByTheInputLayer()
    {
        KeySuppressionRegistry r;
        const InputEvent down = button(InputEventType::MouseDown, MouseButtonId::Right);
        const InputEvent up = button(InputEventType::MouseUp, MouseButtonId::Right);
        QVERIFY(!r.shouldSwallowMouse(down)); // idle: clicks pass
        QVERIFY(!r.shouldSwallowMouse(up));

        r.setDragLockHolding(true);
        QVERIFY(r.isDragLockHolding());
        QVERIFY(!r.shouldSwallowMouse(button(InputEventType::MouseDown, MouseButtonId::Left, 0, 0, true))); // ours
        QVERIFY(r.shouldSwallowMouse(down)); // the releasing press ...
        r.setDragLockHolding(false);         // (the drag lock releases meanwhile)
        QVERIFY(r.shouldSwallowMouse(up));   // ... and its release are swallowed
        QVERIFY(!r.shouldSwallowMouse(down)); // only one click is consumed
        QVERIFY(!r.shouldSwallowMouse(up));
        QCOMPARE(r.swallowedCount(), quint64(2));
    }

    void onlyOneClickIsConsumedEvenWhileStillHolding()
    {
        KeySuppressionRegistry r;
        r.setDragLockHolding(true);
        QVERIFY(r.shouldSwallowMouse(button(InputEventType::MouseDown, MouseButtonId::Left)));
        QVERIFY(!r.shouldSwallowMouse(button(InputEventType::MouseDown, MouseButtonId::Right)));
        QVERIFY(r.shouldSwallowMouse(button(InputEventType::MouseUp, MouseButtonId::Left)));
        QVERIFY(!r.shouldSwallowMouse(button(InputEventType::MouseUp, MouseButtonId::Right)));
    }

    // ---------------------------------------------------------------- other release reasons
    void applicationReleases_data()
    {
        QTest::addColumn<int>("reason");
        QTest::newRow("break window") << int(Reason::BreakWindow);
        QTest::newRow("app exit") << int(Reason::AppExit);
        QTest::newRow("key control off") << int(Reason::KeyControlOff);
        QTest::newRow("binding changed") << int(Reason::BindingChanged);
        QTest::newRow("input lost") << int(Reason::InputLost);
        QTest::newRow("autoclick off") << int(Reason::AutoClickOff);
    }

    void applicationReleases()
    {
        QFETCH(int, reason);
        Fixture f;
        f.hold();
        QVERIFY(f.engine.release(Reason(reason)));
        QVERIFY(!f.engine.isHolding());
        QVERIFY(!f.device.holdingFlag);
        QCOMPARE(int(f.engine.lastReleaseReason()), reason);
        QVERIFY(!f.engine.release(Reason(reason))); // releasing twice sends one button-up
        QCOMPARE(f.device.calls.count(QStringLiteral("up")), 1);
        QVERIFY(!DragLockEngine::reasonText(Reason(reason)).isEmpty());
    }

    void failedReleaseIsReported()
    {
        Fixture f;
        f.hold();
        f.device.failRelease = true;
        QVERIFY(f.engine.release(Reason::AppExit));
        QVERIFY(!f.engine.lastReleaseResult().ok);
        QVERIFY(!f.device.holdingFlag); // input layer back to normal anyway
    }

    void nothingHappensWhileIdle()
    {
        Fixture f;
        f.engine.onInputEvent(key(Qt::Key_A));
        f.engine.onInputEvent(button(InputEventType::MouseDown, MouseButtonId::Left));
        f.clock.advanceMs(60000);
        QVERIFY(!f.engine.poll());
        QCOMPARE(f.engine.nextDeadlineMs(), qint64(-1));
        QVERIFY(f.device.calls.isEmpty());
        QVERIFY(f.changes.isEmpty());
    }

    // ---------------------------------------------------------------- AutoClick (B.4 rule 7, D-04)
    void autoClickDoesNotClickWhileHoldingNorAfterReleaseUntilTheDistanceIsMoved()
    {
        Fixture f;
        AutoClickEngine autoclick(f.clock);
        AutoClickEngine::Config config;
        config.triggerMs = 200;
        config.distancePx = 15;
        config.jitterPx = 3;
        autoclick.setConfig(config);
        autoclick.setEnabled(true);
        f.engine.setHoldingCallback(
            [&](bool holding, const QPointF &p) { autoclick.setDragLockActive(holding, p); });

        const auto both = [&](const InputEvent &e) {
            autoclick.onInputEvent(e);
            f.engine.onInputEvent(e);
        };
        both(move(100, 100));
        f.engine.toggle(QPointF(100, 100));
        // Drag far and stop: no auto-click while holding.
        both(move(300, 100));
        f.clock.advanceMs(1000);
        QVERIFY(!autoclick.poll());
        QCOMPARE(autoclick.suppressReason(), AutoClickEngine::SuppressReason::DragLock);

        // Release at (300,100): reference point = release position, nothing clicks.
        f.engine.release(Reason::Hotkey);
        f.clock.advanceMs(1000);
        QVERIFY(!autoclick.poll());
        both(move(310, 100)); // 10 px < 15 px: still disarmed
        f.clock.advanceMs(1000);
        QVERIFY(!autoclick.poll());
        both(move(316, 100)); // 16 px from the release position: armed
        f.clock.advanceMs(200);
        QVERIFY(autoclick.poll());
    }

    void autoClickReferenceAfterAConsumedClickRelease()
    {
        Fixture f;
        AutoClickEngine autoclick(f.clock);
        AutoClickEngine::Config config;
        config.triggerMs = 200;
        config.distancePx = 15;
        autoclick.setConfig(config);
        autoclick.setEnabled(true);
        f.engine.setHoldingCallback(
            [&](bool holding, const QPointF &p) { autoclick.setDragLockActive(holding, p); });
        const auto both = [&](const InputEvent &e) {
            autoclick.onInputEvent(e);
            f.engine.onInputEvent(e);
        };
        both(move(0, 0));
        f.engine.toggle(QPointF(0, 0));
        both(move(200, 0));
        both(button(InputEventType::MouseDown, MouseButtonId::Left, 200, 0)); // consumed click releases
        both(button(InputEventType::MouseUp, MouseButtonId::Left, 200, 0));
        QVERIFY(!f.engine.isHolding());
        f.clock.advanceMs(1000);
        QVERIFY(!autoclick.poll());
        both(move(220, 0));
        f.clock.advanceMs(200);
        QVERIFY(autoclick.poll());
    }
};

QTEST_GUILESS_MAIN(TestDragLock)
#include "tst_draglock.moc"
