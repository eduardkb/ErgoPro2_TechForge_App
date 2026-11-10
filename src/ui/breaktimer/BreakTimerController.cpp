#include "ui/breaktimer/BreakTimerController.h"

#include "core/LogCategories.h"
#include "core/settings/BreakTimerSettings.h"
#include "core/settings/SettingsStore.h"
#include "platform/PlatformFactory.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/InputService.h"
#include "ui/breaktimer/BreakWindowManager.h"

namespace ergo {

namespace BT = BreakTimerSettings;

namespace {

const char *stateName(BreakTimerEngine::State s)
{
    switch (s) {
    case BreakTimerEngine::State::Disabled: return "Disabled";
    case BreakTimerEngine::State::WaitingForActivity: return "Idle";
    case BreakTimerEngine::State::Working: return "Working";
    case BreakTimerEngine::State::BreakShown: return "On break";
    case BreakTimerEngine::State::Postponed: return "Postponed";
    }
    return "?";
}

} // namespace

BreakTimerController::BreakTimerController(SettingsStore &settings, FeatureAvailabilityService &availability,
                                           InputService &input, Platform &platform, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_availability(availability)
    , m_input(input)
    , m_platform(platform)
    , m_engine(m_clock)
{
    m_engine.setEnabled(false); // until start() checks availability
    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, &BreakTimerController::onTick);
}

BreakTimerController::~BreakTimerController()
{
    shutdown();
}

void BreakTimerController::start()
{
    if (m_started)
        return;
    m_started = true;
    m_windows = std::make_unique<BreakWindowManager>(*m_platform.desktop);
    connect(m_windows.get(), &BreakWindowManager::dismissClicked, this, &BreakTimerController::dismiss);
    connect(m_windows.get(), &BreakWindowManager::postponeClicked, this, &BreakTimerController::postpone);

    connect(&m_input.dispatcher(), &InputDispatcher::inputEvent, this, &BreakTimerController::onInputEvent);
    connect(&m_input, &InputService::powerEventOccurred, this, &BreakTimerController::onPowerEvent);
    connect(&m_input, &InputService::statusChanged, this, &BreakTimerController::updateEnabled);
    connect(&m_availability, &FeatureAvailabilityService::changed, this, &BreakTimerController::updateEnabled);
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (!key.startsWith(QLatin1String("breaktimer.")))
            return;
        if (key == BT::Enabled) {
            emit enabledChanged(isEnabled());
            updateEnabled();
        } else {
            applyConfig();
        }
    });
    applyConfig();
    updateEnabled();
    m_tick.start();
}

void BreakTimerController::shutdown()
{
    if (!m_started)
        return;
    m_tick.stop();
    if (m_windows)
        m_windows->hideAll();
    m_started = false;
}

bool BreakTimerController::isEnabled() const
{
    return m_settings.boolValue(BT::Enabled);
}

bool BreakTimerController::isAvailable() const
{
    return m_availability.status(Feature::BreakTimerActivity).available && m_input.stats().running;
}

bool BreakTimerController::fullscreenDetectionSupported() const
{
    return m_platform.desktop->isFullscreenAppActive().has_value();
}

void BreakTimerController::setEnabled(bool enabled)
{
    if (!m_settings.setValue(BT::Enabled, enabled))
        qCWarning(lcApp) << "Break Timer enabled setting rejected";
}

void BreakTimerController::applyConfig()
{
    BreakTimerEngine::Config c;
    c.idleResetMs = qint64(m_settings.intValue(BT::IdleResetMin)) * 60000;
    c.maxWorkMs = qint64(m_settings.intValue(BT::MaxWorkMin)) * 60000;
    c.breakMs = qint64(m_settings.intValue(BT::BreakMin)) * 60000;
    c.postponeMs = qint64(m_settings.intValue(BT::PostponeMin)) * 60000;
    c.pauseAfterMs = qint64(m_settings.intValue(BT::ActivityPauseS)) * 1000;
    c.suppressWhenFullscreen = m_settings.boolValue(BT::SuppressWhenFullscreen) && fullscreenDetectionSupported();
    m_engine.setConfig(c); // applies immediately, also to the running cycle
    qCDebug(lcApp) << "Break Timer config: idle reset" << c.idleResetMs / 60000 << "min, max work"
                   << c.maxWorkMs / 60000 << "min, break" << c.breakMs / 60000 << "min, postpone"
                   << c.postponeMs / 60000 << "min, work time stops after" << c.pauseAfterMs / 1000
                   << "s without input, full-screen suppression" << c.suppressWhenFullscreen;
    if (m_windows)
        m_windows->setPostponeMinutes(m_settings.intValue(BT::PostponeMin));
    sync();
}

void BreakTimerController::updateEnabled()
{
    if (!m_started)
        return;
    // Without the input hook no activity can be measured: the timer cannot run.
    const bool available = isAvailable();
    if (available != m_lastAvailable) {
        m_lastAvailable = available;
        emit availabilityChanged(available); // the tab enables/disables its controls
    }
    const bool run = isEnabled() && available;
    if (run != m_engine.isEnabled()) {
        m_engine.setEnabled(run);
        qCInfo(lcApp) << "Break Timer" << (run ? "running" : "stopped") << "(enabled:" << isEnabled()
                      << ", activity monitoring available:" << isAvailable() << ")";
    }
    sync();
}

void BreakTimerController::onTick()
{
    if (m_engine.config().suppressWhenFullscreen)
        m_engine.setFullscreenActive(m_platform.desktop->isFullscreenAppActive().value_or(false));
    m_engine.update();
    sync();
}

void BreakTimerController::onInputEvent(const InputEvent &event)
{
    // Only real user input counts as activity (B-02); key releases are not presses.
    if (event.synthetic || event.type == InputEventType::KeyUp)
        return;
    m_engine.onActivity();
}

void BreakTimerController::onPowerEvent(int event)
{
    switch (PowerEvent(event)) {
    case PowerEvent::Sleep: m_engine.onSleep(); break;
    case PowerEvent::Resume: m_engine.onResume(); break;
    case PowerEvent::ScreenLocked: m_engine.onScreenLocked(); break;
    case PowerEvent::ScreenUnlocked: m_engine.onScreenUnlocked(); break;
    }
    sync();
}

void BreakTimerController::dismiss()
{
    qCInfo(lcApp) << "Break dismissed by the user";
    m_engine.dismiss();
    sync();
}

void BreakTimerController::postpone()
{
    qCInfo(lcApp) << "Break postponed by" << m_settings.intValue(BT::PostponeMin) << "min";
    m_engine.postpone();
    sync();
}

void BreakTimerController::startBreakNow()
{
    qCInfo(lcApp) << "Break started from the tray menu";
    m_engine.startBreakNow();
    sync();
}

void BreakTimerController::sync()
{
    const BreakTimerEngine::State s = m_engine.state();
    if (s != m_lastState) {
        qCInfo(lcApp) << "Break Timer:" << stateName(m_lastState) << "->" << stateName(s)
                      << "(work time" << m_engine.interactionMs() / 1000 << "s)";
        const bool wasBreak = m_lastState == BreakTimerEngine::State::BreakShown;
        m_lastState = s;
        if (s == BreakTimerEngine::State::BreakShown && m_windows) {
            m_windows->setPostponeMinutes(m_settings.intValue(BT::PostponeMin));
            m_windows->showAll(m_engine.breakRemainingMs());
            emit breakWindowShown();
        } else if (wasBreak && m_windows) {
            m_windows->hideAll();
            emit breakWindowClosed();
        }
    }
    if (s == BreakTimerEngine::State::BreakShown && m_windows)
        m_windows->setRemaining(m_engine.breakRemainingMs());
    emit ticked();
}

} // namespace ergo
