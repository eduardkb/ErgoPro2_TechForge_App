#include "ui/statistics/StatisticsController.h"

#include "core/LogCategories.h"
#include "core/settings/BreakTimerSettings.h"
#include "core/settings/SettingsStore.h"
#include "core/settings/StatisticsSettings.h"
#include "platform/PlatformInterfaces.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/InputService.h"
#include "ui/autoclick/AutoClickController.h"
#include "ui/breaktimer/BreakTimerController.h"
#include "ui/keycontrol/KeyControlController.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>

namespace ergo {

namespace {

constexpr int kTickMs = 1000;
constexpr double kFallbackDpi = 96.0;

// Logical pixels per centimetre of the screen at 'point' (B.8.1).
double pixelsPerCmAt(const QPointF &point)
{
    QScreen *screen = QGuiApplication::screenAt(point.toPoint());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    double dpi = screen ? screen->physicalDotsPerInch() : 0.0;
    if (!(dpi > 10.0 && dpi < 2000.0))
        dpi = kFallbackDpi;
    return dpi / 2.54;
}

} // namespace

StatisticsController::StatisticsController(SettingsStore &settings, FeatureAvailabilityService &availability,
                                           InputService &input, AutoClickController &autoclick,
                                           KeyControlController &keycontrol, BreakTimerController &breaktimer,
                                           QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_availability(availability)
    , m_input(input)
    , m_autoclick(autoclick)
    , m_keycontrol(keycontrol)
    , m_breaktimer(breaktimer)
    , m_collector(m_clock)
    , m_store(defaultDirectory())
{
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, &StatisticsController::onTick);
}

StatisticsController::~StatisticsController()
{
    shutdown();
}

QString StatisticsController::defaultDirectory()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("statistics"));
}

void StatisticsController::start()
{
    if (m_started)
        return;
    m_started = true;
    applyConfig();
    m_lastDate = m_collector.currentDate();
    cleanup("startup");
    restore();

    connect(&m_input.dispatcher(), &InputDispatcher::inputEvent, this,
            [this](const InputEvent &event) { m_collector.onInputEvent(event); });
    connect(&m_input, &InputService::powerEventOccurred, this, [this](int event) {
        switch (PowerEvent(event)) {
        case PowerEvent::Sleep:
        case PowerEvent::ScreenLocked: m_collector.setLocked(true); break;
        case PowerEvent::Resume:
        case PowerEvent::ScreenUnlocked: m_collector.setLocked(false); break;
        }
    });
    connect(&m_autoclick, &AutoClickController::clickPerformed, this, [this] { m_collector.onClicksDoneForYou(1); });
    connect(&m_keycontrol, &KeyControlController::clicksDoneForYou, this,
            [this](int count) { m_collector.onClicksDoneForYou(count); });
    connect(&m_breaktimer, &BreakTimerController::breakEvent, this, [this](int event) {
        m_collector.onBreakEvent(BreakEvent(event));
        if (BreakEvent(event) == BreakEvent::Completed)
            qCInfo(lcApp) << "Statistics: break completed - usage reset to 0 %";
        checkUsageLimit();
    });
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key == BreakTimerSettings::MaxWorkMin || key == BreakTimerSettings::BreakMin)
            applyConfig();
        else if (key == StatisticsSettings::RetentionDays)
            // Applied at the next start or day change only, so moving the slider by
            // mistake deletes nothing (1.7.1, owner request).
            qCInfo(lcApp) << "Statistics retention set to" << m_settings.intValue(key)
                          << "day(s); applied at the next start or day change";
    });
    const auto availabilityCheck = [this] {
        const bool available = isAvailable();
        if (available != m_lastAvailable) {
            m_lastAvailable = available;
            qCInfo(lcApp) << "Statistics recording is" << (available ? "running" : "waiting for the input hook");
            emit availabilityChanged();
        }
    };
    connect(&m_input, &InputService::statusChanged, this, availabilityCheck);
    connect(&m_availability, &FeatureAvailabilityService::changed, this, availabilityCheck);
    availabilityCheck();
    m_tick.start();
    qCInfo(lcApp).noquote() << "Statistics folder:" << QDir::toNativeSeparators(m_store.directory());
    // The tab was built before today's file was read.
    emit daysChanged();
    emit todayChanged();
    checkUsageLimit(); // a restored usage can already be above 100 %
}

void StatisticsController::shutdown()
{
    if (!m_started)
        return;
    m_tick.stop();
    m_collector.flush();
    writeFinished();
    m_started = false;
    qCInfo(lcApp) << "Statistics saved";
}

bool StatisticsController::isAvailable() const
{
    return m_availability.status(Feature::Statistics).available && m_input.stats().running;
}

void StatisticsController::applyConfig()
{
    StatisticsCollector::Config c;
    c.maxWorkMs = qint64(m_settings.intValue(BreakTimerSettings::MaxWorkMin)) * 60000;
    c.resetAfterMs = qint64(m_settings.intValue(BreakTimerSettings::BreakMin)) * 60000;
    m_collector.setConfig(c);
    qCInfo(lcApp) << "Statistics: usage 100 % =" << m_collector.scaleMaxWorkMs() / 60000
                  << "min of use (setting:" << c.maxWorkMs / 60000
                  << "min, applies when the usage restarts at 0), reset after" << c.resetAfterMs / 60000
                  << "min without input or when a break completes";
}

void StatisticsController::restore()
{
    QStringList problems;
    const QDate today = m_collector.currentDate();
    const DayStats day = m_store.load(today, &problems);
    // The last stored minute (today, else the day before) continues the usage percent.
    qint64 lastEndMs = 0;
    double lastPercent = 0;
    DayStats last = day;
    if (last.isEmpty()) {
        if (const auto previous = m_store.previousDay(today); previous && *previous == today.addDays(-1))
            last = m_store.load(*previous, &problems);
    }
    if (!last.isEmpty()) {
        const MinuteRecord &r = last.minutes.last();
        lastEndMs = QDateTime(last.date, QTime(r.minute / 60, r.minute % 60)).addSecs(60).toMSecsSinceEpoch();
        lastPercent = r.usagePercent;
    }
    for (const QString &p : std::as_const(problems))
        qCWarning(lcApp).noquote() << "Statistics:" << p;
    m_collector.restore(day, lastEndMs, lastPercent);
    if (!day.isEmpty())
        qCInfo(lcApp) << "Statistics: continuing today with" << day.minutes.size() << "stored minute(s)";
}

void StatisticsController::onTick()
{
    m_collector.setPixelsPerCm(pixelsPerCmAt(m_input.stats().pointer));
    m_collector.tick();
    writeFinished();
    checkUsageLimit();
    if (m_collector.currentDate() != m_lastDate) {
        qCInfo(lcApp).noquote() << "Statistics: new day" << m_collector.currentDate().toString(Qt::ISODate);
        m_lastDate = m_collector.currentDate();
        cleanup("new day");
        emit daysChanged();
        emit todayChanged();
    }
}

void StatisticsController::checkUsageLimit()
{
    const double percent = m_collector.usagePercent();
    const bool over = percent > 100.0;
    if (over == m_usageOverLimit)
        return;
    m_usageOverLimit = over;
    qCInfo(lcApp).noquote() << QStringLiteral("Statistics: usage %1 %% (100 %% = %2 min) - %3")
                                   .arg(percent, 0, 'f', 1)
                                   .arg(m_collector.scaleMaxWorkMs() / 60000)
                                   .arg(over ? QStringLiteral("above 100 %, breaks after half the work time")
                                             : QStringLiteral("at most 100 %"));
    emit usageOverLimitChanged(over);
}

void StatisticsController::writeFinished()
{
    const QList<StatisticsCollector::FinishedMinute> finished = m_collector.takeFinished();
    if (finished.isEmpty())
        return;
    bool today = false;
    bool newFile = false;
    // Group consecutive minutes of the same date into one append.
    int i = 0;
    while (i < finished.size()) {
        const QDate date = finished[i].date;
        QList<MinuteRecord> records;
        while (i < finished.size() && finished[i].date == date)
            records << finished[i++].record;
        newFile = newFile || !QFile::exists(m_store.filePath(date));
        QString error;
        if (m_store.append(date, records, &error)) {
            if (m_writeFailing)
                qCInfo(lcApp) << "Statistics: writing works again";
            m_writeFailing = false;
        } else if (!m_writeFailing) {
            m_writeFailing = true; // log once per failure streak
            qCCritical(lcApp).noquote() << "Statistics could not be saved:" << error;
        }
        today = today || date == m_collector.currentDate();
        qCDebug(lcApp) << "Statistics:" << records.size() << "minute(s) written for" << date.toString(Qt::ISODate);
    }
    if (newFile)
        emit daysChanged();
    if (today)
        emit todayChanged();
}

void StatisticsController::cleanup(const char *reason)
{
    QStringList failed;
    const int days = m_settings.intValue(StatisticsSettings::RetentionDays);
    const QStringList removed = m_store.cleanup(days, m_collector.currentDate(), &failed);
    for (const QString &name : removed)
        qCInfo(lcApp).noquote() << "Statistics: deleted" << name << "(older than" << days << "days," << reason << ")";
    for (const QString &name : failed)
        qCWarning(lcApp).noquote() << "Statistics: could not delete" << name;
    if (!removed.isEmpty())
        emit daysChanged();
}

DayStats StatisticsController::day(const QDate &date) const
{
    if (date == m_collector.currentDate())
        return m_collector.today();
    QStringList problems;
    DayStats d = m_store.load(date, &problems);
    for (const QString &p : std::as_const(problems))
        qCWarning(lcApp).noquote() << "Statistics:" << p;
    return d;
}

std::optional<QDate> StatisticsController::previousDay(const QDate &date) const
{
    return m_store.previousDay(date);
}

std::optional<QDate> StatisticsController::nextDay(const QDate &date) const
{
    const QDate today = m_collector.currentDate();
    if (date >= today)
        return std::nullopt;
    const auto next = m_store.nextDay(date);
    if (next && *next < today)
        return next;
    return today; // today always exists (live)
}

} // namespace ergo
