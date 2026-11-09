#pragma once

#include "core/Clock.h"
#include "core/LogLevel.h"

#include <QDate>
#include <QFile>
#include <QMutex>
#include <QString>
#include <QStringView>

namespace ergo {

// Thread-safe file logger (implementation_stages.md B.3).
//
// - One file per day: ergopro2-YYYY-MM-DD.log
// - Files larger than maxFileSizeBytes roll over to ...-1.log, ...-2.log, ...
// - Files older than retentionDays are deleted when the day changes and
//   whenever cleanupOldFiles() is called. open() does NOT clean up, because at
//   startup the logger is opened before the user's retention setting is known;
//   the application calls cleanupOldFiles() once the setting has been applied.
// - Line format: "2026-11-09 14:33:01.123 [LEVEL] [thread] [module] message"
//
// Qt messages (qDebug/qCWarning/...) are routed here by installQtMessageHandler().
class Logger {
public:
    static constexpr qint64 kDefaultMaxFileSize = 5 * 1024 * 1024;
    static constexpr int kMinRetentionDays = 1;
    static constexpr int kMaxRetentionDays = 180;

    struct Config {
        QString directory;
        LogLevelSelection selection = kDefaultLogLevelSelection;
        int retentionDays = 30;
        qint64 maxFileSizeBytes = kDefaultMaxFileSize;
        bool consoleOutput = false; // also echo to stderr (debug builds)
    };

    explicit Logger(const IClock &clock);
    ~Logger();

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    // Creates the directory and opens today's file. Returns false (and writes
    // the reason to stderr and lastError()) if the log file cannot be opened;
    // the logger then keeps trying on every write.
    bool open(const Config &config);
    void close();

    void setSelection(LogLevelSelection selection);
    LogLevelSelection selection() const;

    void setRetentionDays(int days);
    int retentionDays() const;

    void write(LogLevel level, QStringView module, QStringView message);

    // Deletes log files outside the retention window. Returns the number removed.
    int cleanupOldFiles();

    QString directory() const;
    QString currentFilePath() const;
    QString lastError() const;

    // File name for a given day and roll-over index (0 = no suffix).
    static QString fileNameFor(const QDate &date, int index);

    // Routes all Qt messages to this logger. Pass nullptr to restore stderr.
    static void installQtMessageHandler(Logger *logger);

private:
    void ensureFileLocked(const QDate &today, qint64 incomingBytes);
    bool openFileLocked(const QDate &date, int index);
    int findWritableIndexLocked(const QDate &date) const;
    int cleanupLocked(const QDate &today);
    void writeLineLocked(const QString &line);
    QString formatLine(const QDateTime &time, LogLevel level, QStringView module, QStringView message) const;

    const IClock &m_clock;
    mutable QMutex m_mutex;
    Config m_config;
    QFile m_file;
    QDate m_fileDate;
    int m_fileIndex = 0;
    bool m_opened = false;
    QString m_lastError;
};

// Writes a CRITICAL line through the logger installed with
// Logger::installQtMessageHandler() (stderr if none). Qt has no non-fatal
// "critical" message type distinct from errors (qCritical is mapped to ERROR,
// qFatal aborts), so critical failures use this function explicitly.
void logCritical(QStringView module, const QString &message);

} // namespace ergo
