#include "core/statistics/StatisticsTypes.h"

#include <algorithm>

namespace ergo {

QString breakEventToken(BreakEvent event)
{
    switch (event) {
    case BreakEvent::Shown: return QStringLiteral("break_shown");
    case BreakEvent::Dismissed: return QStringLiteral("break_dismissed");
    case BreakEvent::Postponed: return QStringLiteral("break_postponed");
    case BreakEvent::Completed: return QStringLiteral("break_completed");
    }
    return {};
}

bool breakEventFromToken(const QString &token, BreakEvent *event)
{
    for (BreakEvent e : {BreakEvent::Shown, BreakEvent::Dismissed, BreakEvent::Postponed, BreakEvent::Completed}) {
        if (breakEventToken(e) == token) {
            *event = e;
            return true;
        }
    }
    return false;
}

bool MinuteRecord::hasData() const
{
    return clicks() > 0 || clicksForYou > 0 || keyPresses > 0 || wheel > 0 || distancePx > 0 || mouseSeconds > 0
           || keyboardSeconds > 0 || activeSeconds > 0 || !events.isEmpty();
}

void MinuteRecord::merge(const MinuteRecord &o)
{
    clicksLeft += o.clicksLeft;
    clicksRight += o.clicksRight;
    clicksOther += o.clicksOther;
    clicksForYou += o.clicksForYou;
    keyPresses += o.keyPresses;
    wheel += o.wheel;
    distancePx += o.distancePx;
    distanceCm += o.distanceCm;
    // A minute has at most 60 seconds, even when it was recorded in two parts.
    mouseSeconds = std::min(60, mouseSeconds + o.mouseSeconds);
    keyboardSeconds = std::min(60, keyboardSeconds + o.keyboardSeconds);
    activeSeconds = std::min(60, activeSeconds + o.activeSeconds);
    usagePercent = std::max(usagePercent, o.usagePercent);
    events += o.events;
}

void DayStats::add(const MinuteRecord &record)
{
    auto it = std::lower_bound(minutes.begin(), minutes.end(), record.minute,
                               [](const MinuteRecord &r, int minute) { return r.minute < minute; });
    if (it != minutes.end() && it->minute == record.minute)
        it->merge(record);
    else
        minutes.insert(it, record);
}

const MinuteRecord *DayStats::find(int minute) const
{
    auto it = std::lower_bound(minutes.begin(), minutes.end(), minute,
                               [](const MinuteRecord &r, int m) { return r.minute < m; });
    return it != minutes.end() && it->minute == minute ? &*it : nullptr;
}

DayTotals computeTotals(const DayStats &day)
{
    DayTotals t;
    for (const MinuteRecord &r : day.minutes) {
        t.clicksLeft += r.clicksLeft;
        t.clicksRight += r.clicksRight;
        t.clicksOther += r.clicksOther;
        t.clicksForYou += r.clicksForYou;
        t.keyPresses += r.keyPresses;
        t.wheel += r.wheel;
        t.distanceCm += r.distanceCm;
        t.mouseSeconds += r.mouseSeconds;
        t.keyboardSeconds += r.keyboardSeconds;
        t.activeSeconds += r.activeSeconds;
        t.maxUsagePercent = std::max(t.maxUsagePercent, r.usagePercent);
    }
    return t;
}

} // namespace ergo
