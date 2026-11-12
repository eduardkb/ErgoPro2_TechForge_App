#pragma once

#include "core/Clock.h"
#include "core/statistics/StatisticsCollector.h"
#include "core/statistics/StatisticsStore.h"

#include <QDate>
#include <QObject>
#include <QTimer>

#include <optional>

namespace ergo {

class AutoClickController;
class BreakTimerController;
class FeatureAvailabilityService;
class InputService;
class KeyControlController;
class SettingsStore;

// GUI-thread side of Function 4 – Statistics (B.8): feeds the collector with
// the input events (delivered by the dispatcher on the GUI thread; the hook
// thread does no statistics work), ErgoPro2's own clicks, break events and
// sleep/lock; writes every finished minute to the day's CSV file; continues
// today's data after a restart; deletes old files (retention).
class StatisticsController : public QObject {
    Q_OBJECT
public:
    StatisticsController(SettingsStore &settings, FeatureAvailabilityService &availability, InputService &input,
                         AutoClickController &autoclick, KeyControlController &keycontrol,
                         BreakTimerController &breaktimer, QObject *parent = nullptr);
    ~StatisticsController() override;

    // Default folder: QStandardPaths::AppDataLocation/statistics.
    static QString defaultDirectory();

    void start();
    void shutdown(); // writes the current (partial) minute

    QString directory() const { return m_store.directory(); }
    bool isAvailable() const; // recording possible (input hook running)
    bool isUsageOverLimit() const { return m_usageOverLimit; }

    QDate today() const { return m_collector.currentDate(); }
    // Today: the live data (finished minutes); other days: read from the file.
    DayStats day(const QDate &date) const;
    // Nearest day with data before / after 'date' (today always counts).
    std::optional<QDate> previousDay(const QDate &date) const;
    std::optional<QDate> nextDay(const QDate &date) const;

signals:
    void todayChanged();     // a minute of today was finished (once per minute)
    void daysChanged();      // files were added or deleted (startup, new day)
    void availabilityChanged();
    // The usage percent went above / back to at most 100 % (1.7.6: the Break
    // Timer then uses half of "Break after working").
    void usageOverLimitChanged(bool over);

private:
    void applyConfig();
    void onTick();
    void writeFinished();
    void cleanup(const char *reason);
    void restore();
    void checkUsageLimit();
    void applyNewScale(const char *reason);

    SettingsStore &m_settings;
    FeatureAvailabilityService &m_availability;
    InputService &m_input;
    AutoClickController &m_autoclick;
    KeyControlController &m_keycontrol;
    BreakTimerController &m_breaktimer;
    SystemClock m_clock;
    StatisticsCollector m_collector;
    StatisticsStore m_store;
    QTimer m_tick;
    QDate m_lastDate;
    bool m_started = false;
    bool m_writeFailing = false;
    bool m_lastAvailable = false;
    bool m_usageOverLimit = false;
};

} // namespace ergo
