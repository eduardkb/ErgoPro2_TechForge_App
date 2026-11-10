#include "core/input/InputDispatcher.h"

#include <QMetaObject>
#include <QMutexLocker>

namespace ergo {

InputDispatcher::InputDispatcher(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<ergo::InputEvent>();
    m_pending.reserve(256);
}

void InputDispatcher::post(const InputEvent &event)
{
    bool schedule = false;
    {
        QMutexLocker lock(&m_mutex);
        if (m_pending.size() >= kMaxPending) {
            m_dropped.fetch_add(1);
            return;
        }
        m_pending.append(event);
        if (!m_drainScheduled) {
            m_drainScheduled = true;
            schedule = true;
        }
    }
    if (schedule)
        QMetaObject::invokeMethod(this, &InputDispatcher::drain, Qt::QueuedConnection);
}

void InputDispatcher::drain()
{
    QVector<InputEvent> batch;
    {
        QMutexLocker lock(&m_mutex);
        batch.swap(m_pending);
        m_pending.reserve(256);
        m_drainScheduled = false;
    }
    for (const InputEvent &event : std::as_const(batch)) {
        m_delivered.fetch_add(1);
        emit inputEvent(event);
    }
}

} // namespace ergo
