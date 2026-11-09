#include "platform/stub/StubPlatform.h"

#include "core/LogCategories.h"

#include <QCoreApplication>
#include <QProcess>

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

PlatformResult StubPowerEvents::start()
{
    return reportNotImplemented(QStringLiteral("Sleep/lock notifications"));
}

PlatformResult GenericDesktopIntegration::launchApplication(const QString &executablePath,
                                                            const QStringList &arguments)
{
    if (!QProcess::startDetached(executablePath, arguments)) {
        const QString message =
            QCoreApplication::translate("ergo::GenericDesktopIntegration", "Cannot start '%1'.").arg(executablePath);
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    return PlatformResult::success();
}

} // namespace ergo
