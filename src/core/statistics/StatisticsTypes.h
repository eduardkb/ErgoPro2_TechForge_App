#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>

namespace ergo {

// Statistics data model (implementation_stages.md B.8). Only counts and
// durations: never key identities, pointer positions or application names.

// Break Timer events shown in the usage chart and stored in the CSV.
enum class BreakEvent { Shown, Dismissed, Postponed, Completed };

QString breakEventToken(BreakEvent event); // "break_shown", ... (CSV)
bool breakEventFromToken(const QString &token, BreakEvent *event);

constexpr int kMinutesPerDay = 24 * 60;

// One minute of one day (local time).
struct MinuteRecord {
    int minute = 0; // minute of the day, 0..1439
    int clicksLeft = 0;
    int clicksRight = 0;
    int clicksOther = 0;
    int clicksForYou = 0; // clicks made by Auto Click / Key Control / Drag Lock
    int keyPresses = 0;
    int wheel = 0;
    double distancePx = 0; // logical pixels
    double distanceCm = 0; // approximate (screen DPI)
    int mouseSeconds = 0;
    int keyboardSeconds = 0;
    int activeSeconds = 0;
    double usagePercent = 0; // value at the end of the minute (B.8.2)
    QStringList events;      // break event tokens

    int clicks() const { return clicksLeft + clicksRight + clicksOther; }
    bool hasData() const;
    // Adds another record of the same minute (e.g. written twice after a restart).
    void merge(const MinuteRecord &other);
};

// All recorded minutes of one day, sorted by minute, at most one per minute.
struct DayStats {
    QDate date;
    QList<MinuteRecord> minutes;

    bool isEmpty() const { return minutes.isEmpty(); }
    void add(const MinuteRecord &record); // merges a record of an existing minute
    const MinuteRecord *find(int minute) const;
};

struct DayTotals {
    int clicksLeft = 0;
    int clicksRight = 0;
    int clicksOther = 0;
    int clicksForYou = 0;
    int keyPresses = 0;
    int wheel = 0;
    double distanceCm = 0;
    qint64 mouseSeconds = 0;
    qint64 keyboardSeconds = 0;
    qint64 activeSeconds = 0;
    double maxUsagePercent = 0;

    int clicks() const { return clicksLeft + clicksRight + clicksOther; }
};

DayTotals computeTotals(const DayStats &day);

} // namespace ergo
