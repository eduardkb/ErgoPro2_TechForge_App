#pragma once

// Linux input layer (B.6).
//   X11:     XRecord monitoring, XGrabKey exclusive keys, XTest click injection.
//   Wayland: best effort with libevdev (/dev/input/event*) and uinput; only when
//            ErgoPro2 was built with libevdev (ERGOPRO2_HAVE_LIBEVDEV).
//   Power:   logind PrepareForSleep + session Lock/Unlock and the desktop
//            screensaver over D-Bus (ERGOPRO2_HAVE_QTDBUS).

#include "platform/PlatformInterfaces.h"
#include "platform/common/Synthetic.h"

#include <QMutex>
#include <QSet>

#include <atomic>
#include <memory>
#include <thread>

namespace ergo {

// ------------------------------------------------------------------ X11

class X11InputHook final : public IInputHook {
public:
    explicit X11InputHook(SyntheticTracker *tracker);
    ~X11InputHook() override;

    PlatformResult start(InputEventSink sink, KeySuppressionRegistry *suppression) override;
    void stop() override;
    bool isRunning() const override { return m_running.load(); }
    bool canSuppressKeys() const override { return true; }
    bool canSuppressMouseClicks() const override { return false; } // XRecord only observes
    QString backendName() const override { return QStringLiteral("X11 XRecord + XGrabKey"); }
    bool checkHealth() override;
    int selfRepairCount() const override { return m_regrabs.load(); }
    PlatformResult registerKey(const KeyCombo &combo) override;
    PlatformResult unregisterKey(const KeyCombo &combo) override;

    struct Impl; // Xlib state, defined in LinuxX11Input.cpp

private:
    std::unique_ptr<Impl> m_impl;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<int> m_regrabs{0};
};

class X11ClickInjector final : public IClickInjector {
public:
    explicit X11ClickInjector(SyntheticTracker *tracker) : m_tracker(tracker) {}
    ~X11ClickInjector() override;
    PlatformResult click(MouseButton button) override;
    PlatformResult doubleClick(MouseButton button) override;
    PlatformResult press(MouseButton button) override;
    PlatformResult release(MouseButton button) override;
    int doubleClickIntervalMs() const override { return 400; } // X11 has no standard setting

private:
    PlatformResult send(MouseButton button, std::initializer_list<bool> presses);
    SyntheticTracker *m_tracker;
    void *m_display = nullptr; // Display*, opened on first use
    QMutex m_mutex;
};

// X11 keysym <-> Qt::Key
int x11KeysymToQtKey(unsigned long keysym, bool *keypad);
unsigned long qtKeyToX11Keysym(int key, bool keypad);

// ------------------------------------------------------------------ Wayland / evdev

// Factory helpers: return nullptr when built without libevdev.
std::unique_ptr<IInputHook> createEvdevInputHook();
std::unique_ptr<IClickInjector> createUinputClickInjector();

// evdev KEY_* code <-> Qt::Key (also used by the uinput forwarding).
int evdevKeyToQtKey(unsigned int code, bool *keypad);

// Hook used when no backend is available for the session (reports why).
class UnsupportedInputHook final : public IInputHook {
public:
    explicit UnsupportedInputHook(QString reason) : m_reason(std::move(reason)) {}
    PlatformResult start(InputEventSink, KeySuppressionRegistry *) override
    {
        return PlatformResult::notSupported(m_reason);
    }
    void stop() override {}
    bool isRunning() const override { return false; }
    bool canSuppressKeys() const override { return false; }
    bool canSuppressMouseClicks() const override { return false; }
    QString backendName() const override { return QStringLiteral("none"); }
    bool checkHealth() override { return false; }
    int selfRepairCount() const override { return 0; }
    PlatformResult registerKey(const KeyCombo &) override { return PlatformResult::notSupported(m_reason); }
    PlatformResult unregisterKey(const KeyCombo &) override { return PlatformResult::success(); }

private:
    QString m_reason;
};

class UnsupportedClickInjector final : public IClickInjector {
public:
    explicit UnsupportedClickInjector(QString reason) : m_reason(std::move(reason)) {}
    PlatformResult click(MouseButton) override { return PlatformResult::notSupported(m_reason); }
    PlatformResult doubleClick(MouseButton) override { return PlatformResult::notSupported(m_reason); }
    PlatformResult press(MouseButton) override { return PlatformResult::notSupported(m_reason); }
    PlatformResult release(MouseButton) override { return PlatformResult::notSupported(m_reason); }
    int doubleClickIntervalMs() const override { return 400; }

private:
    QString m_reason;
};

// ------------------------------------------------------------------ power

std::unique_ptr<IPowerEvents> createLinuxPowerEvents();

} // namespace ergo
