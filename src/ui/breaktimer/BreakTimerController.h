#pragma once

#include "core/Clock.h"
#include "core/breaktimer/BreakTimerEngine.h"
#include "core/input/InputEvent.h"

#include <QObject>
#include <QTimer>

#include <memory>

namespace ergo {

class BreakWindowManager;
class FeatureAvailabilityService;
class InputService;
class SettingsStore;
struct Platform;

// GUI-thread side of the Break Timer (B.7): feeds real activity and
// sleep/lock events into the engine, evaluates it every second, shows/hides
// the break window on all screens and applies the settings.
class BreakTimerController : public QObject {
    Q_OBJECT
public:
    BreakTimerController(SettingsStore &settings, FeatureAvailabilityService &availability, InputService &input,
                         Platform &platform, QObject *parent = nullptr);
    ~BreakTimerController() override;

    void start();
    void shutdown();

    bool isEnabled() const;   // user setting
    bool isAvailable() const; // activity monitoring possible (input hook)
    void setEnabled(bool enabled);

    BreakTimerEngine::State state() const { return m_engine.state(); }
    bool isWorkTimePaused() const { return m_engine.isWorkTimePaused(); }
    qint64 interactionMs() const { return m_engine.interactionMs(); }
    qint64 timeUntilBreakMs() const { return m_engine.timeUntilBreakMs(); }
    bool isBreakDelayedByFullscreen() const { return m_engine.isBreakDelayedByFullscreen(); }
    bool fullscreenDetectionSupported() const;

    void dismiss();
    void postpone();
    void startBreakNow();

signals:
    void ticked();           // once per second and after every state change
    void enabledChanged(bool enabled);
    // Activity monitoring became available / unavailable (input hook started, permissions).
    void availabilityChanged(bool available);
    // Emitted when the break window appears / disappears (stage 6: release Drag Lock).
    void breakWindowShown();
    void breakWindowClosed();
    // Statistics (stage 7): BreakEvent (core/statistics/StatisticsTypes.h) as int.
    void breakEvent(int event);

private:
    void applyConfig();
    void updateEnabled();
    void onTick();
    void onInputEvent(const InputEvent &event);
    void onPowerEvent(int event);
    void sync(); // reacts to state changes of the engine

    SettingsStore &m_settings;
    FeatureAvailabilityService &m_availability;
    InputService &m_input;
    Platform &m_platform;
    SystemClock m_clock;
    BreakTimerEngine m_engine;
    std::unique_ptr<BreakWindowManager> m_windows;
    QTimer m_tick;
    BreakTimerEngine::State m_lastState = BreakTimerEngine::State::Disabled;
    bool m_started = false;
    bool m_lastAvailable = false;
    bool m_lastPaused = false;
    bool m_closedByUser = false; // Dismiss / Postpone: not "completed"
};

} // namespace ergo
