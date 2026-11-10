#pragma once

#include "platform/PlatformInterfaces.h"

namespace ergo {

// macOS: switches the NSApplication activation policy so the Dock icon is
// only shown while the main window is visible.
class MacDesktopIntegration final : public IDesktopIntegration {
public:
    void setDockIconVisible(bool visible) override;
    void activateApplication() override;
    bool isSessionEnding() const override;
    bool prefersMonochromeTrayIcon() const override { return true; }
    PlatformResult launchApplication(const QString &executablePath, const QStringList &arguments) override;
    void raiseOverlayWindow(quintptr nativeWindow) override;
    bool overlayUsesFullScreenState() const override { return false; }
    std::optional<bool> isFullscreenAppActive() const override;
};

} // namespace ergo
