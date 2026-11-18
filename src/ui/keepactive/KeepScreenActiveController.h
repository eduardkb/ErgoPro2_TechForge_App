#pragma once

#include "core/Clock.h"
#include "core/keepactive/KeepScreenActiveEngine.h"

#include <QObject>
#include <QTimer>

namespace ergo {

class BreakTimerController;
class InputService;
class KeyControlController;
class SettingsStore;
struct InputEvent;
struct Platform;

// GUI-thread side of Keep Screen Active (B.10, stage 9): feeds the engine with
// real input, sleep/lock, the break window and the drag lock; follows the
// "Keep screen active on/off" hotkey of Key Control; runs a cycle when the
// engine says so (Teams -> Zoom -> harmless pointer nudge) through the
// platform's IAppWindows. All injected events are synthetic, so Statistics,
// Break Timer and Auto Click ignore them.
class KeepScreenActiveController : public QObject {
    Q_OBJECT
public:
    KeepScreenActiveController(SettingsStore &settings, InputService &input, KeyControlController &keycontrol,
                               BreakTimerController &breaktimer, Platform &platform, QObject *parent = nullptr);

    void start();
    void shutdown();

    bool isEnabled() const { return m_engine.isEnabled(); }
    KeepScreenActiveEngine::State state() const { return m_engine.state(); }
    // A hotkey with the action exists in Key Control (the card is shown only then).
    bool hasHotkey() const { return !m_bindingId.isEmpty(); }
    bool isHotkeyEnabled() const { return m_bindingEnabled; }
    QString hotkeyText() const { return m_hotkeyText; }
    // Non-empty when other apps' windows cannot be targeted (Wayland).
    QString limitation() const;

signals:
    void changed(); // enabled, state or hotkey changed

private:
    void applyConfig();
    void setEnabled(bool enabled, const char *reason);
    void syncHotkey();
    void onInputEvent(const InputEvent &event);
    void onPowerEvent(int event);
    void onTick();
    void runCycle();
    void tryApp(int index);
    void finishWithNudge();
    void notifyIfChanged();

    SettingsStore &m_settings;
    InputService &m_input;
    KeyControlController &m_keycontrol;
    BreakTimerController &m_breaktimer;
    Platform &m_platform;
    SystemClock m_clock;
    KeepScreenActiveEngine m_engine;
    QTimer m_tick;
    QString m_bindingId;
    bool m_bindingEnabled = false;
    QString m_hotkeyText;
    bool m_hotkeySynced = false; // the first sync (start) never switches the feature on
    bool m_cycleRunning = false;
    int m_cycleSerial = 0;
    KeepScreenActiveEngine::State m_lastState = KeepScreenActiveEngine::State::Disabled;
    bool m_started = false;
};

} // namespace ergo
