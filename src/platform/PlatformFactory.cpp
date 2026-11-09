#include "platform/PlatformFactory.h"

#include "core/LogCategories.h"
#include "platform/stub/StubPlatform.h"

#include <QtGlobal>

#if defined(Q_OS_MACOS)
#include "platform/mac/MacDesktopIntegration.h"
#endif

namespace ergo {

Platform createPlatform()
{
    Platform p;

#if defined(Q_OS_MACOS)
    p.osName = QStringLiteral("macOS");
    p.desktop = std::make_unique<MacDesktopIntegration>();
#elif defined(Q_OS_WIN)
    p.osName = QStringLiteral("Windows");
    p.desktop = std::make_unique<GenericDesktopIntegration>();
#elif defined(Q_OS_LINUX)
    p.osName = QStringLiteral("Linux");
    p.desktop = std::make_unique<GenericDesktopIntegration>();
#else
#error "Unsupported operating system"
#endif

    // Stage 0: the remaining services are stubs on every OS.
    p.inputHook = std::make_unique<StubInputHook>();       // stage 2
    p.clickInjector = std::make_unique<StubClickInjector>(); // stage 2
    p.powerEvents = std::make_unique<StubPowerEvents>();   // stage 2
    p.autostart = std::make_unique<StubAutostart>();       // stage 1
    p.installer = std::make_unique<StubInstaller>();       // stage 1
    p.permissions = std::make_unique<StubPermissions>();   // stage 1

    qCInfo(lcPlatform).noquote() << "Platform:" << p.osName
                                 << "- input hook, click injection, power events (stage 2) and "
                                    "autostart, installer, permissions (stage 1) are not implemented yet";
    return p;
}

} // namespace ergo
