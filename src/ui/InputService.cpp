#include "ui/InputService.h"

#include "core/LogCategories.h"
#include "platform/PlatformFactory.h"
#include "ui/FeatureAvailabilityService.h"

#include <QCoreApplication>
#include <QScopeGuard>

namespace ergo {

namespace {
const QString kHookRequirement = QStringLiteral("input-hook");
}

InputService::InputService(Platform &platform, FeatureAvailabilityService &availability, QObject *parent)
    : QObject(parent)
    , m_platform(platform)
    , m_availability(availability)
    , m_watchdog(m_clock)
{
    connect(&m_dispatcher, &InputDispatcher::inputEvent, this, &InputService::onEvent);
    m_watchdogTimer.setInterval(m_watchdog.config().checkIntervalMs);
    connect(&m_watchdogTimer, &QTimer::timeout, this, &InputService::runHealthCheck);
    // Start the hook as soon as the user grants the missing permissions.
    connect(&m_availability, &FeatureAvailabilityService::changed, this, [this] {
        if (m_started && !m_platform.inputHook->isRunning() && !m_availability.anyOsPermissionMissing())
            tryStartHook("permissions changed");
    });
    m_secondTimer.start();
}

InputService::~InputService()
{
    stop();
}

void InputService::start()
{
    if (m_started)
        return;
    m_started = true;

    const PlatformResult power = m_platform.powerEvents->start([this](PowerEvent event) {
        // May be called on any thread: continue on the GUI thread.
        QMetaObject::invokeMethod(this, [this, event] { onPowerEvent(event); }, Qt::QueuedConnection);
    });
    if (!power.ok())
        qCWarning(lcPlatform).noquote() << "Sleep/lock notifications unavailable:" << power.message;

    if (m_availability.anyOsPermissionMissing())
        qCInfo(lcPlatform) << "Input hook waits for the missing OS permissions";
    else
        tryStartHook("application start");
    m_watchdogTimer.start();
}

void InputService::stop()
{
    if (!m_started)
        return;
    m_watchdogTimer.stop();
    m_platform.powerEvents->stop();
    m_platform.inputHook->stop();
    m_started = false;
    qCInfo(lcPlatform) << "Input layer stopped";
}

bool InputService::tryStartHook(const char *reason)
{
    if (m_starting)
        return false;
    m_starting = true;
    const auto done = qScopeGuard([this] { m_starting = false; });
    InputDispatcher *dispatcher = &m_dispatcher;
    const PlatformResult r = m_platform.inputHook->start([dispatcher](const InputEvent &e) { dispatcher->post(e); },
                                                         &m_suppression);
    if (r.ok()) {
        qCInfo(lcPlatform).noquote() << "Input hook running (" << reason << ") -" << m_platform.inputHook->backendName();
        // Re-apply OS-level key registrations (X11 grabs) after a restart.
        for (const KeyCombo &combo : m_suppression.combos()) {
            if (const PlatformResult k = m_platform.inputHook->registerKey(combo); !k.ok())
                qCWarning(lcPlatform).noquote() << "Key" << combo.toString() << "not re-registered:" << k.message;
        }
        updateAvailability({});
    } else {
        qCWarning(lcPlatform).noquote() << "Input hook failed to start (" << reason << "):" << r.message;
        updateAvailability(r.message);
    }
    emit statusChanged();
    return r.ok();
}

void InputService::reinstall(const char *reason)
{
    qCInfo(lcPlatform) << "Reinstalling the input hook:" << reason;
    m_platform.inputHook->stop();
    const bool ok = !m_availability.anyOsPermissionMissing() && tryStartHook(reason);
    m_watchdog.onReinstallResult(ok);
    if (!ok)
        qCWarning(lcPlatform) << "Input hook reinstall failed; consecutive failures:" << m_watchdog.consecutiveFailures();
    if (m_watchdog.persistentFailure()) {
        qCCritical(lcPlatform) << "Input hook keeps failing after" << m_watchdog.consecutiveFailures() << "attempts";
        updateAvailability(m_lastError.isEmpty() ? tr("The input hook keeps failing.") : m_lastError);
    }
    emit statusChanged();
}

void InputService::runHealthCheck()
{
    if (!m_started || m_availability.anyOsPermissionMissing())
        return;
    const bool healthy = m_platform.inputHook->isRunning() && m_platform.inputHook->checkHealth();
    const int repairs = m_platform.inputHook->selfRepairCount();
    if (repairs != m_lastSelfRepairs) {
        // e.g. macOS re-enabled a tap that the system had disabled
        qCInfo(lcPlatform) << "Input hook self-repairs:" << repairs;
        m_lastSelfRepairs = repairs;
    }
    qCDebug(lcPlatform) << "Input hook health check:" << (healthy ? "healthy" : "NOT healthy");
    if (m_watchdog.onHealthCheck(healthy) == HookWatchdog::Action::Reinstall)
        reinstall("health check failed");
    emit statusChanged();
}

void InputService::onPowerEvent(PowerEvent event)
{
    emit powerEventOccurred(int(event)); // Break Timer: sleep/lock count as idle
    switch (event) {
    case PowerEvent::Sleep:
        qCInfo(lcPlatform) << "System is going to sleep";
        break;
    case PowerEvent::ScreenLocked:
        qCInfo(lcPlatform) << "Screen locked";
        break;
    case PowerEvent::Resume:
        qCInfo(lcPlatform) << "System resumed from sleep";
        if (m_started && m_watchdog.onResume() == HookWatchdog::Action::Reinstall)
            reinstall("resume from sleep");
        break;
    case PowerEvent::ScreenUnlocked:
        qCInfo(lcPlatform) << "Screen unlocked";
        if (m_started && m_watchdog.onUnlock() == HookWatchdog::Action::Reinstall)
            reinstall("session unlock");
        break;
    }
}

void InputService::updateAvailability(const QString &error)
{
    m_lastError = error;
    // A missing OS permission already explains the problem; report the hook
    // itself only when the permissions are fine but it still does not run.
    if (error.isEmpty() || m_availability.anyOsPermissionMissing()) {
        m_availability.clearRuntimeRequirement(kHookRequirement);
        return;
    }
    PermissionState hook;
    hook.id = kHookRequirement;
    hook.displayName = tr("Global input hook");
    hook.granted = false;
    hook.explanation = error;
    hook.howToGrant = tr("Restart ErgoPro2. If the problem persists, check the log (General tab › Open log folder).");
    hook.requiredFor = allFeatures();
    m_availability.setRuntimeRequirement(hook);
}

void InputService::onEvent(const InputEvent &e)
{
    ++m_eventsThisSecond;
    if (m_secondTimer.elapsed() >= 1000) {
        m_stats.eventsPerSecond = int(m_eventsThisSecond * 1000 / m_secondTimer.restart());
        m_eventsThisSecond = 0;
    }
    if (e.type == InputEventType::MouseMove || e.isMouseButton()) {
        m_stats.pointer = e.position;
        m_stats.pointerKnown = e.positionKnown;
        if (e.screen >= 0)
            m_stats.screen = e.screen;
    }
    if (e.isKey())
        m_stats.lastKey = describeEvent(e);
    if (e.isMouseButton())
        m_stats.lastButton = describeEvent(e);
    m_stats.lastEventSynthetic = e.synthetic;
    if (e.synthetic) {
        m_stats.lastSynthetic = describeEvent(e);
        if (e.type == InputEventType::MouseDown)
            emit syntheticClickSeen(e);
    }
}

InputService::Stats InputService::stats() const
{
    Stats s = m_stats;
    s.running = m_platform.inputHook->isRunning();
    s.canSuppressKeys = s.running && m_platform.inputHook->canSuppressKeys();
    s.backend = m_platform.inputHook->backendName();
    s.lastError = m_lastError;
    s.dropped = m_dispatcher.droppedCount();
    s.swallowed = m_suppression.swallowedCount();
    s.watchdogReinstalls = m_watchdog.reinstallCount();
    s.selfRepairs = m_platform.inputHook->selfRepairCount();
    s.persistentFailure = m_watchdog.persistentFailure();
    return s;
}

PlatformResult InputService::registerKey(const KeyCombo &combo)
{
    if (m_suppression.contains(combo))
        return PlatformResult::failed(tr("%1 is already registered.").arg(combo.toString()));
    const PlatformResult r = m_platform.inputHook->registerKey(combo);
    if (!r.ok()) {
        qCWarning(lcPlatform).noquote() << "Key" << combo.toString() << "not registered:" << r.message;
        return r;
    }
    m_suppression.add(combo);
    qCInfo(lcPlatform).noquote() << "Key registered (blocked for other apps):" << combo.toString();
    return r;
}

PlatformResult InputService::unregisterKey(const KeyCombo &combo)
{
    m_suppression.remove(combo);
    const PlatformResult r = m_platform.inputHook->unregisterKey(combo);
    if (r.ok())
        qCInfo(lcPlatform).noquote() << "Key released:" << combo.toString();
    else
        qCWarning(lcPlatform).noquote() << "Key" << combo.toString() << "not released:" << r.message;
    return r;
}

PlatformResult InputService::injectLeftClick()
{
    const PlatformResult r = m_platform.clickInjector->click(MouseButton::Left);
    if (r.ok())
        qCInfo(lcPlatform) << "Synthetic left click injected";
    else
        qCWarning(lcPlatform).noquote() << "Click injection failed:" << r.message;
    return r;
}

} // namespace ergo
