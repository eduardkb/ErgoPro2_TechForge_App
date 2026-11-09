#include "platform/stub/StubPlatform.h"

#include "core/LogCategories.h"

namespace ergo {

namespace {

PlatformResult reportNotImplemented(const QString &what)
{
    const PlatformResult result = PlatformResult::notImplemented(what);
    qCWarning(lcPlatform).noquote() << result.message;
    return result;
}

} // namespace

PlatformResult StubInputHook::start()
{
    return reportNotImplemented(QStringLiteral("Global input hook"));
}

PlatformResult StubClickInjector::click(MouseButton)
{
    return reportNotImplemented(QStringLiteral("Click injection"));
}

PlatformResult StubClickInjector::doubleClick(MouseButton)
{
    return reportNotImplemented(QStringLiteral("Double-click injection"));
}

PlatformResult StubClickInjector::press(MouseButton)
{
    return reportNotImplemented(QStringLiteral("Mouse button press injection"));
}

PlatformResult StubClickInjector::release(MouseButton)
{
    return reportNotImplemented(QStringLiteral("Mouse button release injection"));
}

PlatformResult StubAutostart::enable(const QString &, const QStringList &)
{
    return reportNotImplemented(QStringLiteral("Start at login"));
}

PlatformResult StubAutostart::disable()
{
    return reportNotImplemented(QStringLiteral("Start at login"));
}

PlatformResult StubInstaller::install()
{
    return reportNotImplemented(QStringLiteral("Install"));
}

PlatformResult StubInstaller::uninstall()
{
    return reportNotImplemented(QStringLiteral("Uninstall"));
}

PlatformResult StubPermissions::openSettingsFor(const QString &permissionId)
{
    return reportNotImplemented(QStringLiteral("Opening permission settings (%1)").arg(permissionId));
}

PlatformResult StubPowerEvents::start()
{
    return reportNotImplemented(QStringLiteral("Sleep/lock notifications"));
}

} // namespace ergo
