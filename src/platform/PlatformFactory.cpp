#include "platform/PlatformFactory.h"

#include "core/LogCategories.h"
#include "platform/stub/StubPlatform.h"

#include <QtGlobal>

#if defined(Q_OS_MACOS)
#include "platform/mac/MacAutostart.h"
#include "platform/mac/MacDesktopIntegration.h"
#include "platform/mac/MacInput.h"
#include "platform/mac/MacInstaller.h"
#include "platform/mac/MacPermissions.h"
#elif defined(Q_OS_WIN)
#include "platform/win/WinInput.h"
#include "platform/win/WinPlatform.h"
#elif defined(Q_OS_LINUX)
#include "platform/linux/LinuxInput.h"
#include "platform/linux/LinuxPlatform.h"
#endif

#include <QCoreApplication>

namespace ergo {

Platform createPlatform()
{
    Platform p;

#if defined(Q_OS_MACOS)
    p.osName = QStringLiteral("macOS");
    p.desktop = std::make_unique<MacDesktopIntegration>();
    p.autostart = std::make_unique<MacAutostart>();
    p.installer = std::make_unique<MacInstaller>();
    p.permissions = std::make_unique<MacPermissions>();
    p.inputHook = std::make_unique<MacInputHook>();
    p.clickInjector = std::make_unique<MacClickInjector>();
    p.powerEvents = std::make_unique<MacPowerEvents>();
#elif defined(Q_OS_WIN)
    p.osName = QStringLiteral("Windows");
    p.desktop = std::make_unique<WinDesktopIntegration>();
    p.autostart = std::make_unique<WinAutostart>();
    p.installer = std::make_unique<WinInstaller>();
    p.permissions = std::make_unique<WinPermissions>();
    p.inputHook = std::make_unique<WinInputHook>();
    p.clickInjector = std::make_unique<WinClickInjector>();
    p.powerEvents = std::make_unique<WinPowerEvents>();
#elif defined(Q_OS_LINUX)
    p.osName = QStringLiteral("Linux");
    p.desktop = std::make_unique<GenericDesktopIntegration>();
    p.autostart = std::make_unique<LinuxAutostart>();
    p.installer = std::make_unique<LinuxInstaller>();
    p.permissions = std::make_unique<LinuxPermissions>();
    p.powerEvents = createLinuxPowerEvents();
    // X11 cannot tag injected events: injector and hook share a tracker.
    static SyntheticTracker tracker;
    switch (LinuxPermissions::detectSession()) {
    case LinuxPermissions::Session::X11:
        p.inputHook = std::make_unique<X11InputHook>(&tracker);
        p.clickInjector = std::make_unique<X11ClickInjector>(&tracker);
        break;
    case LinuxPermissions::Session::Wayland:
        p.inputHook = createEvdevInputHook();
        p.clickInjector = createUinputClickInjector();
        if (!p.inputHook) {
            const QString reason = QCoreApplication::translate(
                "ergo::PlatformFactory", "This ErgoPro2 was built without libevdev, so mouse and keyboard features "
                                         "are not available on Wayland. Use an X11 session or rebuild with libevdev.");
            p.inputHook = std::make_unique<UnsupportedInputHook>(reason);
            p.clickInjector = std::make_unique<UnsupportedClickInjector>(reason);
        }
        break;
    case LinuxPermissions::Session::Unknown: {
        const QString reason = QCoreApplication::translate("ergo::PlatformFactory",
                                                           "No X11 or Wayland desktop session was detected.");
        p.inputHook = std::make_unique<UnsupportedInputHook>(reason);
        p.clickInjector = std::make_unique<UnsupportedClickInjector>(reason);
        break;
    }
    }
#else
#error "Unsupported operating system"
#endif

    qCInfo(lcPlatform).noquote() << "Platform:" << p.osName << "- session:" << p.permissions->sessionDescription()
                                 << "- input backend:" << p.inputHook->backendName();
    return p;
}

} // namespace ergo
