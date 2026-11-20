#include "platform/mac/MacInstaller.h"

#include "core/InstallPolicy.h"
#include "core/LogCategories.h"
#include "platform/common/FileOperations.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace ergo {

namespace {

QString executableIn(const QString &bundle)
{
    return QDir(bundle).filePath(QStringLiteral("Contents/MacOS/ErgoPro2"));
}

bool bundleExists(const QString &bundle)
{
    return QFileInfo::exists(executableIn(bundle));
}

} // namespace

QString MacInstaller::systemBundlePath()
{
    return QStringLiteral("/Applications/ErgoPro2.app");
}

QString MacInstaller::userBundlePath()
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
    const QString system = systemBundlePath();
    const QString user = userBundlePath();
    const bool systemExists = bundleExists(system);
    const bool userExists = bundleExists(user);
    const QString running = runningBundlePath();

    const QString installed =
        chooseInstalledCopy(running, {{system, systemExists}, {user, userExists}}, Qt::CaseInsensitive);
    s.installed = !installed.isEmpty();
    // Not installed: where Install would copy to (/Applications needs an
    // administrator account; a standard account gets ~/Applications).
    s.installLocation = s.installed ? installed
                                    : chooseInstallTarget(system, QFileInfo(QFileInfo(system).absolutePath()).isWritable(),
                                                          user);
    s.installedExecutable = executableIn(s.installLocation);
    s.currentLocation = running.isEmpty() ? QCoreApplication::applicationFilePath() : running;
    s.runningFromInstalledCopy = s.installed && fileops::samePath(running, s.installLocation);
    s.registerStartAtLoginOnStart = true;
    if (s.installed && fileops::samePath(installed, system) && userExists)
        s.olderCopy = user;
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
    const QString target = state().installLocation;
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
    // An older ~/Applications copy goes too, otherwise it would count as
    // installed afterwards (2.0.0).
    if (!s.olderCopy.isEmpty()) {
        if (const PlatformResult r = removeOlderCopy(); !r.ok())
            return r;
    }
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
    const QString bundle = state().installLocation;
    // -n: start a new instance even if LaunchServices thinks one is running.
    if (!QProcess::startDetached(QStringLiteral("/usr/bin/open"), {QStringLiteral("-n"), bundle})) {
        return PlatformResult::failed(
            QCoreApplication::translate("ergo::MacInstaller", "Cannot start '%1'.").arg(bundle));
    }
    return PlatformResult::success();
}

PlatformResult MacInstaller::removeOlderCopy()
{
    const InstallState s = state();
    if (s.olderCopy.isEmpty())
        return PlatformResult::success();
    const PlatformResult r = fileops::removePath(s.olderCopy);
    if (r.ok())
        qCInfo(lcPlatform).noquote() << "Older copy removed:" << s.olderCopy;
    else
        qCCritical(lcPlatform).noquote() << "Older copy" << s.olderCopy << "not removed:" << r.message;
    return r;
}

} // namespace ergo
