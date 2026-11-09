#include "platform/mac/MacInstaller.h"

#include "core/LogCategories.h"
#include "platform/common/FileOperations.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace ergo {

QString MacInstaller::installedBundlePath()
{
    return QDir(QDir::homePath()).filePath(QStringLiteral("Applications/ErgoPro2.app"));
}

QString MacInstaller::runningBundlePath()
{
    // <bundle>.app/Contents/MacOS/<executable>
    QDir dir(QCoreApplication::applicationDirPath());
    if (!dir.cdUp() || !dir.cdUp())
        return {};
    return dir.absolutePath().endsWith(QLatin1String(".app")) ? dir.absolutePath() : QString();
}

InstallState MacInstaller::state() const
{
    InstallState s;
    s.installLocation = installedBundlePath();
    s.installedExecutable = QDir(s.installLocation).filePath(QStringLiteral("Contents/MacOS/ErgoPro2"));
    s.installed = QFileInfo::exists(s.installedExecutable);
    const QString running = runningBundlePath();
    s.currentLocation = running.isEmpty() ? QCoreApplication::applicationFilePath() : running;
    s.runningFromInstalledCopy = s.installed && fileops::samePath(running, s.installLocation);
    return s;
}

PlatformResult MacInstaller::install()
{
    const QString source = runningBundlePath();
    if (source.isEmpty()) {
        return PlatformResult::failed(QCoreApplication::translate(
            "ergo::MacInstaller", "ErgoPro2 is not running from an application bundle (.app), so it cannot be "
                                  "installed."));
    }
    const QString target = installedBundlePath();
    if (fileops::samePath(source, target)) {
        qCInfo(lcPlatform) << "Install: already running from the installed copy";
        return PlatformResult::success();
    }

    if (!QDir().mkpath(QFileInfo(target).absolutePath())) {
        return PlatformResult::failed(QCoreApplication::translate("ergo::MacInstaller", "Cannot create the folder '%1'.")
                                          .arg(QFileInfo(target).absolutePath()));
    }
    if (const PlatformResult removed = fileops::removePath(target); !removed.ok())
        return removed;

    // ditto copies bundles correctly (symlinks inside frameworks, extended
    // attributes, code signature).
    QProcess ditto;
    ditto.start(QStringLiteral("/usr/bin/ditto"), {source, target});
    if (!ditto.waitForFinished(180000) || ditto.exitStatus() != QProcess::NormalExit || ditto.exitCode() != 0) {
        const QString details = QString::fromLocal8Bit(ditto.readAllStandardError()).trimmed();
        const QString message = QCoreApplication::translate("ergo::MacInstaller", "Copying the application to '%1' "
                                                                                  "failed: %2")
                                    .arg(target, details.isEmpty() ? ditto.errorString() : details);
        qCCritical(lcPlatform).noquote() << message;
        fileops::removePath(target); // do not leave a half-copied bundle behind
        return PlatformResult::failed(message);
    }
    qCInfo(lcPlatform).noquote() << "Installed" << source << "to" << target;
    return PlatformResult::success();
}

PlatformResult MacInstaller::uninstall(bool *deferred)
{
    *deferred = false;
    const InstallState s = state();
    if (!QFileInfo::exists(s.installLocation))
        return PlatformResult::success();
    if (s.runningFromInstalledCopy) {
        *deferred = true;
        return fileops::scheduleDeletionAfterExit(s.installLocation);
    }
    return fileops::removePath(s.installLocation);
}

PlatformResult MacInstaller::launchInstalledCopy()
{
    // -n: start a new instance even if LaunchServices thinks one is running.
    if (!QProcess::startDetached(QStringLiteral("/usr/bin/open"), {QStringLiteral("-n"), installedBundlePath()})) {
        return PlatformResult::failed(
            QCoreApplication::translate("ergo::MacInstaller", "Cannot start '%1'.").arg(installedBundlePath()));
    }
    return PlatformResult::success();
}

} // namespace ergo
