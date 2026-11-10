#pragma once

// macOS input layer (B.6): CGEventTap hook, CGEventPost click injection,
// NSWorkspace sleep/lock notifications.

#include "platform/PlatformInterfaces.h"

#include <QMutex>
#include <QSet>

#include <atomic>
#include <memory>
#include <thread>

namespace ergo {

// Session-level event tap (head insert) on a CFRunLoop in its own thread.
// Uses an active tap (can block keys; needs Accessibility) and falls back to a
// listen-only tap (Input Monitoring only; keys cannot be blocked).
// A tap disabled by macOS (timeout / user input) is re-enabled immediately.
class MacInputHook final : public IInputHook {
public:
    MacInputHook();
    ~MacInputHook() override;

    PlatformResult start(InputEventSink sink, KeySuppressionRegistry *suppression) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool canSuppressKeys() const override { return m_active.load(); }
    bool canSuppressMouseClicks() const override { return m_active.load(); }
    QString backendName() const override;
    bool checkHealth() override;
    int selfRepairCount() const override { return m_selfRepairs.load(); }
    PlatformResult registerKey(const KeyCombo &combo) override;
    PlatformResult unregisterKey(const KeyCombo &combo) override;

    struct State; // opaque CoreFoundation state, defined in MacInput.mm

private:
    std::unique_ptr<State> m_state;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_active{false};
    std::atomic<int> m_selfRepairs{0};
};

class MacClickInjector final : public IClickInjector {
public:
    PlatformResult click(MouseButton button) override;
    PlatformResult doubleClick(MouseButton button) override;
    PlatformResult press(MouseButton button) override;
    PlatformResult release(MouseButton button) override;
    int doubleClickIntervalMs() const override;
};

class MacPowerEvents final : public IPowerEvents {
public:
    ~MacPowerEvents() override;
    PlatformResult start(std::function<void(PowerEvent)> callback) override;
    void stop() override;

private:
    QList<void *> m_observers; // NSObject<NSObject> tokens (retained)
    bool m_started = false;
};

// macOS virtual key code (kVK_*) -> Qt::Key; keypad set for numeric keypad keys.
int macKeyCodeToQtKey(quint16 keyCode, bool *keypad);

} // namespace ergo
