#pragma once

#include "core/Clock.h"
#include "core/input/HookWatchdog.h"
#include "core/input/InputDispatcher.h"
#include "core/input/InputEvent.h"
#include "core/input/KeyCombo.h"
#include "platform/PlatformResult.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace ergo {

class FeatureAvailabilityService;
struct Platform;
enum class PowerEvent;

// Owns the shared global input layer (B.6, B.5.4): starts the platform hook
// once the OS permissions allow it, delivers events through InputDispatcher,
// keeps the key suppression registry, runs the health watchdog every 30 s,
// reinstalls the hook after resume/unlock and reports failures through
// FeatureAvailabilityService. Later stages subscribe to dispatcher().
class InputService : public QObject {
    Q_OBJECT
public:
    // Diagnostics snapshot (General tab, "Input diagnostics").
    struct Stats {
        bool running = false;
        bool canSuppressKeys = false;
        QString backend;
        QString lastError;
        QPointF pointer;
        bool pointerKnown = true;
        int screen = -1;
        QString lastKey;
        QString lastButton;
        QString lastSynthetic;     // last synthetic event (description)
        bool lastEventSynthetic = false;
        int eventsPerSecond = 0;
        quint64 dropped = 0;
        quint64 swallowed = 0;
        int watchdogReinstalls = 0;
        int selfRepairs = 0;
        bool persistentFailure = false;
    };

    InputService(Platform &platform, FeatureAvailabilityService &availability, QObject *parent = nullptr);
    ~InputService() override;

    void start();
    void stop();

    InputDispatcher &dispatcher() { return m_dispatcher; }
    Stats stats() const;

    // Exclusive key registration (Key Control, stage 4; F9 test in diagnostics).
    PlatformResult registerKey(const KeyCombo &combo);
    PlatformResult unregisterKey(const KeyCombo &combo);

    PlatformResult injectLeftClick();

signals:
    void statusChanged();
    // Emitted on the GUI thread for every synthetic mouse-down seen by the hook.
    void syntheticClickSeen(const ergo::InputEvent &event);

private:
    bool tryStartHook(const char *reason);
    void reinstall(const char *reason);
    void onEvent(const InputEvent &event);
    void onPowerEvent(PowerEvent event);
    void runHealthCheck();
    void updateAvailability(const QString &error);

    Platform &m_platform;
    FeatureAvailabilityService &m_availability;
    SystemClock m_clock;
    InputDispatcher m_dispatcher;
    KeySuppressionRegistry m_suppression;
    HookWatchdog m_watchdog;
    QTimer m_watchdogTimer;
    bool m_started = false;
    bool m_starting = false; // re-entry guard (availability changes while starting)
    QString m_lastError;
    int m_lastSelfRepairs = 0;

    // Statistics (GUI thread only).
    Stats m_stats;
    int m_eventsThisSecond = 0;
    QElapsedTimer m_secondTimer;
};

} // namespace ergo
