#pragma once

// Shared implementations for OSes that need nothing special.

#include "platform/PlatformInterfaces.h"

namespace ergo {

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
    // Linux: Qt's stay-on-top flag plus the full-screen state set
    // _NET_WM_STATE_ABOVE / _NET_WM_STATE_FULLSCREEN.
    void raiseOverlayWindow(quintptr) override {}
    bool overlayUsesFullScreenState() const override { return true; }
    std::optional<bool> isFullscreenAppActive() const override { return std::nullopt; }
};

} // namespace ergo
