#include "core/statistics/ChartAxis.h"

#include <algorithm>
#include <cmath>

namespace ergo {

HourRange hourRange(const DayStats &day)
{
    if (day.minutes.isEmpty())
        return {};
    HourRange r;
    r.startHour = day.minutes.first().minute / 60;
    r.endHour = (day.minutes.last().minute / 60) + 1;
    r.startHour = std::clamp(r.startHour, 0, 23);
    r.endHour = std::clamp(r.endHour, r.startHour + 1, 24);
    return r;
}

double percentAxisMax(double maxValue, double *step)
{
    const double top = std::max(100.0, maxValue);
    const double s = top <= 200 ? 25.0 : top <= 500 ? 50.0 : 100.0;
    if (step)
        *step = s;
    return std::ceil(top / s) * s;
}

double niceCeiling(double value)
{
    if (value <= 1)
        return 1;
    const double magnitude = std::pow(10.0, std::floor(std::log10(value)));
    for (double m : {1.0, 2.0, 5.0, 10.0}) {
        if (value <= m * magnitude)
            return m * magnitude;
    }
    return 10 * magnitude;
}

QList<ActivityBucket> fiveMinuteBuckets(const DayStats &day)
{
    QList<ActivityBucket> buckets(kMinutesPerDay / kBucketMinutes);
    for (int i = 0; i < buckets.size(); ++i)
        buckets[i].startMinute = i * kBucketMinutes;
    for (const MinuteRecord &r : day.minutes) {
        if (r.minute < 0 || r.minute >= kMinutesPerDay)
            continue;
        ActivityBucket &b = buckets[r.minute / kBucketMinutes];
        b.clicks += r.clicks();
        b.keyPresses += r.keyPresses;
        b.distanceCm += r.distanceCm;
    }
    return buckets;
}

} // namespace ergo
