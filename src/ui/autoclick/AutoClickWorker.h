#pragma once

#include "core/Clock.h"
#include "core/autoclick/AutoClickEngine.h"
#include "core/input/InputEvent.h"

#include <QObject>
#include <QPointF>

class QTimer;

namespace ergo {

class IClickInjector;

// Runs the AutoClick engine in its OWN thread (B.4): receives input events
// from the dispatcher (queued), schedules a precise timer for the engine's
// next deadline and injects the click. Recognizes its own click when the hook
// reports it back as synthetic. All slots must be invoked queued.
class AutoClickWorker : public QObject {
    Q_OBJECT
public:
    explicit AutoClickWorker(IClickInjector &injector);

public slots:
    void initialize(); // creates the timer in the worker thread
    void setEnabled(bool enabled);
    void setConfig(int triggerMs, int distancePx, int jitterPx);
    void setFeedbackEnabled(bool enabled);
    void onInputEvent(const ergo::InputEvent &event);
    void externalClick(const QPointF &position);              // Key Control (stage 4)
    void setDragLockActive(bool active, const QPointF &position); // Drag Lock (stage 6)

signals:
    // State for the tab (AutoClickEngine::State / SuppressReason as int).
    void stateChanged(int state, int suppressReason);
    // Visual feedback: the countdown X is shown at 'position' and grows from
    // startMs to deadlineMs (monotonicNowMs epoch), or is hidden (active = false).
    // It is shown only after the pointer has been completely still for half
    // the trigger time (AutoClickEngine::feedbackShowAtMs, 1.4.2).
    void countdownChanged(bool active, const QPointF &position, bool positionKnown, qint64 startMs,
                          qint64 deadlineMs);
    void clicked(const QPointF &position);
    void clickFailed(const QString &message);

private:
    void schedule();
    void onTimeout();
    void publish();

    IClickInjector &m_injector;
    SystemClock m_clock;
    AutoClickEngine m_engine;
    QTimer *m_timer = nullptr;
    QTimer *m_feedbackTimer = nullptr; // fires when the X may become visible
    bool m_feedback = false;
    bool m_positionKnown = true;
    int m_lastState = -1;
    int m_lastReason = -1;
    qint64 m_lastDeadline = -2;
    qint64 m_lastShowAt = -2;
    bool m_lastVisible = false;
    qint64 m_ownClickSentMs = -1; // waiting for our click to come back as synthetic
};

} // namespace ergo
