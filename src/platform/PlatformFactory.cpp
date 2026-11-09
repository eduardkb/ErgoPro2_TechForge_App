#include "platform/PlatformFactory.h"

#include "core/LogCategories.h"
#include "platform/stub/StubPlatform.h"

#include <QtGlobal>

#if defined(Q_OS_MACOS)
#include "platform/mac/MacAutostart.h"
#include "platform/mac/MacDesktopIntegration.h"
#include "platform/mac/MacInstaller.h"
#include "platform/mac/MacPermissions.h"
#elif defined(Q_OS_WIN)
#include "platform/win/WinPlatform.h"
#elif defined(Q_OS_LINUX)
#include "platform/linux/LinuxPlatform.h"
#endif

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
#elif defined(Q_OS_WIN)
    p.osName = QStringLiteral("Windows");
    p.desktop = std::make_unique<GenericDesktopIntegration>();
    p.autostart = std::make_unique<WinAutostart>();
    p.installer = std::make_unique<WinInstaller>();
    p.permissions = std::make_unique<WinPermissions>();
#elif defined(Q_OS_LINUX)
    p.osName = QStringLiteral("Linux");
    p.desktop = std::make_unique<GenericDesktopIntegration>();
    p.autostart = std::make_unique<LinuxAutostart>();
    p.installer = std::make_unique<LinuxInstaller>();
    p.permissions = std::make_unique<LinuxPermissions>();
#else
#error "Unsupported operating system"
#endif

    // Implemented in stage 2; stubs on every OS until then.
    p.inputHook = std::make_unique<StubInputHook>();
    p.clickInjector = std::make_unique<StubClickInjector>();
    p.powerEvents = std::make_unique<StubPowerEvents>();

    qCInfo(lcPlatform).noquote() << "Platform:" << p.osName << "- session:" << p.permissions->sessionDescription()
                                 << "- input hook, click injection and power events are implemented in stage 2";
    return p;
}

} // namespace ergo
