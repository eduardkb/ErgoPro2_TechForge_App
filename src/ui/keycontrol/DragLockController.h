#pragma once

#include "core/Clock.h"
#include "core/keycontrol/DragLockEngine.h"

#include <QObject>
#include <QTimer>

#include <memory>

namespace ergo {

class AutoClickController;
class InputService;
class SettingsStore;
struct Platform;

// GUI-thread side of Drag Lock (B.5.3): drives DragLockEngine with the input
// events, holds and releases the left button through the click injector,
// tells the input layer to consume the releasing click, keeps AutoClick
// suppressed while holding and re-arms it from the release position (D-04),
// and makes sure the button is never left down (exit, input lost, crash).
// Release requests from other features (break window, Key Control changes)
// come in through release().
class DragLockController : public QObject {
    Q_OBJECT
public:
    using Reason = DragLockEngine::ReleaseReason;

    DragLockController(SettingsStore &settings, InputService &input, AutoClickController &autoclick,
                       Platform &platform, QObject *parent = nullptr);
    ~DragLockController() override;

    void start();
    void shutdown(); // releases the button (AppExit)

    // Drag-lock hotkey: hold the left button, or release it if held.
    OpResult toggle();
    // Releases the button if held; returns true if it was held.
    bool release(Reason reason);
    bool isHolding() const { return m_engine.isHolding(); }

signals:
    void holdingChanged(bool holding);
    // The button could not be pressed or released (shown on the Key Control tab).
    void failed(const QString &message);

private:
    class Device;

    void onHoldingChanged(bool holding, const QPointF &position);
    void applyTimeout();

    SettingsStore &m_settings;
    InputService &m_input;
    AutoClickController &m_autoclick;
    Platform &m_platform;
    SystemClock m_clock;
    std::unique_ptr<Device> m_device;
    DragLockEngine m_engine;
    QTimer m_timer; // timeout polling while holding
    bool m_started = false;
};

} // namespace ergo
