#pragma once

#include <QDate>
#include <QLocale>
#include <QString>

namespace ergo {

// The locale's long date format without the day of the week, e.g.
// "18 November 2026" (en_GB), "November 18, 2026" (en_US), "18. November 2026"
// (de_DE). Used by the Statistics header "Today – <date>" / "<weekday> – <date>"
// (B.8.3, 1.8.1).
QString longDateWithoutWeekday(const QDate &date, const QLocale &locale);

// Full name of the day of the week in the locale, e.g. "Tuesday".
QString weekdayName(const QDate &date, const QLocale &locale);

} // namespace ergo
