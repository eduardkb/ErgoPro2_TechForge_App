#include "core/input/InputDispatcher.h"

#include <QSignalSpy>
#include <QTest>

#include <thread>

using namespace ergo;

class TestDispatcher : public QObject {
    Q_OBJECT
private slots:
    void deliversInOrderFromAnotherThread()
    {
        InputDispatcher dispatcher;
        QList<qint64> received;
        connect(&dispatcher, &InputDispatcher::inputEvent, this,
                [&received](const InputEvent &e) { received.append(e.timestampMs); });

        std::thread hook([&dispatcher] {
            for (int i = 0; i < 1000; ++i) {
                InputEvent e;
                e.timestampMs = i;
                dispatcher.post(e);
            }
        });
        hook.join();
        QTRY_COMPARE_WITH_TIMEOUT(received.size(), 1000, 5000);
        for (int i = 0; i < 1000; ++i)
            QCOMPARE(received.at(i), qint64(i));
        QCOMPARE(dispatcher.deliveredCount(), quint64(1000));
        QCOMPARE(dispatcher.droppedCount(), quint64(0));
    }

    void deliveryIsAsynchronous()
    {
        InputDispatcher dispatcher;
        QSignalSpy spy(&dispatcher, &InputDispatcher::inputEvent);
        dispatcher.post(InputEvent{});
        QCOMPARE(spy.count(), 0); // never emitted inside post()
        QTRY_COMPARE(spy.count(), 1);
    }

    void dropsWhenTheReceiverCannotKeepUp()
    {
        InputDispatcher dispatcher;
        QSignalSpy spy(&dispatcher, &InputDispatcher::inputEvent);
        for (int i = 0; i < InputDispatcher::kMaxPending + 50; ++i)
            dispatcher.post(InputEvent{});
        QCOMPARE(dispatcher.droppedCount(), quint64(50));
        QTRY_COMPARE(spy.count(), InputDispatcher::kMaxPending);
        // After draining, events are accepted again.
        dispatcher.post(InputEvent{});
        QTRY_COMPARE(spy.count(), InputDispatcher::kMaxPending + 1);
    }

    void eventFieldsSurviveTheQueue()
    {
        InputDispatcher dispatcher;
        InputEvent received;
        connect(&dispatcher, &InputDispatcher::inputEvent, this, [&received](const InputEvent &e) { received = e; });
        InputEvent e;
        e.type = InputEventType::KeyDown;
        e.key = Qt::Key_F9;
        e.modifiers = Modifier::Control;
        e.synthetic = true;
        e.swallowed = true;
        e.position = QPointF(3, 4);
        dispatcher.post(e);
        QTRY_COMPARE(received.key, int(Qt::Key_F9));
        QCOMPARE(received.type, InputEventType::KeyDown);
        QCOMPARE(received.modifiers, Modifiers(Modifier::Control));
        QVERIFY(received.synthetic);
        QVERIFY(received.swallowed);
        QCOMPARE(received.position, QPointF(3, 4));
    }
};

QTEST_GUILESS_MAIN(TestDispatcher)
#include "tst_dispatcher.moc"
