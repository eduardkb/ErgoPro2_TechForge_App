#include "core/LogProblems.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>

#include <algorithm>

namespace ergo {

namespace {

// Logger::fileNameFor: ergopro2-YYYY-MM-DD.log / ergopro2-YYYY-MM-DD-N.log
const QRegularExpression &fileNamePattern()
{
    static const QRegularExpression re(QStringLiteral(R"(^ergopro2-(\d{4}-\d{2}-\d{2})(-\d+)?\.log$)"));
    return re;
}

// Logger::formatLine: "yyyy-MM-dd HH:mm:ss.zzz [LEVEL] [thread] [module] message"
const QRegularExpression &linePattern()
{
    static const QRegularExpression re(
        QStringLiteral(R"(^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}) \[([A-Z]+)\])"));
    return re;
}

std::optional<LogLevel> problemLevel(QStringView name)
{
    if (name == u"WARNING")
        return LogLevel::Warning;
    if (name == u"ERROR")
        return LogLevel::Error;
    if (name == u"CRITICAL")
        return LogLevel::Critical;
    return std::nullopt;
}

} // namespace

bool logProblemsCardEnabled(LogLevelSelection selection)
{
    return isLevelEnabled(selection, LogLevel::Warning) || isLevelEnabled(selection, LogLevel::Error)
           || isLevelEnabled(selection, LogLevel::Critical);
}

QStringList logFilesForDays(const QStringList &fileNames, const QDate &today, int days)
{
    const QDate first = today.addDays(-(std::max(days, 1) - 1));
    QStringList selected;
    for (const QString &name : fileNames) {
        const QRegularExpressionMatch m = fileNamePattern().match(name);
        if (!m.hasMatch())
            continue;
        const QDate date = QDate::fromString(m.captured(1), Qt::ISODate);
        if (date.isValid() && date >= first)
            selected << name;
    }
    return selected;
}

void collectLogProblems(const QString &text, QList<LogProblem> *problems)
{
    bool continuing = false; // the previous entry is a problem: its continuation lines belong to it
    for (QStringView line : QStringView(text).split(u'\n')) {
        if (line.endsWith(u'\r'))
            line.chop(1);
        const QRegularExpressionMatch m = linePattern().matchView(line);
        if (!m.hasMatch()) {
            if (continuing && !line.isEmpty())
                problems->last().text += QLatin1Char('\n') + line.toString();
            continue;
        }
        const std::optional<LogLevel> level = problemLevel(m.capturedView(2));
        continuing = level.has_value();
        if (!level)
            continue;
        LogProblem p;
        p.time = QDateTime::fromString(m.captured(1), QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        p.level = *level;
        p.text = line.toString();
        problems->append(p);
    }
}

LogProblemScan scanLogProblems(const QString &directory, const QDate &today, int days, int maxEntries)
{
    LogProblemScan scan;
    const QDir dir(directory);
    if (!dir.exists())
        return scan;
    const QStringList files =
        logFilesForDays(dir.entryList({QStringLiteral("ergopro2-*.log")}, QDir::Files, QDir::Name), today, days);
    for (const QString &name : files) {
        QFile file(dir.filePath(name));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            scan.unreadableFiles << QStringLiteral("%1 (%2)").arg(file.fileName(), file.errorString());
            continue;
        }
        ++scan.filesSearched;
        collectLogProblems(QString::fromUtf8(file.readAll()), &scan.problems);
    }
    // Newest first; entries with the same time keep the file order reversed
    // (later lines / roll-over files are newer).
    std::reverse(scan.problems.begin(), scan.problems.end());
    std::stable_sort(scan.problems.begin(), scan.problems.end(),
                     [](const LogProblem &a, const LogProblem &b) { return a.time > b.time; });
    scan.totalFound = int(scan.problems.size());
    if (maxEntries >= 0 && scan.problems.size() > maxEntries)
        scan.problems.resize(maxEntries);
    return scan;
}

QList<LogLevel> logProblemLevels(const QList<LogProblem> &problems)
{
    QList<LogLevel> levels;
    for (LogLevel level : {LogLevel::Warning, LogLevel::Error, LogLevel::Critical}) {
        if (std::any_of(problems.begin(), problems.end(), [level](const LogProblem &p) { return p.level == level; }))
            levels << level;
    }
    return levels;
}

QList<QDate> logProblemDates(const QList<LogProblem> &problems)
{
    QList<QDate> dates;
    for (const LogProblem &p : problems) {
        const QDate d = p.time.date();
        if (d.isValid() && !dates.contains(d))
            dates << d;
    }
    std::sort(dates.begin(), dates.end(), std::greater<QDate>());
    return dates;
}

QList<LogProblem> filterLogProblems(const QList<LogProblem> &problems, const LogProblemFilter &filter)
{
    QList<LogProblem> shown;
    for (const LogProblem &p : problems) {
        if (filter.hiddenLevels.contains(p.level))
            continue;
        if (filter.date && p.time.date() != *filter.date)
            continue;
        shown << p;
    }
    return shown;
}

bool LogProblemSchedule::searchDue(qint64 nowMs, bool tabShown, bool searching) const
{
    if (!tabShown || searching)
        return false;
    return !m_lastStartMs || nowMs - *m_lastStartMs >= m_intervalMs;
}

} // namespace ergo
