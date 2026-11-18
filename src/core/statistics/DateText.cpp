#include "core/statistics/DateText.h"

#include <QRegularExpression>

namespace ergo {

QString longDateWithoutWeekday(const QDate &date, const QLocale &locale)
{
    QString format = locale.dateFormat(QLocale::LongFormat);
    // Remove the weekday tokens "dddd" / "ddd" (not "d" / "dd" = day number)
    // with the punctuation and spaces around them.
    static const QRegularExpression weekday(QStringLiteral("[\\s,]*(?<!d)d{3,4}(?!d)[,.]?\\s*"));
    format.replace(weekday, QStringLiteral(" "));
    format = format.trimmed();
    while (format.endsWith(QLatin1Char(',')))
        format.chop(1);
    if (format.isEmpty())
        return locale.toString(date, QLocale::ShortFormat);
    return locale.toString(date, format);
}

QString weekdayName(const QDate &date, const QLocale &locale)
{
    return locale.dayName(date.dayOfWeek(), QLocale::LongFormat);
}

} // namespace ergo
