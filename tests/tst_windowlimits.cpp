#include "core/WindowLimits.h"

#include <QTest>

using namespace ergo;

// Main window size limits (B.1.1, 1.7.8): never larger than 80 % of the
// screen's usable area, minimum 720 x 560 unless the screen is too small.
class TestWindowLimits : public QObject {
    Q_OBJECT
private slots:
    void fullHdScreen()
    {
        const WindowLimits l = windowLimitsFor(QSize(1920, 1040));
        QCOMPARE(l.maximum, QSize(1536, 832));
        QCOMPARE(l.minimum, QSize(720, 560));
        QCOMPARE(l.initial, QSize(820, 620));
    }

    void smallScreenShrinksTheMinimumAndTheStartSize()
    {
        const WindowLimits l = windowLimitsFor(QSize(1024, 700)); // e.g. 1024 x 768 with a task bar
        QCOMPARE(l.maximum, QSize(819, 560));
        QCOMPARE(l.minimum, QSize(720, 560));
        QCOMPARE(l.initial, QSize(819, 560));

        const WindowLimits tiny = windowLimitsFor(QSize(800, 600));
        QCOMPARE(tiny.maximum, QSize(640, 480));
        QCOMPARE(tiny.minimum, QSize(640, 480)); // never above the maximum
        QCOMPARE(tiny.initial, QSize(640, 480));
    }

    void neverMoreThanEightyPercent_data()
    {
        QTest::addColumn<QSize>("available");
        QTest::newRow("macbook") << QSize(1512, 945);
        QTest::newRow("4k") << QSize(3840, 2100);
        QTest::newRow("portrait") << QSize(1080, 1880);
        QTest::newRow("odd") << QSize(1367, 767);
    }

    void neverMoreThanEightyPercent()
    {
        QFETCH(QSize, available);
        const WindowLimits l = windowLimitsFor(available);
        QVERIFY(l.maximum.width() <= available.width() * 0.8);
        QVERIFY(l.maximum.height() <= available.height() * 0.8);
        QVERIFY(l.minimum.width() <= l.maximum.width());
        QVERIFY(l.minimum.height() <= l.maximum.height());
        QVERIFY(l.initial.width() >= l.minimum.width() && l.initial.width() <= l.maximum.width());
        QVERIFY(l.initial.height() >= l.minimum.height() && l.initial.height() <= l.maximum.height());
    }

    void boundedKeepsASizeInsideTheLimits()
    {
        const WindowLimits l = windowLimitsFor(QSize(1920, 1040));
        QCOMPARE(l.bounded(QSize(2000, 300)), QSize(1536, 560));
        QCOMPARE(l.bounded(QSize(900, 700)), QSize(900, 700));
    }

    void invalidScreenUsesTheDefaults()
    {
        const WindowLimits l = windowLimitsFor(QSize());
        QCOMPARE(l.minimum, QSize(720, 560));
        QCOMPARE(l.initial, QSize(820, 620));
        QVERIFY(l.maximum.width() >= l.initial.width());
    }
};

QTEST_GUILESS_MAIN(TestWindowLimits)
#include "tst_windowlimits.moc"
