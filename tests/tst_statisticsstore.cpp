#include "core/statistics/StatisticsStore.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace ergo;

namespace {

MinuteRecord record(int minute, int keys = 1)
{
    MinuteRecord r;
    r.minute = minute;
    r.clicksLeft = 3;
    r.clicksRight = 1;
    r.clicksOther = 0;
    r.clicksForYou = 2;
    r.keyPresses = keys;
    r.wheel = 4;
    r.distancePx = 1234.4;
    r.distanceCm = 32.66;
    r.mouseSeconds = 40;
    r.keyboardSeconds = 20;
    r.activeSeconds = 50;
    r.usagePercent = 104.25;
    return r;
}

QByteArray readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll();
}

void writeFile(const QString &path, const QByteArray &data)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

const QDate kDay(2026, 3, 10);

} // namespace

class TestStatisticsStore : public QObject {
    Q_OBJECT
private slots:
    void fileNames()
    {
        QCOMPARE(StatisticsStore::fileNameFor(kDay), QStringLiteral("ergopro2-stats-2026-03-10.csv"));
        QCOMPARE(StatisticsStore::dateFromFileName(QStringLiteral("ergopro2-stats-2026-03-10.csv")), kDay);
        QVERIFY(!StatisticsStore::dateFromFileName(QStringLiteral("ergopro2-stats-2026-13-40.csv")));
        QVERIFY(!StatisticsStore::dateFromFileName(QStringLiteral("ergopro2-2026-03-10.log")));
        QVERIFY(!StatisticsStore::dateFromFileName(QStringLiteral("ergopro2-stats-2026-03-10.csv.bak")));
    }

    void rowFormatAndRoundTrip()
    {
        MinuteRecord r = record(9 * 60 + 5);
        r.events << QStringLiteral("break_shown") << QStringLiteral("break_postponed");
        const QString row = StatisticsStore::formatRow(r);
        QCOMPARE(row, QStringLiteral("09:05,3,1,0,2,1,4,1234,32.7,40,20,50,104.3,break_shown;break_postponed"));
        MinuteRecord back;
        QString error;
        QVERIFY2(StatisticsStore::parseRow(row, &back, &error), qPrintable(error));
        QCOMPARE(back.minute, r.minute);
        QCOMPARE(back.clicksForYou, 2);
        QCOMPARE(back.distanceCm, 32.7);
        QCOMPARE(back.usagePercent, 104.3);
        QCOMPARE(back.events, r.events);
    }

    void writeHeaderOnceAndReadBack()
    {
        QTemporaryDir dir;
        StatisticsStore store(dir.filePath(QStringLiteral("statistics"))); // created when needed
        QString error;
        QVERIFY(store.append(kDay, {record(540), record(541)}, &error));
        QVERIFY(store.append(kDay, {record(542)}, &error)); // continue after a restart
        const QList<QByteArray> lines = readFile(store.filePath(kDay)).split('\n');
        QCOMPARE(lines.first(), StatisticsStore::header().toUtf8());
        QCOMPARE(lines.count(StatisticsStore::header().toUtf8()), 1);
        QStringList problems;
        const DayStats day = store.load(kDay, &problems);
        QVERIFY(problems.isEmpty());
        QCOMPARE(day.date, kDay);
        QCOMPARE(day.minutes.size(), 3);
        QCOMPARE(day.minutes[2].minute, 542);
    }

    void sameMinuteTwiceIsMerged()
    {
        QTemporaryDir dir;
        StatisticsStore store(dir.path());
        QString error;
        store.append(kDay, {record(600, 5)}, &error);
        store.append(kDay, {record(600, 7)}, &error); // flushed on exit, restarted in the same minute
        const DayStats day = store.load(kDay);
        QCOMPARE(day.minutes.size(), 1);
        QCOMPARE(day.minutes[0].keyPresses, 12);
    }

    void partialLastLineIsRepaired()
    {
        QTemporaryDir dir;
        StatisticsStore store(dir.path());
        QString error;
        store.append(kDay, {record(540)}, &error);
        QFile f(store.filePath(kDay));
        QVERIFY(f.open(QIODevice::Append));
        f.write("09:01,1,2,"); // crash while writing
        f.close();
        QStringList problems;
        const DayStats day = store.load(kDay, &problems);
        QCOMPARE(day.minutes.size(), 1);
        QCOMPARE(problems.size(), 1);
        QVERIFY(problems[0].contains(QStringLiteral("incomplete")));
        QVERIFY(readFile(store.filePath(kDay)).endsWith('\n'));
        // Appending after the repair produces valid rows.
        QVERIFY(store.append(kDay, {record(541)}, &error));
        problems.clear();
        QCOMPARE(store.load(kDay, &problems).minutes.size(), 2);
        QVERIFY(problems.isEmpty());
    }

    void partialLineIsRepairedBeforeAppending()
    {
        QTemporaryDir dir;
        StatisticsStore store(dir.path());
        QString error;
        store.append(kDay, {record(540)}, &error);
        QFile f(store.filePath(kDay));
        QVERIFY(f.open(QIODevice::Append));
        f.write("09:01,1");
        f.close();
        QVERIFY(store.append(kDay, {record(541)}, &error));
        QStringList problems;
        QCOMPARE(store.load(kDay, &problems).minutes.size(), 2);
        QVERIFY(problems.isEmpty());
    }

    void invalidLinesAreSkipped()
    {
        QTemporaryDir dir;
        StatisticsStore store(dir.path());
        const QByteArray header = StatisticsStore::header().toUtf8();
        writeFile(store.filePath(kDay), header + "\n"
                                                 "09:00,1,0,0,0,1,0,10,0.3,1,1,1,0.3,\n"
                                                 "garbage\n"
                                                 "25:00,1,0,0,0,1,0,10,0.3,1,1,1,0.3,\n"
                                                 "09:02,x,0,0,0,1,0,10,0.3,1,1,1,0.3,\n"
                                                 "09:03,1,0,0,0,1,0,10,0.3,1,1,1,0.3,coffee\n"
                                                 "09:04,1,0,0,0,1,0,10,0.3,61,1,1,0.3,\n"
                                                 "\n"
                                                 "09:05,2,0,0,0,1,0,10,0.3,1,1,1,0.3,break_completed\n");
        QStringList problems;
        const DayStats day = store.load(kDay, &problems);
        QCOMPARE(day.minutes.size(), 2);
        QCOMPARE(problems.size(), 5);
        QVERIFY(problems[0].contains(QStringLiteral("line 3")));
        QCOMPARE(day.minutes[1].events, QStringList{QStringLiteral("break_completed")});
    }

    void missingDayIsEmpty()
    {
        QTemporaryDir dir;
        StatisticsStore store(dir.path());
        const DayStats day = store.load(kDay);
        QVERIFY(day.isEmpty());
        QCOMPARE(day.date, kDay);
    }

    void navigationSkipsDaysWithoutAFile()
    {
        QTemporaryDir dir;
        StatisticsStore store(dir.path());
        QString error;
        for (const QDate &d : {QDate(2026, 3, 1), QDate(2026, 3, 4), QDate(2026, 3, 9)})
            store.append(d, {record(600)}, &error);
        writeFile(dir.filePath(QStringLiteral("notes.txt")), "x"); // ignored
        QCOMPARE(store.days().size(), 3);
        QCOMPARE(store.previousDay(kDay), QDate(2026, 3, 9));
        QCOMPARE(store.previousDay(QDate(2026, 3, 9)), QDate(2026, 3, 4));
        QVERIFY(!store.previousDay(QDate(2026, 3, 1)));
        QCOMPARE(store.nextDay(QDate(2026, 3, 1)), QDate(2026, 3, 4));
        QVERIFY(!store.nextDay(QDate(2026, 3, 9)));
    }

    void retentionCleanup()
    {
        QTemporaryDir dir;
        StatisticsStore store(dir.path());
        QString error;
        for (int i = 0; i < 6; ++i)
            store.append(kDay.addDays(-i), {record(600)}, &error); // 10th .. 5th
        QStringList failed;
        const QStringList removed = store.cleanup(3, kDay, &failed); // keeps 8th, 9th, 10th
        QCOMPARE(removed.size(), 3);
        QVERIFY(failed.isEmpty());
        QCOMPARE(store.days(), (QList<QDate>{QDate(2026, 3, 8), QDate(2026, 3, 9), kDay}));
        QCOMPARE(store.cleanup(1, kDay).size(), 2); // only today
        QCOMPARE(store.days(), QList<QDate>{kDay});
        QVERIFY(store.cleanup(90, kDay).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestStatisticsStore)
#include "tst_statisticsstore.moc"
