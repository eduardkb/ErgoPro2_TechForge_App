#include "platform/stub/StubPlatform.h"

#include "core/LogCategories.h"

#include <QCoreApplication>
#include <QProcess>

namespace ergo {

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
