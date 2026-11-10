#pragma once

#include "core/Clock.h"
#include "core/input/InputEvent.h"
#include "core/keycontrol/KeyAction.h"

#include <QPointF>
#include <QString>

#include <functional>

namespace ergo {

// What the drag lock needs from the system (implemented by the application
// with the click injector and the input layer; faked in unit tests).
class IDragLockDevice {
public:
    virtual ~IDragLockDevice() = default;
    // Left button down / up at the current pointer position (synthetic).
    virtual OpResult pressLeft() = 0;
    virtual OpResult releaseLeft() = 0;
    // Called with true BEFORE the press and with false AFTER the release:
    // the input layer then swallows the next real click, turns pointer moves
    // into drags (macOS) and the crash handler knows a button is held.
    virtual void setHolding(bool holding) = 0;
};

// Drag Lock state machine (B.5.3, D-01..D-06). Pure: driven by input events,
// the injectable clock and explicit release requests from the application.
//
//   Idle --toggle()--> Holding   (left button down at the current position)
//   Holding --release(reason)--> Idle   (left button up at the current position)
//
// Release reasons handled inside the engine: the hotkey again (toggle), the
// timeout since the last mouse movement (poll), any non-modifier key press
// (passed through to the OS), any real mouse-button press (consumed by the
// input layer). All others (break window, exit, Key Control off, binding
// changed, input lost, ...) are requested by the application via release().
// There is only one engine, so only one drag lock can be active at a time.
class DragLockEngine {
public:
    enum class State { Idle, Holding };
    enum class ReleaseReason {
        Hotkey,         // the drag-lock hotkey was pressed again
        Timeout,        // DRAGLOCK_TIMEOUT without mouse movement (D-02)
        KeyPressed,     // a non-modifier key (or another hotkey) was pressed
        MouseClick,     // a mouse button was pressed (the click is consumed, D-03)
        BreakWindow,    // the break window appeared (D-06)
        AppExit,        // the application exits
        KeyControlOff,  // Key Control paused or unavailable
        BindingChanged, // the drag-lock hotkey was disabled, deleted or modified
        InputLost,      // the input hook stopped (reinstall failed)
        AutoClickOff,   // AutoClick was switched off (B.5.3 safety rule)
        Error,          // unexpected error
    };

    // holding, pointer position (press or release position).
    using HoldingCallback = std::function<void(bool holding, const QPointF &position)>;

    DragLockEngine(const IClock &clock, IDragLockDevice &device);

    void setTimeoutMs(int timeoutMs) { m_timeoutMs = timeoutMs; }
    int timeoutMs() const { return m_timeoutMs; }
    void setHoldingCallback(HoldingCallback callback) { m_callback = std::move(callback); }

    // Hotkey: Idle -> press and hold; Holding -> release (ReleaseReason::Hotkey).
    // 'pointer' is the current pointer position (used until the first move).
    OpResult toggle(const QPointF &pointer);

    // Releases the button if held. Returns true if a drag lock was released.
    bool release(ReleaseReason reason);

    // Every input event (on the GUI thread, in order). Tracks the pointer and
    // releases on key presses and mouse clicks while Holding.
    void onInputEvent(const InputEvent &event);

    // Releases after the timeout; returns true if it released. Call
    // periodically while Holding (see nextDeadlineMs()).
    bool poll();
    // Monotonic time of the timeout release, -1 while Idle.
    qint64 nextDeadlineMs() const;

    State state() const { return m_state; }
    bool isHolding() const { return m_state == State::Holding; }
    QPointF position() const { return m_position; }
    ReleaseReason lastReleaseReason() const { return m_lastReason; }
    // Result of the last button-up injection (false = the button may be stuck).
    OpResult lastReleaseResult() const { return m_lastReleaseResult; }

    static QString reasonText(ReleaseReason reason); // for the log (English)

private:
    qint64 now() const { return m_clock.monotonicMs(); }

    const IClock &m_clock;
    IDragLockDevice &m_device;
    HoldingCallback m_callback;
    State m_state = State::Idle;
    int m_timeoutMs = 10000;
    qint64 m_lastMoveMs = 0;
    QPointF m_position;
    bool m_positionSeen = false;
    ReleaseReason m_lastReason = ReleaseReason::Hotkey;
    OpResult m_lastReleaseResult;
};

} // namespace ergo
