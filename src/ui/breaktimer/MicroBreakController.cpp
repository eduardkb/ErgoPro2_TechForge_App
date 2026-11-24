#include "ui/breaktimer/MicroBreakController.h"

#include "core/LogCategories.h"
#include "core/settings/BreakTimerSettings.h"
#include "core/settings/MicroBreakSettings.h"
#include "core/settings/SettingsStore.h"
#include "platform/PlatformInterfaces.h"
#include "ui/InputService.h"
#include "ui/breaktimer/BreakTimerController.h"
#include "ui/breaktimer/MicroBreakWindow.h"

namespace ergo {

namespace MB = MicroBreakSettings;

namespace {

const char *stateName(MicroBreakEngine::State s)
{
    switch (s) {
    case MicroBreakEngine::State::Disabled: return "Disabled";
    case MicroBreakEngine::State::Waiting: return "Idle";
    case MicroBreakEngine::State::Working: return "Counting";
    case MicroBreakEngine::State::Shown: return "Micro-break shown";
    }
    return "?";
}

} // namespace

MicroBreakController::MicroBreakController(SettingsStore &settings, InputService &input,
                                           BreakTimerController &breakTimer, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_input(input)
    , m_breakTimer(breakTimer)
    , m_engine(m_clock)
{
    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, &MicroBreakController::onTick);
}

MicroBreakController::~MicroBreakController()
{
    shutdown();
}

void MicroBreakController::start()
{
    if (m_started)
        return;
    m_started = true;
    m_window = std::make_unique<MicroBreakWindow>();
    connect(m_window.get(), &MicroBreakWindow::clicked, this, [this] {
        qCInfo(lcApp) << "Micro-break dismissed by the user";
        m_dismissedByUser = true;
        m_engine.dismiss();
        sync();
    });
    connect(&m_input.dispatcher(), &InputDispatcher::inputEvent, this, &MicroBreakController::onInputEvent);
    connect(&m_input, &InputService::powerEventOccurred, this, &MicroBreakController::onPowerEvent);
    connect(&m_breakTimer, &BreakTimerController::availabilityChanged, this, &MicroBreakController::updateEnabled);
    // A normal break, its Dismiss or an idle reset of the Break Timer restarts the count.
    connect(&m_breakTimer, &BreakTimerController::workCycleReset, this, [this] {
        if (m_engine.state() == MicroBreakEngine::State::Working)
            qCDebug(lcApp) << "Micro-breaks: count restarted with the Break Timer cycle at"
                           << m_engine.activeMs() / 1000 << "s";
        m_engine.restart();
        sync();
    });
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key == MB::Enabled)
            updateEnabled();
        else if (key.startsWith(QLatin1String("microBreaks.")) || key == BreakTimerSettings::ActivityPauseS)
            applyConfig();
    });
    applyConfig();
    updateEnabled();
    m_tick.start();
}

void MicroBreakController::shutdown()
{
    if (!m_started)
        return;
    m_tick.stop();
    if (m_window)
        m_window->hide();
    m_started = false;
}

bool MicroBreakController::isEnabled() const
{
    return m_settings.boolValue(MB::Enabled);
}

bool MicroBreakController::isAvailable() const
{
    return m_breakTimer.isAvailable();
}

void MicroBreakController::setEnabled(bool enabled)
{
    if (!m_settings.setValue(MB::Enabled, enabled))
        qCWarning(lcApp) << "Micro-breaks enabled setting rejected";
}

void MicroBreakController::applyConfig()
{
    MicroBreakEngine::Config c;
    c.intervalMs = qint64(m_settings.intValue(MB::IntervalMinutes)) * 60000;
    c.showMs = qint64(m_settings.intValue(MB::ShowSeconds)) * 1000;
    c.pauseAfterMs = qint64(m_settings.intValue(BreakTimerSettings::ActivityPauseS)) * 1000;
    m_engine.setConfig(c);
    qCInfo(lcApp) << "Micro-breaks config: every" << c.intervalMs / 60000 << "min of active use, shown for"
                  << c.showMs / 1000 << "s, active use stops after" << c.pauseAfterMs / 1000 << "s without input";
    sync();
}

void MicroBreakController::updateEnabled()
{
    if (!m_started)
        return;
    const bool run = isEnabled() && isAvailable();
    if (run != m_engine.isEnabled()) {
        m_engine.setEnabled(run);
        qCInfo(lcApp) << "Micro-breaks" << (run ? "running" : "stopped") << "(enabled:" << isEnabled()
                      << ", activity monitoring available:" << isAvailable() << ")";
    }
    emit availabilityChanged(isAvailable());
    sync();
}

void MicroBreakController::onTick()
{
    // Gives way to the Break Timer (owner decision): no micro-break while the
    // break window or the pre-break reminder is shown.
    const bool blocked =
        m_breakTimer.state() == BreakTimerEngine::State::BreakShown || m_breakTimer.isReminderShown();
    if (blocked != m_blocked) {
        m_blocked = blocked;
        qCDebug(lcApp) << "Micro-breaks:" << (blocked ? "waiting for the Break Timer" : "Break Timer done");
    }
    m_engine.setBlocked(blocked);
    m_engine.update();
    sync();
}

void MicroBreakController::onInputEvent(const InputEvent &event)
{
    // Only real input counts; key releases are not presses (same as the Break Timer).
    if (event.synthetic || event.type == InputEventType::KeyUp)
        return;
    m_engine.onActivity();
}

void MicroBreakController::onPowerEvent(int event)
{
    switch (PowerEvent(event)) {
    case PowerEvent::Sleep:
    case PowerEvent::ScreenLocked: m_engine.setLocked(true); break;
    case PowerEvent::Resume:
    case PowerEvent::ScreenUnlocked: m_engine.setLocked(false); break;
    }
    sync();
}

void MicroBreakController::sync()
{
    using S = MicroBreakEngine::State;
    const S s = m_engine.state();
    if (s != m_lastState) {
        qCInfo(lcApp) << "Micro-breaks:" << stateName(m_lastState) << "->" << stateName(s);
        if (m_lastState == S::Shown && m_window) {
            m_window->hide();
            qCInfo(lcApp) << "Micro-break window hidden ("
                          << (m_dismissedByUser ? "clicked" : m_blocked ? "Break Timer" : "time elapsed or stopped")
                          << ")";
        }
        m_dismissedByUser = false;
        m_lastState = s;
    }
    if (s == S::Shown && m_window)
        m_window->showMicroBreak(m_engine.shownRemainingMs(), m_settings.intValue(MB::ShowSeconds),
                                 m_trayGeometry ? m_trayGeometry() : QRect());
    emit ticked();
}

} // namespace ergo
