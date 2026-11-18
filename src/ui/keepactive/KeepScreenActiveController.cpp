#include "ui/keepactive/KeepScreenActiveController.h"

#include "core/LogCategories.h"
#include "core/input/InputEvent.h"
#include "core/keycontrol/KeyAction.h"
#include "core/settings/KeepScreenActiveSettings.h"
#include "core/settings/SettingsStore.h"
#include "platform/PlatformFactory.h"
#include "ui/InputService.h"
#include "ui/breaktimer/BreakTimerController.h"
#include "ui/keycontrol/DragLockController.h"
#include "ui/keycontrol/KeyControlController.h"

#include <array>

namespace ergo {

namespace {

namespace KS = KeepScreenActiveSettings;
using Engine = KeepScreenActiveEngine;

constexpr int kTickMs = 1000;
// Time for a restored / raised window to appear before its frame is read.
constexpr int kRaiseSettleMs = 600;
constexpr std::array<KeepActiveApp, 2> kApps{KeepActiveApp::Teams, KeepActiveApp::Zoom};

const char *appName(KeepActiveApp app)
{
    return app == KeepActiveApp::Teams ? "Teams" : "Zoom";
}

const char *stateName(Engine::State s)
{
    switch (s) {
    case Engine::State::Disabled: return "Disabled";
    case Engine::State::Idle: return "Idle";
    case Engine::State::Working: return "Working";
    }
    return "?";
}

} // namespace

KeepScreenActiveController::KeepScreenActiveController(SettingsStore &settings, InputService &input,
                                                       KeyControlController &keycontrol,
                                                       BreakTimerController &breaktimer, Platform &platform,
                                                       QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_input(input)
    , m_keycontrol(keycontrol)
    , m_breaktimer(breaktimer)
    , m_platform(platform)
    , m_engine(m_clock)
{
    m_tick.setInterval(kTickMs);
    connect(&m_tick, &QTimer::timeout, this, &KeepScreenActiveController::onTick);
}

void KeepScreenActiveController::start()
{
    if (m_started)
        return;
    m_started = true;
    applyConfig();

    connect(&m_input.dispatcher(), &InputDispatcher::inputEvent, this, &KeepScreenActiveController::onInputEvent);
    connect(&m_input, &InputService::powerEventOccurred, this, &KeepScreenActiveController::onPowerEvent);
    connect(&m_breaktimer, &BreakTimerController::breakWindowShown, this,
            [this] { m_engine.setPaused(Engine::BreakWindow, true); notifyIfChanged(); });
    connect(&m_breaktimer, &BreakTimerController::breakWindowClosed, this,
            [this] { m_engine.setPaused(Engine::BreakWindow, false); });
    connect(&m_keycontrol.dragLock(), &DragLockController::holdingChanged, this, [this](bool holding) {
        m_engine.setPaused(Engine::DragLock, holding);
        notifyIfChanged();
    });
    connect(&m_keycontrol, &KeyControlController::keepScreenActiveToggleRequested, this,
            [this] { setEnabled(!m_engine.isEnabled(), "hotkey pressed"); });
    connect(&m_keycontrol, &KeyControlController::changed, this, &KeepScreenActiveController::syncHotkey);
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key == KS::StartAfterIdleMinutes || key == KS::IntervalMinutes)
            applyConfig();
        else if (key == KS::Enabled && m_settings.boolValue(key) != m_engine.isEnabled())
            setEnabled(m_settings.boolValue(key), "settings changed");
    });

    // Remembered across restarts (owner decision), but only with a hotkey.
    syncHotkey();
    if (m_settings.boolValue(KS::Enabled) && hasHotkey() && m_bindingEnabled)
        setEnabled(true, "restored at start");
    else if (m_settings.boolValue(KS::Enabled))
        setEnabled(false, "no active hotkey at start");
    m_tick.start();
    if (!limitation().isEmpty())
        qCInfo(lcApp).noquote() << "Keep screen active:" << limitation();
}

void KeepScreenActiveController::shutdown()
{
    m_tick.stop();
    ++m_cycleSerial; // a pending delayed step does nothing
    m_started = false;
    m_hotkeySynced = false;
}

QString KeepScreenActiveController::limitation() const
{
    return m_platform.appWindows ? m_platform.appWindows->unsupportedReason()
                                 : tr("Not available on this system.");
}

void KeepScreenActiveController::applyConfig()
{
    Engine::Config c;
    c.startAfterMs = qint64(m_settings.intValue(KS::StartAfterIdleMinutes)) * 60000;
    c.intervalMs = qint64(m_settings.intValue(KS::IntervalMinutes)) * 60000;
    m_engine.setConfig(c);
    qCInfo(lcApp) << "Keep screen active: first action after" << c.startAfterMs / 60000 << "min without input, then every"
                  << c.intervalMs / 60000 << "min";
}

void KeepScreenActiveController::setEnabled(bool enabled, const char *reason)
{
    if (enabled && (!hasHotkey() || !m_bindingEnabled)) {
        qCWarning(lcApp) << "Keep screen active cannot be enabled without an active hotkey (" << reason << ")";
        enabled = false;
    }
    if (m_settings.boolValue(KS::Enabled) != enabled)
        m_settings.setValue(KS::Enabled, enabled);
    if (m_engine.isEnabled() == enabled)
        return;
    m_engine.setEnabled(enabled);
    qCInfo(lcApp) << "Keep screen active" << (enabled ? "enabled" : "disabled") << "(" << reason << ")";
    notifyIfChanged();
    emit changed();
}

void KeepScreenActiveController::syncHotkey()
{
    QString id;
    bool bindingEnabled = false;
    QString text;
    for (const KeyBinding &b : m_keycontrol.bindings()) {
        if (b.actionId == KeyActionRegistry::ToggleKeepScreenActive && b.isValid()) {
            id = b.id;
            bindingEnabled = b.enabled;
            text = m_keycontrol.displayText(b.combo);
            break;
        }
    }
    const bool hotkeyChanged = id != m_bindingId || bindingEnabled != m_bindingEnabled || text != m_hotkeyText;
    // 1.9.1 (owner request): adding the hotkey or turning it back on in the
    // table switches the feature on. Not at start: there the remembered
    // setting decides.
    const bool becameActive =
        m_hotkeySynced && keepActiveHotkeyBecameActive(m_bindingId, m_bindingEnabled, id, bindingEnabled);
    const bool added = id != m_bindingId;
    m_bindingId = id;
    m_bindingEnabled = bindingEnabled;
    m_hotkeyText = text;
    m_hotkeySynced = true;
    if (becameActive && !m_engine.isEnabled())
        setEnabled(true, added ? "hotkey added in Key Control" : "hotkey enabled in Key Control");
    else if (m_engine.isEnabled() && id.isEmpty())
        setEnabled(false, "hotkey removed in Key Control");
    else if (m_engine.isEnabled() && !bindingEnabled)
        setEnabled(false, "hotkey disabled in Key Control");
    if (hotkeyChanged)
        emit changed();
}

void KeepScreenActiveController::onInputEvent(const InputEvent &event)
{
    if (event.synthetic)
        return; // our own clicks / moves (and the other features') never count
    m_engine.onRealInput();
    if (m_cycleRunning) {
        ++m_cycleSerial; // the user is back: a pending step is dropped
        m_cycleRunning = false;
    }
    notifyIfChanged();
}

void KeepScreenActiveController::onPowerEvent(int event)
{
    switch (PowerEvent(event)) {
    case PowerEvent::Sleep:
    case PowerEvent::ScreenLocked: m_engine.setPaused(Engine::Locked, true); break;
    case PowerEvent::Resume:
    case PowerEvent::ScreenUnlocked: m_engine.setPaused(Engine::Locked, false); break;
    }
    notifyIfChanged();
}

void KeepScreenActiveController::onTick()
{
    if (m_engine.update() && !m_cycleRunning)
        runCycle();
    notifyIfChanged();
}

void KeepScreenActiveController::notifyIfChanged()
{
    const Engine::State s = m_engine.state();
    if (s == m_lastState)
        return;
    qCInfo(lcApp) << "Keep screen active:" << stateName(m_lastState) << "->" << stateName(s);
    m_lastState = s;
    emit changed();
}

void KeepScreenActiveController::runCycle()
{
    m_cycleRunning = true;
    ++m_cycleSerial;
    qCDebug(lcApp) << "Keep screen active: cycle" << m_engine.cycleCount() << "after"
                   << m_engine.idleMs() / 1000 << "s without input";
    if (!m_platform.appWindows) {
        qCWarning(lcApp) << "Keep screen active: no platform support";
        m_cycleRunning = false;
        return;
    }
    if (!m_platform.appWindows->unsupportedReason().isEmpty()) {
        finishWithNudge(); // Wayland: other windows cannot be targeted
        return;
    }
    tryApp(0);
}

// Step 1 / 2 of a cycle (B.10): restore + raise the app window, wait until it
// is on screen, click its top-left corner once and put the pointer back.
void KeepScreenActiveController::tryApp(int index)
{
    if (index >= int(kApps.size())) {
        finishWithNudge();
        return;
    }
    const KeepActiveApp app = kApps[size_t(index)];
    QString detail;
    switch (m_platform.appWindows->raise(app, &detail)) {
    case IAppWindows::Raise::NotRunning:
        tryApp(index + 1);
        return;
    case IAppWindows::Raise::Failed:
        qCWarning(lcApp).noquote() << "Keep screen active:" << appName(app) << "window could not be restored:" << detail;
        tryApp(index + 1);
        return;
    case IAppWindows::Raise::Raised:
        break;
    }
    const int serial = m_cycleSerial;
    QTimer::singleShot(kRaiseSettleMs, this, [this, serial, index, app] {
        if (serial != m_cycleSerial)
            return; // user input or shutdown in the meantime: no click
        if (m_engine.state() != Engine::State::Working) {
            m_cycleRunning = false; // paused in the meantime (lock, break window, drag lock)
            return;
        }
        const std::optional<QRectF> frame = m_platform.appWindows->mainWindowFrame(app);
        QPointF point;
        if (!frame || !keepActiveClickPoint(*frame, &point)) {
            qCWarning(lcApp) << "Keep screen active:" << appName(app) << "window not visible"
                             << (frame ? *frame : QRectF());
            tryApp(index + 1);
            return;
        }
        const PlatformResult r = m_platform.appWindows->clickAt(point);
        if (!r.ok()) {
            qCWarning(lcApp).noquote() << "Keep screen active: click on the" << appName(app)
                                       << "window corner failed:" << r.message;
            tryApp(index + 1);
            return;
        }
        qCDebug(lcApp) << "Keep screen active: clicked the" << appName(app) << "window corner at" << point << "frame"
                       << *frame;
        m_cycleRunning = false;
    });
}

// Step 3: neither app (or Wayland): 1 px pointer move and back, no click.
void KeepScreenActiveController::finishWithNudge()
{
    const PlatformResult r = m_platform.appWindows->nudgePointer();
    if (r.ok())
        qCDebug(lcApp) << "Keep screen active: pointer nudged";
    else
        qCWarning(lcApp).noquote() << "Keep screen active: pointer nudge failed:" << r.message;
    m_cycleRunning = false;
}

} // namespace ergo
