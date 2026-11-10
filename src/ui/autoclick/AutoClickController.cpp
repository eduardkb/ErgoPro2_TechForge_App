#include "ui/autoclick/AutoClickController.h"

#include "core/LogCategories.h"
#include "core/settings/AutoClickSettings.h"
#include "core/settings/SettingsStore.h"
#include "platform/PlatformFactory.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/InputService.h"
#include "ui/autoclick/AutoClickOverlay.h"
#include "ui/autoclick/AutoClickWorker.h"

namespace ergo {

AutoClickController::AutoClickController(SettingsStore &settings, FeatureAvailabilityService &availability,
                                         InputService &input, Platform &platform, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_availability(availability)
    , m_input(input)
    , m_platform(platform)
{
    m_thread.setObjectName(QStringLiteral("autoclick"));
    m_worker = new AutoClickWorker(*m_platform.clickInjector);
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(&m_thread, &QThread::started, m_worker, &AutoClickWorker::initialize);

    // Events go straight from the dispatcher to the worker thread (queued).
    connect(&m_input.dispatcher(), &InputDispatcher::inputEvent, m_worker, &AutoClickWorker::onInputEvent);

    connect(m_worker, &AutoClickWorker::stateChanged, this, [this](int state, int reason) {
        m_state = state;
        m_reason = reason;
        emit stateChanged(state, reason);
    });
    connect(m_worker, &AutoClickWorker::clickFailed, this, &AutoClickController::clickFailed);
    connect(m_worker, &AutoClickWorker::clicked, this, &AutoClickController::clickPerformed);

    m_overlay = std::make_unique<AutoClickOverlay>();
    connect(m_worker, &AutoClickWorker::countdownChanged, m_overlay.get(), &AutoClickOverlay::setCountdown);

    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (!key.startsWith(QLatin1String("autoclick.")))
            return;
        applyConfig();
        if (key == AutoClickSettings::Enabled) {
            emit enabledChanged(isEnabled());
            updateActive();
        }
    });
    connect(&m_availability, &FeatureAvailabilityService::changed, this, &AutoClickController::updateActive);
    connect(&m_input, &InputService::statusChanged, this, &AutoClickController::updateActive);
}

AutoClickController::~AutoClickController()
{
    shutdown();
}

void AutoClickController::start()
{
    if (m_started)
        return;
    m_started = true;
    m_thread.start();
    applyConfig();
    updateActive();
}

void AutoClickController::shutdown()
{
    if (!m_thread.isRunning())
        return;
    QMetaObject::invokeMethod(m_worker, "setEnabled", Qt::BlockingQueuedConnection, Q_ARG(bool, false));
    m_thread.quit();
    m_thread.wait();
    m_overlay->hide();
    qCInfo(lcApp) << "AutoClick stopped";
}

bool AutoClickController::isEnabled() const
{
    return m_settings.boolValue(AutoClickSettings::Enabled);
}

bool AutoClickController::isAvailable() const
{
    return m_availability.status(Feature::AutoClick).available;
}

bool AutoClickController::visualFeedbackSupported() const
{
    // Wayland exposes no global pointer position and no way to place a window
    // under the pointer.
    return m_platform.permissions->sessionDescription() != QLatin1String("Wayland");
}

QString AutoClickController::visualFeedbackUnsupportedReason() const
{
    return visualFeedbackSupported()
               ? QString()
               : tr("Not available on Wayland: the pointer position cannot be read and windows cannot be placed "
                    "under the pointer.");
}

void AutoClickController::setEnabled(bool enabled)
{
    if (!m_settings.setValue(AutoClickSettings::Enabled, enabled))
        qCWarning(lcApp) << "AutoClick enabled setting rejected";
}

void AutoClickController::applyConfig()
{
    const int trigger = m_settings.intValue(AutoClickSettings::TriggerTimerMs);
    const int distance = m_settings.intValue(AutoClickSettings::TriggerDistancePx);
    const int jitter = m_settings.intValue(AutoClickSettings::JitterTolerancePx);
    const bool feedback = m_settings.boolValue(AutoClickSettings::ShowVisualFeedback) && visualFeedbackSupported();
    QMetaObject::invokeMethod(m_worker, [w = m_worker, trigger, distance, jitter, feedback] {
        w->setConfig(trigger, distance, jitter);
        w->setFeedbackEnabled(feedback);
    });
    if (!feedback)
        m_overlay->hide();
}

void AutoClickController::updateActive()
{
    if (!m_started)
        return;
    const bool active = isEnabled() && isAvailable() && m_input.stats().running;
    if (active == m_active)
        return;
    m_active = active;
    QMetaObject::invokeMethod(m_worker, [w = m_worker, active] { w->setEnabled(active); });
    if (!active)
        m_overlay->hide();
    qCInfo(lcApp) << "AutoClick is" << (active ? "ACTIVE" : "INACTIVE") << "(enabled:" << isEnabled()
                  << ", available:" << isAvailable() << ", hook running:" << m_input.stats().running << ")";
    emit activeChanged(active);
}

void AutoClickController::notifyExternalClick(const QPointF &position)
{
    QMetaObject::invokeMethod(m_worker, [w = m_worker, position] { w->externalClick(position); });
}

void AutoClickController::setDragLockActive(bool active, const QPointF &position)
{
    QMetaObject::invokeMethod(m_worker, [w = m_worker, active, position] { w->setDragLockActive(active, position); });
}

} // namespace ergo
