#include "ui/autoclick/AutoClickWorker.h"

#include "core/LogCategories.h"
#include "platform/PlatformInterfaces.h"

#include <QTimer>

namespace ergo {

AutoClickWorker::AutoClickWorker(IClickInjector &injector)
    : m_injector(injector)
    , m_engine(m_clock)
{
}

void AutoClickWorker::initialize()
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &AutoClickWorker::onTimeout);
}

void AutoClickWorker::setEnabled(bool enabled)
{
    if (enabled == m_engine.isEnabled())
        return;
    m_engine.setEnabled(enabled);
    qCInfo(lcApp) << "AutoClick engine" << (enabled ? "enabled" : "disabled");
    schedule();
}

void AutoClickWorker::setConfig(int triggerMs, int distancePx, int jitterPx)
{
    AutoClickEngine::Config c = m_engine.config();
    c.triggerMs = triggerMs;
    c.distancePx = distancePx;
    c.jitterPx = jitterPx;
    m_engine.setConfig(c);
    qCDebug(lcApp) << "AutoClick config: trigger" << triggerMs << "ms, distance" << distancePx << "px, jitter"
                   << jitterPx << "px";
    schedule(); // a changed trigger time moves the pending deadline
}

void AutoClickWorker::setFeedbackEnabled(bool enabled)
{
    m_feedback = enabled;
    m_lastDeadline = -2;
    schedule();
}

void AutoClickWorker::onInputEvent(const InputEvent &event)
{
    if (event.synthetic) {
        // Our own click coming back from the hook (B.6): confirms the injection.
        if (event.type == InputEventType::MouseDown && m_ownClickSentMs >= 0) {
            qCDebug(lcApp) << "AutoClick: own click confirmed by the hook after"
                           << (m_clock.monotonicMs() - m_ownClickSentMs) << "ms";
            m_ownClickSentMs = -1;
        }
        return;
    }
    if (event.type == InputEventType::MouseMove)
        m_positionKnown = event.positionKnown;
    m_engine.onInputEvent(event);
    schedule();
}

void AutoClickWorker::externalClick(const QPointF &position)
{
    m_engine.onExternalClick(position);
    schedule();
}

void AutoClickWorker::setDragLockActive(bool active, const QPointF &position)
{
    m_engine.setDragLockActive(active, position);
    schedule();
}

void AutoClickWorker::schedule()
{
    if (!m_timer)
        return;
    const qint64 deadline = m_engine.nextDeadlineMs();
    if (deadline < 0) {
        m_timer->stop();
    } else {
        const qint64 wait = std::max<qint64>(0, deadline - m_clock.monotonicMs());
        m_timer->start(int(wait));
    }
    publish();
}

void AutoClickWorker::onTimeout()
{
    if (m_engine.poll()) {
        const QPointF position = m_engine.position();
        const PlatformResult r = m_injector.click(MouseButton::Left);
        if (r.ok()) {
            m_ownClickSentMs = m_clock.monotonicMs();
            qCInfo(lcApp).noquote() << "AutoClick: left click at" << qRound(position.x()) << ","
                                    << qRound(position.y());
            emit clicked(position);
        } else {
            qCWarning(lcApp).noquote() << "AutoClick: click failed:" << r.message;
            emit clickFailed(r.message);
        }
    }
    schedule();
}

void AutoClickWorker::publish()
{
    const int state = int(m_engine.state());
    const int reason = int(m_engine.suppressReason());
    if (state != m_lastState || reason != m_lastReason) {
        m_lastState = state;
        m_lastReason = reason;
        emit stateChanged(state, reason);
    }
    if (!m_feedback)
        return;
    const qint64 deadline = m_engine.nextDeadlineMs();
    if (deadline == m_lastDeadline)
        return;
    m_lastDeadline = deadline;
    emit countdownChanged(deadline >= 0, m_engine.position(), m_positionKnown, deadline, m_engine.config().triggerMs);
}

} // namespace ergo
