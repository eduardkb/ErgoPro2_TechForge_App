#pragma once

// Placeholder implementations used until the stage that implements each
// interface. They never crash: every call is logged and returns
// PlatformResult::NotImplemented.

#include "platform/PlatformInterfaces.h"

namespace ergo {

class StubInputHook final : public IInputHook {
public:
    PlatformResult start() override;
    void stop() override {}
    bool isRunning() const override { return false; }
};

class StubClickInjector final : public IClickInjector {
public:
    PlatformResult click(MouseButton) override;
    PlatformResult doubleClick(MouseButton) override;
    PlatformResult press(MouseButton) override;
    PlatformResult release(MouseButton) override;
};

class StubPowerEvents final : public IPowerEvents {
public:
    PlatformResult start() override;
    void stop() override {}
};

// Desktop integration for OSes that need nothing special (Windows, Linux).
class GenericDesktopIntegration final : public IDesktopIntegration {
public:
    void setDockIconVisible(bool) override {}
    void activateApplication() override {}
    // Windows/Linux: Qt ends the application itself when the session ends
    // (it does not route this through the main window's close event).
    bool isSessionEnding() const override { return false; }
    bool prefersMonochromeTrayIcon() const override { return false; }
    PlatformResult launchApplication(const QString &executablePath, const QStringList &arguments) override;
};

} // namespace ergo
