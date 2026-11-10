#pragma once

// Windows input layer (B.6): WH_KEYBOARD_LL + WH_MOUSE_LL hooks on a thread
// with its own message loop, SendInput click injection, power/session
// notifications through a hidden window.

#include "platform/PlatformInterfaces.h"

#include <atomic>
#include <thread>

namespace ergo {

class WinInputHook final : public IInputHook {
public:
    ~WinInputHook() override;

    PlatformResult start(InputEventSink sink, KeySuppressionRegistry *suppression) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool canSuppressKeys() const override { return true; }
    bool canSuppressMouseClicks() const override { return true; }
    QString backendName() const override { return QStringLiteral("WH_KEYBOARD_LL + WH_MOUSE_LL"); }

    // Windows silently removes low-level hooks whose callback was too slow and
    // gives no notification. The health check therefore re-installs ("re-arms")
    // both hooks on the hook thread every time it runs.
    bool checkHealth() override;
    int selfRepairCount() const override { return m_rearms.load(); }

    PlatformResult registerKey(const KeyCombo &combo) override;
    PlatformResult unregisterKey(const KeyCombo &combo) override;

private:
    std::thread m_thread;
    std::atomic<unsigned long> m_threadId{0};
    std::atomic<bool> m_running{false};
    std::atomic<int> m_rearms{0};
};

class WinClickInjector final : public IClickInjector {
public:
    PlatformResult click(MouseButton button) override;
    PlatformResult doubleClick(MouseButton button) override;
    PlatformResult press(MouseButton button) override;
    PlatformResult release(MouseButton button) override;
    int doubleClickIntervalMs() const override;
};

class WinPowerEvents final : public IPowerEvents {
public:
    ~WinPowerEvents() override;
    PlatformResult start(std::function<void(PowerEvent)> callback) override;
    void stop() override;

private:
    std::thread m_thread;
    std::atomic<void *> m_window{nullptr};
};

// Windows virtual-key code -> Qt::Key (keypad set for numeric keypad keys).
int winVirtualKeyToQtKey(unsigned int vk, bool extended, bool *keypad);

} // namespace ergo
