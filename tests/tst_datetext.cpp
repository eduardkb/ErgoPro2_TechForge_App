#include "core/statistics/DateText.h"

#include <QTest>

using namespace ergo;

// Statistics header (B.8.3, 1.8.1): "<weekday> – <date>" without repeating the weekday.
class TestDateText : public QObject {
    Q_OBJECT
private slots:
    void withoutWeekday_data()
    {
        QTest::addColumn<QString>("locale");
        QTest::addColumn<QString>("expected");
        QTest::newRow("en_GB") << QStringLiteral("en_GB") << QStringLiteral("10 November 2026");
        QTest::newRow("en_US") << QStringLiteral("en_US") << QStringLiteral("November 10, 2026");
        QTest::newRow("de_DE") << QStringLiteral("de_DE") << QStringLiteral("10. November 2026");
        QTest::newRow("pt_PT") << QStringLiteral("pt_PT") << QStringLiteral("10 de novembro de 2026");
    }

    void withoutWeekday()
    {
        QFETCH(QString, locale);
        QFETCH(QString, expected);
        const QLocale l(locale);
        const QDate date(2026, 11, 10); // a Tuesday
        const QString text = longDateWithoutWeekday(date, l);
        QCOMPARE(text, expected);
        QVERIFY(!text.contains(weekdayName(date, l), Qt::CaseInsensitive));
    }

    void weekday()
    {
        QCOMPARE(weekdayName(QDate(2026, 11, 10), QLocale(QStringLiteral("en_GB"))), QStringLiteral("Tuesday"));
        QCOMPARE(weekdayName(QDate(2026, 11, 18), QLocale(QStringLiteral("en_GB"))), QStringLiteral("Wednesday"));
    }
};

QTEST_GUILESS_MAIN(TestDateText)
#include "tst_datetext.moc"
