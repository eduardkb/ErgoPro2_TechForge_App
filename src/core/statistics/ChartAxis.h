#pragma once

#include "core/statistics/StatisticsTypes.h"

#include <QList>

namespace ergo {

// Axis and bucket logic of the Statistics charts (B.8.3), kept in core so it
// is unit-tested; the widgets only paint.

struct HourRange {
    int startHour = 8; // inclusive
    int endHour = 18;  // exclusive, 1..24
    int startMinute() const { return startHour * 60; }
    int minutes() const { return (endHour - startHour) * 60; }
};

// Hours from the first to the last recorded minute, rounded out to whole
// hours; an empty day shows 08:00-18:00.
HourRange hourRange(const DayStats &day);

// Top of the percent axis: at least 100, rounded up to a step of
// percentAxisStep(). Returns the step through *step if given.
double percentAxisMax(double maxValue, double *step = nullptr);

// "Nice" upper bound for a bar chart scale (1, 2, 5 x 10^n), at least 1.
double niceCeiling(double value);

constexpr int kBucketMinutes = 5;

struct ActivityBucket {
    int startMinute = 0;
    int clicks = 0;      // real clicks
    int keyPresses = 0;
    double distanceCm = 0;
};

// 5-minute buckets of the whole day (288), empty ones included.
QList<ActivityBucket> fiveMinuteBuckets(const DayStats &day);

} // namespace ergo
