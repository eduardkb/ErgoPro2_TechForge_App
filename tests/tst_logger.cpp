#include "core/Clock.h"
#include "core/LogCategories.h"
#include "core/Logger.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

using namespace ergo;

namespace {

QString readAll(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(f.readAll());
}

void touch(const QString &path)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("old\n");
}

} // namespace

class TestLogger : public QObject {
    Q_OBJECT
private slots:
    void selectionMatrix_data()
    {
        QTest::addColumn<int>("selection");
        QTest::addColumn<bool>("debug");
        QTest::addColumn<bool>("info");
        QTest::addColumn<bool>("warning");
        QTest::addColumn<bool>("error");
        QTest::addColumn<bool>("critical");

        QTest::newRow("info") << int(LogLevelSelection::InfoOnly) << false << true << false << false << false;
        QTest::newRow("info+warn") << int(LogLevelSelection::InfoWarning) << false << true << true << false << false;
        QTest::newRow("info+warn+err") << int(LogLevelSelection::InfoWarningError) << false << true << true << true
                                       << false;
        QTest::newRow("+critical") << int(LogLevelSelection::InfoWarningErrorCritical) << false << true << true
                                   << true << true;
        QTest::newRow("all") << int(LogLevelSelection::All) << true << true << true << true << true;
    }

    void selectionMatrix()
    {
        QFETCH(int, selection);
        const auto s = static_cast<LogLevelSelection>(selection);
        QTEST(isLevelEnabled(s, LogLevel::Debug), "debug");
        QTEST(isLevelEnabled(s, LogLevel::Info), "info");
        QTEST(isLevelEnabled(s, LogLevel::Warning), "warning");
        QTEST(isLevelEnabled(s, LogLevel::Error), "error");
        QTEST(isLevelEnabled(s, LogLevel::Critical), "critical");
    }

    void selectionIdsRoundTrip()
    {
        for (const QString &id : allLogLevelSelectionIds()) {
            const auto s = logLevelSelectionFromId(id);
            QVERIFY(s.has_value());
            QCOMPARE(logLevelSelectionId(*s), id);
        }
        QVERIFY(!logLevelSelectionFromId(QStringLiteral("nonsense")).has_value());
        QCOMPARE(logLevelSelectionId(kDefaultLogLevelSelection), QStringLiteral("info_warning_error_critical"));
    }

    void writesFormattedLinesAndFilters()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        FakeClock clock(QDateTime(QDate(2026, 11, 9), QTime(14, 33, 1, 123)));
        Logger logger(clock);
        Logger::Config config;
        config.directory = dir.path();
        config.selection = LogLevelSelection::InfoWarning;
        QVERIFY(logger.open(config));

        QThread::currentThread()->setObjectName(QStringLiteral("main"));
        logger.write(LogLevel::Info, u"test", u"hello");
        logger.write(LogLevel::Debug, u"test", u"hidden-debug");
        logger.write(LogLevel::Error, u"test", u"hidden-error");
        logger.write(LogLevel::Warning, u"test", u"careful");

        const QString content = readAll(QDir(dir.path()).filePath(QStringLiteral("ergopro2-2026-11-09.log")));
        QVERIFY(content.contains(QStringLiteral("2026-11-09 14:33:01.123 [INFO] [main] [test] hello")));
        QVERIFY(content.contains(QStringLiteral("[WARNING] [main] [test] careful")));
        QVERIFY(!content.contains(QStringLiteral("hidden-debug")));
        QVERIFY(!content.contains(QStringLiteral("hidden-error")));

        logger.setSelection(LogLevelSelection::All);
        logger.write(LogLevel::Debug, u"test", u"now-visible");
        QVERIFY(readAll(logger.currentFilePath()).contains(QStringLiteral("[DEBUG]")));
    }

    void qtMessagesAreRouted()
    {
        QTemporaryDir dir;
        FakeClock clock;
        Logger logger(clock);
        Logger::Config config;
        config.directory = dir.path();
        config.selection = LogLevelSelection::All;
        QVERIFY(logger.open(config));
        Logger::installQtMessageHandler(&logger);
        qCWarning(lcSettings) << "routed warning";
        qCCritical(lcSettings) << "routed error";
        logCritical(u"app", QStringLiteral("explicit critical"));
        Logger::installQtMessageHandler(nullptr);

        const QString content = readAll(logger.currentFilePath());
        QVERIFY2(content.contains(QStringLiteral("[WARNING] [main] [settings] routed warning"))
                     || content.contains(QStringLiteral("[settings] routed warning")),
                 qPrintable(content));
        QVERIFY(content.contains(QStringLiteral("[ERROR]")));
        QVERIFY(content.contains(QStringLiteral("[CRITICAL]")));
        QVERIFY(content.contains(QStringLiteral("explicit critical")));
    }

    void rollsOverWhenFileTooLarge()
    {
        QTemporaryDir dir;
        FakeClock clock(QDateTime(QDate(2026, 11, 9), QTime(10, 0)));
        Logger logger(clock);
        Logger::Config config;
        config.directory = dir.path();
        config.maxFileSizeBytes = 200;
        QVERIFY(logger.open(config));

        for (int i = 0; i < 10; ++i)
            logger.write(LogLevel::Info, u"test", QStringLiteral("line %1 ").arg(i) + QString(40, QLatin1Char('x')));

        const QDir d(dir.path());
        QVERIFY(d.exists(QStringLiteral("ergopro2-2026-11-09.log")));
        QVERIFY(d.exists(QStringLiteral("ergopro2-2026-11-09-1.log")));
        QVERIFY(QFileInfo(d.filePath(QStringLiteral("ergopro2-2026-11-09.log"))).size() <= 200);
    }

    void switchesFileOnNewDay()
    {
        QTemporaryDir dir;
        FakeClock clock(QDateTime(QDate(2026, 11, 9), QTime(23, 59, 59)));
        Logger logger(clock);
        Logger::Config config;
        config.directory = dir.path();
        QVERIFY(logger.open(config));
        logger.write(LogLevel::Info, u"test", u"before midnight");
        clock.advanceMs(2000);
        logger.write(LogLevel::Info, u"test", u"after midnight");
        QVERIFY(readAll(QDir(dir.path()).filePath(QStringLiteral("ergopro2-2026-11-10.log")))
                    .contains(QStringLiteral("after midnight")));
    }

    void retentionCleanupRemovesOldFiles()
    {
        QTemporaryDir dir;
        const QDir d(dir.path());
        touch(d.filePath(QStringLiteral("ergopro2-2026-11-09.log")));   // today
        touch(d.filePath(QStringLiteral("ergopro2-2026-11-07.log")));   // 2 days ago -> kept (3 days)
        touch(d.filePath(QStringLiteral("ergopro2-2026-11-06.log")));   // 3 days ago -> removed
        touch(d.filePath(QStringLiteral("ergopro2-2026-11-06-2.log"))); // roll-over file -> removed
        touch(d.filePath(QStringLiteral("ergopro2-2025-01-01.log")));   // very old -> removed
        touch(d.filePath(QStringLiteral("unrelated.txt")));             // never touched

        FakeClock clock(QDateTime(QDate(2026, 11, 9), QTime(9, 0)));
        Logger logger(clock);
        Logger::Config config;
        config.directory = dir.path();
        config.retentionDays = 3;
        QVERIFY(logger.open(config));
        // open() must not delete anything: the real retention setting is applied later.
        QVERIFY(d.exists(QStringLiteral("ergopro2-2025-01-01.log")));
        QCOMPARE(logger.cleanupOldFiles(), 3);

        QVERIFY(d.exists(QStringLiteral("ergopro2-2026-11-09.log")));
        QVERIFY(d.exists(QStringLiteral("ergopro2-2026-11-07.log")));
        QVERIFY(!d.exists(QStringLiteral("ergopro2-2026-11-06.log")));
        QVERIFY(!d.exists(QStringLiteral("ergopro2-2026-11-06-2.log")));
        QVERIFY(!d.exists(QStringLiteral("ergopro2-2025-01-01.log")));
        QVERIFY(d.exists(QStringLiteral("unrelated.txt")));

        // Shorter retention applied later.
        logger.setRetentionDays(1);
        QCOMPARE(logger.cleanupOldFiles(), 1);
        QVERIFY(!d.exists(QStringLiteral("ergopro2-2026-11-07.log")));
        QVERIFY(d.exists(QStringLiteral("ergopro2-2026-11-09.log")));
    }

    void retentionIsClamped()
    {
        FakeClock clock;
        Logger logger(clock);
        logger.setRetentionDays(0);
        QCOMPARE(logger.retentionDays(), Logger::kMinRetentionDays);
        logger.setRetentionDays(999);
        QCOMPARE(logger.retentionDays(), Logger::kMaxRetentionDays);
    }
};

QTEST_GUILESS_MAIN(TestLogger)
#include "tst_logger.moc"
