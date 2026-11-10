#pragma once

#include "core/Clock.h"
#include "core/input/InputEvent.h"

#include <QPointF>
#include <QSet>

namespace ergo {

// Dwell-click decision logic (implementation_stages.md B.4). Pure: driven by
// input events and an injectable clock; it never clicks itself. The caller
// (AutoClickWorker) schedules a timer for nextDeadlineMs() and calls poll(),
// which returns true when exactly one left click must be performed now.
//
// Rules:
//  1. Armed when the pointer is >= distancePx (straight line, logical px) from
//     the reference point (last click position, or the first position seen
//     after enabling).
//  2. While armed, a movement larger than jitterPx restarts the stop timer;
//     smaller movements are ignored (tremor).
//  3. No movement beyond the tolerance for triggerMs -> one click at the
//     current position -> reference = that position -> disarmed.
//  4. A manual click (any button), or a Key Control click (onExternalClick),
//     cancels a pending click, disarms and moves the reference point to the
//     click position (on press and again on release, so a manual drag never
//     leaves AutoClick armed at the drop position).
//  5. No auto-click while any mouse button is held.
//  6. No auto-click within typingSuspendMs after a key press (modifier keys
//     alone do not count, so Shift/Cmd + dwell-click keeps working). A pending
//     click is delayed, not cancelled.
//  7. Drag Lock (stage 6): no auto-click while active; on release the
//     reference point is the release position.
//  Synthetic events (ErgoPro2's own clicks) are ignored.
class AutoClickEngine {
public:
    struct Config {
        int triggerMs = 200;
        int distancePx = 15;
        int jitterPx = 3;
        int typingSuspendMs = 500;
    };

    enum class State {
        Disabled,
        WaitingForMovement, // disarmed: the pointer must travel distancePx first
        Armed,              // armed and moving
        Countdown,          // armed, pointer resting: click when the timer expires
        Suppressed          // armed, but a button is held / typing / Drag Lock
    };

    enum class SuppressReason { None, ButtonHeld, Typing, DragLock };

    // A pointer that moved within this time counts as "moving" (Armed state).
    static constexpr int kMovingWindowMs = 50;

    explicit AutoClickEngine(const IClock &clock);

    void setConfig(const Config &config);
    const Config &config() const { return m_config; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void onInputEvent(const InputEvent &event);
    void onExternalClick(const QPointF &position);              // Key Control click (stage 4)
    void setDragLockActive(bool active, const QPointF &position); // Drag Lock (stage 6)
    void resetReferencePoint(const QPointF &position);

    // True if a click must be performed now; the engine is then disarmed with
    // the reference point at the current position.
    bool poll();

    // Monotonic time at which poll() may return true, or -1 if no click is pending.
    qint64 nextDeadlineMs() const;

    State state() const;
    SuppressReason suppressReason() const;
    // 0..1 progress of the countdown (Countdown state only).
    double countdownProgress() const;

    // Visual feedback timing (1.4.2): the countdown X is shown only after the
    // pointer made no movement at all - not even tremor below the tolerance -
    // for half the trigger time, and then grows until the click. Returns the
    // monotonic time from which it is shown, or -1 if it must not be shown for
    // the pending click (nothing pending, suppressed, or not enough still time
    // left). The click timing itself is not affected.
    qint64 feedbackShowAtMs() const;
    bool isFeedbackVisible() const;
    int remainingMs() const;

    QPointF position() const { return m_position; }
    QPointF referencePoint() const { return m_reference; }
    bool hasReference() const { return m_hasReference; }
    bool isArmed() const { return m_armed; }

private:
    void resetMotion(const QPointF &position);
    void onMove(const QPointF &position);
    void onButton(const InputEvent &event);
    void onKey(const InputEvent &event);
    qint64 now() const { return m_clock.monotonicMs(); }

    const IClock &m_clock;
    Config m_config;
    bool m_enabled = false;
    bool m_armed = false;
    bool m_hasReference = false;
    bool m_dragLock = false;
    QPointF m_reference;
    QPointF m_position;
    QPointF m_lastSignificant; // last position that moved more than jitterPx
    qint64 m_lastMoveMs = 0;
    qint64 m_lastAnyMoveMs = 0; // last movement of any size (for the visual feedback)
    qint64 m_suspendUntilMs = 0;
    QSet<int> m_heldButtons;
};

} // namespace ergo
