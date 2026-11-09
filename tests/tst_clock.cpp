#include "core/Clock.h"

#include <QTest>

using namespace ergo;

class TestClock : public QObject {
    Q_OBJECT
private slots:
    void fakeClockStartsAtGivenTime()
    {
        const QDateTime start(QDate(2026, 11, 9), QTime(14, 33));
        FakeClock clock(start);
        QCOMPARE(clock.now(), start);
        QCOMPARE(clock.monotonicMs(), 0);
    }

    void advanceMovesWallAndMonotonicTime()
    {
        FakeClock clock(QDateTime(QDate(2026, 11, 9), QTime(23, 59, 59)));
        clock.advanceMs(1500);
        QCOMPARE(clock.monotonicMs(), 1500);
        QCOMPARE(clock.now(), QDateTime(QDate(2026, 11, 10), QTime(0, 0, 0, 500)));
    }

    void setNowDoesNotChangeMonotonicTime()
    {
        FakeClock clock;
        clock.advanceMs(100);
        clock.setNow(QDateTime(QDate(2030, 1, 1), QTime(8, 0)));
        QCOMPARE(clock.monotonicMs(), 100);
        QCOMPARE(clock.now().date(), QDate(2030, 1, 1));
    }

    void systemClockIsMonotonic()
    {
        SystemClock clock;
        const qint64 a = clock.monotonicMs();
        const qint64 b = clock.monotonicMs();
        QVERIFY(b >= a);
        QVERIFY(clock.now().isValid());
    }
};

QTEST_GUILESS_MAIN(TestClock)
#include "tst_clock.moc"
