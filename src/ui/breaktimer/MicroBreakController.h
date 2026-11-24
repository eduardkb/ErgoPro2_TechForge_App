#pragma once

#include "core/Clock.h"
#include "core/breaktimer/MicroBreakEngine.h"
#include "core/input/InputEvent.h"

#include <QObject>
#include <QRect>
#include <QTimer>

#include <functional>
#include <memory>

namespace ergo {

class BreakTimerController;
class InputService;
class MicroBreakWindow;
class SettingsStore;

// GUI-thread side of Micro-breaks (2.5.0, owner request, spec B.7a): feeds real
// input, lock/sleep and the Break Timer's state into MicroBreakEngine every
// second and shows / hides MicroBreakWindow. Needs the same activity
// monitoring as the Break Timer (Feature::BreakTimerActivity).
class MicroBreakController : public QObject {
    Q_OBJECT
public:
    MicroBreakController(SettingsStore &settings, InputService &input, BreakTimerController &breakTimer,
                         QObject *parent = nullptr);
    ~MicroBreakController() override;

    void start();
    void shutdown();

    bool isEnabled() const;   // user setting
    bool isAvailable() const; // activity monitoring possible
    void setEnabled(bool enabled);

    MicroBreakEngine::State state() const { return m_engine.state(); }
    qint64 activeMs() const { return m_engine.activeMs(); }
    qint64 timeUntilMs() const { return m_engine.timeUntilMs(); }
    bool isPaused() const { return m_engine.isPaused(); }
    bool isWaitingForBreakTimer() const { return m_blocked; }

    void setTrayIconGeometryProvider(std::function<QRect()> provider) { m_trayGeometry = std::move(provider); }

signals:
    void ticked(); // once per second and after every change
    void availabilityChanged(bool available);

private:
    void applyConfig();
    void updateEnabled();
    void onTick();
    void onInputEvent(const InputEvent &event);
    void onPowerEvent(int event);
    void sync();

    SettingsStore &m_settings;
    InputService &m_input;
    BreakTimerController &m_breakTimer;
    SystemClock m_clock;
    MicroBreakEngine m_engine;
    std::unique_ptr<MicroBreakWindow> m_window;
    std::function<QRect()> m_trayGeometry;
    QTimer m_tick;
    MicroBreakEngine::State m_lastState = MicroBreakEngine::State::Disabled;
    bool m_started = false;
    bool m_blocked = false;
    bool m_dismissedByUser = false;
};

} // namespace ergo
