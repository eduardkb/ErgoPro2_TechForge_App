#include "core/LogProblems.h"
#include "core/Logger.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace ergo;

namespace {

void writeFile(const QString &path, const QByteArray &text)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(text);
}

const QDate kToday(2026, 11, 20);

} // namespace

// 2.2.0 (owner request): the General tab card with the WARNING / ERROR /
// CRITICAL entries of the last days' log files.
class TestLogProblems : public QObject {
    Q_OBJECT

private slots:
    void onlyFilesOfTheLastDaysAreSearched()
    {
        const QStringList files = {QStringLiteral("ergopro2-2026-11-20.log"), QStringLiteral("ergopro2-2026-11-20-1.log"),
                                   QStringLiteral("ergopro2-2026-11-14.log"), QStringLiteral("ergopro2-2026-11-13.log"),
                                   QStringLiteral("ergopro2-2026-11-21.log"), // future date (clock change): included
                                   QStringLiteral("notes.log"), QStringLiteral("ergopro2-bad.log")};
        // 7 days = today and the 6 days before it (14 Nov – 20 Nov).
        const QStringList selected = logFilesForDays(files, kToday, 7);
        QCOMPARE(selected,
                 (QStringList{QStringLiteral("ergopro2-2026-11-20.log"), QStringLiteral("ergopro2-2026-11-20-1.log"),
                              QStringLiteral("ergopro2-2026-11-14.log"), QStringLiteral("ergopro2-2026-11-21.log")}));
    }

    void problemLinesAreCollectedWithTheirContinuationLines()
    {
        const QString text = QStringLiteral(
            "2026-11-20 08:00:00.000 [INFO] [main] [app] Started\n"
            "2026-11-20 08:00:01.000 [WARNING] [main] [settings] Invalid value\n"
            "2026-11-20 08:00:02.000 [DEBUG] [main] [app] detail\n"
            "2026-11-20 08:00:03.000 [ERROR] [main] [ui] Cannot open folder\n"
            "second line of the error\n"
            "2026-11-20 08:00:04.000 [INFO] [main] [app] ok\n"
            "line after an INFO entry\n"
            "2026-11-20 08:00:05.000 [CRITICAL] [hook] [input] Hook failed\n");
        QList<LogProblem> problems;
        collectLogProblems(text, &problems);
        QCOMPARE(problems.size(), 3);
        QCOMPARE(problems[0].level, LogLevel::Warning);
        QCOMPARE(problems[0].time, QDateTime(kToday, QTime(8, 0, 1)));
        QCOMPARE(problems[1].level, LogLevel::Error);
        QCOMPARE(problems[1].text, QStringLiteral("2026-11-20 08:00:03.000 [ERROR] [main] [ui] Cannot open folder\n"
                                                  "second line of the error"));
        QCOMPARE(problems[2].level, LogLevel::Critical);
    }

    void directoryScanIsNewestFirstAndLimited()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QDir d(dir.path());
        writeFile(d.filePath(Logger::fileNameFor(kToday.addDays(-1), 0)),
                  "2026-11-19 10:00:00.000 [ERROR] [main] [a] yesterday\n");
        writeFile(d.filePath(Logger::fileNameFor(kToday, 0)),
                  "2026-11-20 09:00:00.000 [WARNING] [main] [a] early\n"
                  "2026-11-20 11:00:00.000 [CRITICAL] [main] [a] late\n");
        writeFile(d.filePath(Logger::fileNameFor(kToday, 1)), "2026-11-20 10:00:00.000 [ERROR] [main] [a] middle\n");
        writeFile(d.filePath(Logger::fileNameFor(kToday.addDays(-10), 0)),
                  "2026-11-10 10:00:00.000 [ERROR] [main] [a] too old\n");

        LogProblemScan scan = scanLogProblems(dir.path(), kToday, 7, 500);
        QCOMPARE(scan.totalFound, 4);
        QCOMPARE(scan.filesSearched, 3);
        QCOMPARE(scan.problems.size(), 4);
        QVERIFY(scan.problems[0].text.endsWith(QLatin1String("late")));
        QVERIFY(scan.problems[1].text.endsWith(QLatin1String("middle")));
        QVERIFY(scan.problems[2].text.endsWith(QLatin1String("early")));
        QVERIFY(scan.problems[3].text.endsWith(QLatin1String("yesterday")));

        scan = scanLogProblems(dir.path(), kToday, 7, 2);
        QCOMPARE(scan.totalFound, 4); // "Showing the newest 2 of 4"
        QCOMPARE(scan.problems.size(), 2);
        QVERIFY(scan.problems[0].text.endsWith(QLatin1String("late")));

        QCOMPARE(scanLogProblems(d.filePath(QStringLiteral("missing")), kToday, 7, 500).totalFound, 0);
    }

    void searchRunsOnlyWhileTheTabIsShownAndAtMostOncePerInterval()
    {
        LogProblemSchedule s;
        s.setIntervalMs(5 * 60000);
        QVERIFY(!s.searchDue(0, false, false));  // tab not shown: never
        QVERIFY(s.searchDue(0, true, false));    // first time the tab is shown
        s.searchStarted(0);
        QVERIFY(!s.searchDue(1000, true, true)); // a search is running
        QVERIFY(!s.searchDue(60000, true, false));
        QVERIFY(!s.searchDue(10 * 60000, false, false)); // hidden: no refresh even when due
        QVERIFY(s.searchDue(10 * 60000, true, false));   // shown again after the interval: at once
        s.searchStarted(10 * 60000);
        QVERIFY(!s.searchDue(10 * 60000 + 4 * 60000, true, false));
        QVERIFY(s.searchDue(10 * 60000 + 5 * 60000, true, false));
        s.invalidate(); // e.g. the log level now includes warnings again
        QVERIFY(s.searchDue(10 * 60000 + 5 * 60000 + 1, true, false));
    }

    void cardOnlyForLevelsThatIncludeWarningsOrErrors()
    {
        QVERIFY(!logProblemsCardEnabled(LogLevelSelection::InfoOnly));
        QVERIFY(logProblemsCardEnabled(LogLevelSelection::InfoWarning));
        QVERIFY(logProblemsCardEnabled(LogLevelSelection::InfoWarningError));
        QVERIFY(logProblemsCardEnabled(LogLevelSelection::InfoWarningErrorCritical));
        QVERIFY(logProblemsCardEnabled(LogLevelSelection::All));
    }
};

QTEST_GUILESS_MAIN(TestLogProblems)
#include "tst_logproblems.moc"
