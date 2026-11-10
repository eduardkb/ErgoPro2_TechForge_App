#pragma once

#include "core/input/InputEvent.h"

#include <QMutex>
#include <QObject>
#include <QVector>

#include <atomic>

namespace ergo {

// Moves input events from the platform hook thread to the thread this object
// lives in, and re-emits them as inputEvent() (B.6). Subscribers in other
// threads connect with queued connections.
//
// post() never blocks for long: it appends to a buffer under a short mutex and
// schedules one drain per batch. The buffer is bounded; when the receiving
// thread cannot keep up, new events are dropped and counted.
class InputDispatcher : public QObject {
    Q_OBJECT
public:
    static constexpr int kMaxPending = 10000;

    explicit InputDispatcher(QObject *parent = nullptr);

    // Thread-safe; callable from the hook thread.
    void post(const InputEvent &event);

    quint64 deliveredCount() const { return m_delivered.load(); }
    quint64 droppedCount() const { return m_dropped.load(); }

signals:
    void inputEvent(const ergo::InputEvent &event);

private:
    void drain();

    QMutex m_mutex;
    QVector<InputEvent> m_pending;
    bool m_drainScheduled = false;
    std::atomic<quint64> m_delivered{0};
    std::atomic<quint64> m_dropped{0};
};

} // namespace ergo
