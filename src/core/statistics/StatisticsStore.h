#pragma once

#include "core/statistics/StatisticsTypes.h"

#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace ergo {

// One CSV file per day in one folder (implementation_stages.md B.8.4):
//   ergopro2-stats-YYYY-MM-DD.csv, UTF-8, header line, one row per minute.
// Rows of the same minute (written twice after a restart) are merged on read.
class StatisticsStore {
public:
    explicit StatisticsStore(QString directory);

    QString directory() const { return m_directory; }

    static QString header();
    static QString fileNameFor(const QDate &date);
    static std::optional<QDate> dateFromFileName(const QString &fileName);
    static QString formatRow(const MinuteRecord &record);
    // Returns false (and sets *error) for a malformed line.
    static bool parseRow(const QString &line, MinuteRecord *record, QString *error);

    QString filePath(const QDate &date) const;

    // Appends the rows (creates the folder and the file with its header if
    // needed; repairs a partial last line first). False + *error on failure.
    bool append(const QDate &date, const QList<MinuteRecord> &records, QString *error);

    // Reads one day. A partial last line (crash while writing) is removed from
    // the file; unreadable lines are skipped. Both are reported in *problems.
    DayStats load(const QDate &date, QStringList *problems = nullptr) const;

    // Dates that have a file, oldest first.
    QList<QDate> days() const;
    std::optional<QDate> previousDay(const QDate &before) const;
    std::optional<QDate> nextDay(const QDate &after) const;

    // Deletes files older than retentionDays (today counts as day 1).
    // Returns the deleted file names; files that could not be deleted are
    // listed in *failed.
    QStringList cleanup(int retentionDays, const QDate &today, QStringList *failed = nullptr) const;

private:
    // Removes a partial last line; returns true if the file was changed.
    bool repairPartialLine(const QString &path, QString *problem) const;

    QString m_directory;
};

} // namespace ergo
