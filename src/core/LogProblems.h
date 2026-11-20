#pragma once

#include "core/LogLevel.h"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace ergo {

// 2.2.0 (owner request): WARNING / ERROR / CRITICAL entries of the recent log
// files, shown in a card on the General tab (spec B.3). Pure functions and a
// small schedule, unit-tested in tests/tst_logproblems.cpp.

// One WARNING / ERROR / CRITICAL entry: the log line and its continuation
// lines (a message that contained line breaks).
struct LogProblem {
    QDateTime time;
    LogLevel level = LogLevel::Warning;
    QString text;
};

// Most entries the card shows (newest first).
inline constexpr int kMaxShownLogProblems = 500;

// The card exists only when the log level writes warnings / errors.
bool logProblemsCardEnabled(LogLevelSelection selection);

// Log file names (Logger::fileNameFor) of the last 'days' days: today and the
// days-1 days before it; a file dated after today is included as well.
QStringList logFilesForDays(const QStringList &fileNames, const QDate &today, int days);

// Appends the problem entries of one log file's text to 'problems'.
void collectLogProblems(const QString &text, QList<LogProblem> *problems);

struct LogProblemScan {
    QList<LogProblem> problems; // newest first, at most maxEntries
    int totalFound = 0;         // before the limit
    int filesSearched = 0;
    QStringList unreadableFiles;
};

// Reads the log files of the last 'days' days in 'directory' (worker thread).
LogProblemScan scanLogProblems(const QString &directory, const QDate &today, int days, int maxEntries);

// 2.2.1 (owner request): filters of the card. Default: everything.
struct LogProblemFilter {
    QList<LogLevel> hiddenLevels; // unticked level boxes
    std::optional<QDate> date;    // nullopt = all dates
};

// Levels present in the list (WARNING, ERROR, CRITICAL order).
QList<LogLevel> logProblemLevels(const QList<LogProblem> &problems);
// Dates that have entries, newest first.
QList<QDate> logProblemDates(const QList<LogProblem> &problems);
// The entries that pass the filter, in the original (newest first) order.
QList<LogProblem> filterLogProblems(const QList<LogProblem> &problems, const LogProblemFilter &filter);

// When the card searches: only while the General tab is shown, at once when
// it is shown and the last search is older than the interval (or there was
// none), then every interval; never two searches at the same time.
class LogProblemSchedule {
public:
    void setIntervalMs(qint64 intervalMs) { m_intervalMs = intervalMs; }
    bool searchDue(qint64 nowMs, bool tabShown, bool searching) const;
    void searchStarted(qint64 nowMs) { m_lastStartMs = nowMs; }
    // The next check searches at once (e.g. the log level or the days changed).
    void invalidate() { m_lastStartMs.reset(); }

private:
    qint64 m_intervalMs = 5 * 60000;
    std::optional<qint64> m_lastStartMs;
};

} // namespace ergo
