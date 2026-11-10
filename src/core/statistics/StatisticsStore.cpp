#include "core/statistics/StatisticsStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>

namespace ergo {

namespace {

constexpr int kColumns = 14;
const QString kPrefix = QStringLiteral("ergopro2-stats-");
const QString kSuffix = QStringLiteral(".csv");

QString number(double value, int decimals)
{
    return QString::number(value, 'f', decimals);
}

} // namespace

StatisticsStore::StatisticsStore(QString directory)
    : m_directory(std::move(directory))
{
}

QString StatisticsStore::header()
{
    return QStringLiteral("time,clicks_left,clicks_right,clicks_other,clicks_for_you,key_presses,wheel,"
                          "mouse_distance_px,mouse_distance_cm,mouse_seconds,keyboard_seconds,active_seconds,"
                          "usage_percent,events");
}

QString StatisticsStore::fileNameFor(const QDate &date)
{
    return kPrefix + date.toString(QStringLiteral("yyyy-MM-dd")) + kSuffix;
}

std::optional<QDate> StatisticsStore::dateFromFileName(const QString &fileName)
{
    static const QRegularExpression re(QStringLiteral("^ergopro2-stats-(\\d{4}-\\d{2}-\\d{2})\\.csv$"));
    const QRegularExpressionMatch m = re.match(fileName);
    if (!m.hasMatch())
        return std::nullopt;
    const QDate date = QDate::fromString(m.captured(1), QStringLiteral("yyyy-MM-dd"));
    return date.isValid() ? std::optional<QDate>(date) : std::nullopt;
}

QString StatisticsStore::formatRow(const MinuteRecord &r)
{
    const QString time = QStringLiteral("%1:%2")
                             .arg(r.minute / 60, 2, 10, QLatin1Char('0'))
                             .arg(r.minute % 60, 2, 10, QLatin1Char('0'));
    const QStringList fields{time,
                             QString::number(r.clicksLeft),
                             QString::number(r.clicksRight),
                             QString::number(r.clicksOther),
                             QString::number(r.clicksForYou),
                             QString::number(r.keyPresses),
                             QString::number(r.wheel),
                             number(r.distancePx, 0),
                             number(r.distanceCm, 1),
                             QString::number(r.mouseSeconds),
                             QString::number(r.keyboardSeconds),
                             QString::number(r.activeSeconds),
                             number(r.usagePercent, 1),
                             r.events.join(QLatin1Char(';'))};
    return fields.join(QLatin1Char(','));
}

bool StatisticsStore::parseRow(const QString &line, MinuteRecord *record, QString *error)
{
    const QStringList f = line.trimmed().split(QLatin1Char(','));
    if (f.size() != kColumns) {
        *error = QStringLiteral("expected %1 columns, found %2").arg(kColumns).arg(f.size());
        return false;
    }
    const QTime time = QTime::fromString(f[0], QStringLiteral("HH:mm"));
    if (!time.isValid()) {
        *error = QStringLiteral("invalid time \"%1\"").arg(f[0]);
        return false;
    }
    MinuteRecord r;
    r.minute = time.hour() * 60 + time.minute();
    bool ok = true;
    const auto toInt = [&](int i) {
        bool good = false;
        const int v = f[i].toInt(&good);
        ok = ok && good && v >= 0;
        return v;
    };
    const auto toDouble = [&](int i) {
        bool good = false;
        const double v = f[i].toDouble(&good);
        ok = ok && good && v >= 0;
        return v;
    };
    r.clicksLeft = toInt(1);
    r.clicksRight = toInt(2);
    r.clicksOther = toInt(3);
    r.clicksForYou = toInt(4);
    r.keyPresses = toInt(5);
    r.wheel = toInt(6);
    r.distancePx = toDouble(7);
    r.distanceCm = toDouble(8);
    r.mouseSeconds = toInt(9);
    r.keyboardSeconds = toInt(10);
    r.activeSeconds = toInt(11);
    r.usagePercent = toDouble(12);
    if (!ok || r.mouseSeconds > 60 || r.keyboardSeconds > 60 || r.activeSeconds > 60) {
        *error = QStringLiteral("invalid number");
        return false;
    }
    if (!f[13].isEmpty()) {
        for (const QString &token : f[13].split(QLatin1Char(';'))) {
            BreakEvent e;
            if (!breakEventFromToken(token, &e)) {
                *error = QStringLiteral("unknown event \"%1\"").arg(token);
                return false;
            }
            r.events << token;
        }
    }
    *record = r;
    return true;
}

QString StatisticsStore::filePath(const QDate &date) const
{
    return QDir(m_directory).filePath(fileNameFor(date));
}

bool StatisticsStore::repairPartialLine(const QString &path, QString *problem) const
{
    QFile file(path);
    if (!file.exists() || file.size() == 0 || !file.open(QIODevice::ReadOnly))
        return false;
    const QByteArray content = file.readAll();
    file.close();
    if (content.endsWith('\n'))
        return false;
    const qsizetype lastNewline = content.lastIndexOf('\n');
    const QByteArray kept = lastNewline >= 0 ? content.left(lastNewline + 1) : QByteArray();
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly) || out.write(kept) != kept.size() || !out.commit()) {
        if (problem)
            *problem = QStringLiteral("%1: partial last line could not be removed").arg(QFileInfo(path).fileName());
        return false;
    }
    if (problem) {
        *problem = QStringLiteral("%1: removed an incomplete last line (\"%2\")")
                       .arg(QFileInfo(path).fileName(), QString::fromUtf8(content.mid(lastNewline + 1)));
    }
    return true;
}

bool StatisticsStore::append(const QDate &date, const QList<MinuteRecord> &records, QString *error)
{
    if (records.isEmpty())
        return true;
    if (!QDir().mkpath(m_directory)) {
        *error = QStringLiteral("cannot create the folder %1").arg(QDir::toNativeSeparators(m_directory));
        return false;
    }
    const QString path = filePath(date);
    repairPartialLine(path, nullptr);
    QFile file(path);
    const bool isNew = !file.exists() || file.size() == 0;
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        *error = QStringLiteral("cannot open %1: %2").arg(QDir::toNativeSeparators(path), file.errorString());
        return false;
    }
    QByteArray data;
    if (isNew)
        data += header().toUtf8() + '\n';
    for (const MinuteRecord &r : records)
        data += formatRow(r).toUtf8() + '\n';
    if (file.write(data) != data.size() || !file.flush()) {
        *error = QStringLiteral("cannot write %1: %2").arg(QDir::toNativeSeparators(path), file.errorString());
        return false;
    }
    return true;
}

DayStats StatisticsStore::load(const QDate &date, QStringList *problems) const
{
    DayStats day;
    day.date = date;
    const QString path = filePath(date);
    QString repaired;
    if (repairPartialLine(path, &repaired) && problems)
        problems->append(repaired);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return day;
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    int lineNumber = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        ++lineNumber;
        if (line.trimmed().isEmpty())
            continue;
        if (lineNumber == 1 && line.trimmed() == header())
            continue;
        MinuteRecord r;
        QString error;
        if (parseRow(line, &r, &error))
            day.add(r);
        else if (problems)
            problems->append(
                QStringLiteral("%1 line %2 skipped: %3").arg(fileNameFor(date)).arg(lineNumber).arg(error));
    }
    return day;
}

QList<QDate> StatisticsStore::days() const
{
    QList<QDate> list;
    const QDir dir(m_directory);
    for (const QString &name : dir.entryList({kPrefix + QStringLiteral("*") + kSuffix}, QDir::Files)) {
        if (const auto date = dateFromFileName(name))
            list << *date;
    }
    std::sort(list.begin(), list.end());
    return list;
}

std::optional<QDate> StatisticsStore::previousDay(const QDate &before) const
{
    const QList<QDate> list = days();
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        if (*it < before)
            return *it;
    }
    return std::nullopt;
}

std::optional<QDate> StatisticsStore::nextDay(const QDate &after) const
{
    for (const QDate &d : days()) {
        if (d > after)
            return d;
    }
    return std::nullopt;
}

QStringList StatisticsStore::cleanup(int retentionDays, const QDate &today, QStringList *failed) const
{
    QStringList removed;
    const QDate oldestKept = today.addDays(-(std::max(1, retentionDays) - 1));
    for (const QDate &d : days()) {
        if (d >= oldestKept)
            continue;
        if (QFile::remove(filePath(d)))
            removed << fileNameFor(d);
        else if (failed)
            failed->append(fileNameFor(d));
    }
    return removed;
}

} // namespace ergo
