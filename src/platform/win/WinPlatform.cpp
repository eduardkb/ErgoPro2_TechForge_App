#include "platform/win/WinPlatform.h"

#include "core/LogCategories.h"
#include "platform/common/FileOperations.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h> // SHQueryUserNotificationState

namespace ergo {

namespace {

const QString kRunKey = QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const QString kValueName = QStringLiteral("ErgoPro2");

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::WinPlatform", text);
}

QString quoteArgument(const QString &argument)
{
    if (!argument.contains(QLatin1Char(' ')) && !argument.contains(QLatin1Char('"')) && !argument.isEmpty())
        return argument;
    QString escaped = argument;
    escaped.replace(QLatin1String("\""), QLatin1String("\\\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}

} // namespace

// ---------------------------------------------------------------- installer

QString WinInstaller::installFolder()
{
    // GenericDataLocation on Windows is %LOCALAPPDATA%.
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
        .filePath(QStringLiteral("Programs/ErgoPro2"));
}

QString WinInstaller::startMenuShortcut()
{
    // ApplicationsLocation on Windows is the user's Start Menu\Programs folder.
    return QDir(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation))
        .filePath(QStringLiteral("ErgoPro2.lnk"));
}

InstallState WinInstaller::state() const
{
    InstallState s;
    s.installLocation = installFolder();
    s.installedExecutable = QDir(s.installLocation).filePath(QStringLiteral("ErgoPro2.exe"));
    s.installed = QFileInfo::exists(s.installedExecutable);
    s.currentLocation = QCoreApplication::applicationDirPath();
    s.runningFromInstalledCopy = s.installed && fileops::samePath(s.currentLocation, s.installLocation);
    return s;
}

PlatformResult WinInstaller::install()
{
    const QString source = QCoreApplication::applicationDirPath();
    const QString target = installFolder();
    if (!fileops::samePath(source, target)) {
        // Only a deployed folder (windeployqt) contains the Qt DLLs the copy needs.
        const QDir sourceDir(source);
        if (!sourceDir.exists(QStringLiteral("Qt6Core.dll")) && !sourceDir.exists(QStringLiteral("Qt6Cored.dll"))) {
            return PlatformResult::failed(
                tr("This copy of ErgoPro2 cannot be installed because its folder does not contain the Qt libraries. "
                   "Prepare it with windeployqt first (see documentation/BUILD.md, section 2.5)."));
        }
        if (const PlatformResult removed = fileops::removePath(target); !removed.ok())
            return removed;
        if (const PlatformResult copied = fileops::copyDirectory(source, target); !copied.ok()) {
            fileops::removePath(target);
            return copied;
        }
        qCInfo(lcPlatform).noquote() << "Installed" << QDir::toNativeSeparators(source) << "to"
                                     << QDir::toNativeSeparators(target);
    }

    // Start Menu shortcut (QFile::link creates a .lnk file on Windows).
    const QString shortcut = startMenuShortcut();
    fileops::removePath(shortcut);
    QDir().mkpath(QFileInfo(shortcut).absolutePath());
    if (!QFile::link(state().installedExecutable, shortcut)) {
        const QString message = tr("ErgoPro2 was installed, but the Start Menu shortcut could not be created.");
        qCWarning(lcPlatform).noquote() << message << shortcut;
        return PlatformResult::failed(message);
    }
    return PlatformResult::success();
}

PlatformResult WinInstaller::uninstall(bool *deferred)
{
    *deferred = false;
    if (const PlatformResult removed = fileops::removePath(startMenuShortcut()); !removed.ok())
        return removed;
    const InstallState s = state();
    if (!QFileInfo::exists(s.installLocation))
        return PlatformResult::success();
    if (s.runningFromInstalledCopy) {
        *deferred = true;
        return fileops::scheduleDeletionAfterExit(s.installLocation);
    }
    return fileops::removePath(s.installLocation);
}

PlatformResult WinInstaller::launchInstalledCopy()
{
    const QString exe = state().installedExecutable;
    if (!QProcess::startDetached(exe, {}))
        return PlatformResult::failed(tr("Cannot start '%1'.").arg(QDir::toNativeSeparators(exe)));
    return PlatformResult::success();
}

// ---------------------------------------------------------------- autostart

bool WinAutostart::isEnabled() const
{
    QSettings run(kRunKey, QSettings::NativeFormat);
    return run.contains(kValueName);
}

QString WinAutostart::location() const
{
    return kRunKey + QStringLiteral("\\") + kValueName;
}

PlatformResult WinAutostart::enable(const QString &executablePath, const QStringList &arguments)
{
    QStringList parts{QStringLiteral("\"%1\"").arg(QDir::toNativeSeparators(executablePath))};
    for (const QString &argument : arguments)
        parts << quoteArgument(argument);

    QSettings run(kRunKey, QSettings::NativeFormat);
    run.setValue(kValueName, parts.join(QLatin1Char(' ')));
    run.sync();
    if (run.status() != QSettings::NoError) {
        const QString message = tr("Cannot write the start-at-login registry value (%1).").arg(location());
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    qCInfo(lcPlatform).noquote() << "Start at login enabled:" << parts.join(QLatin1Char(' '));
    return PlatformResult::success();
}

PlatformResult WinAutostart::disable()
{
    QSettings run(kRunKey, QSettings::NativeFormat);
    run.remove(kValueName);
    run.sync();
    if (run.status() != QSettings::NoError) {
        const QString message = tr("Cannot remove the start-at-login registry value (%1).").arg(location());
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    qCInfo(lcPlatform) << "Start at login disabled";
    return PlatformResult::success();
}

// ---------------------------------------------------------------- permissions

QList<PermissionState> WinPermissions::check() const
{
    PermissionState none;
    none.id = QStringLiteral("windows");
    none.displayName = tr("No special permission needed");
    none.granted = true;
    none.explanation = tr("Note: Windows does not let clicks and hotkeys reach windows of programs that run as "
                          "Administrator, unless ErgoPro2 also runs as Administrator.");
    return {none};
}

QString WinPermissions::sessionDescription() const
{
    return QStringLiteral("Windows desktop");
}

PlatformResult WinPermissions::openSettingsFor(const QString &permissionId)
{
    qCWarning(lcPlatform) << "openSettingsFor called for" << permissionId << "- Windows needs no permission";
    return PlatformResult::notSupported(tr("Windows needs no special permission."));
}

void WinPermissions::runFix(const QString &permissionId, std::function<void(PlatformResult)> done)
{
    done(openSettingsFor(permissionId));
}

// ---------------------------------------------------------------- desktop

PlatformResult WinDesktopIntegration::launchApplication(const QString &executablePath, const QStringList &arguments)
{
    if (!QProcess::startDetached(executablePath, arguments)) {
        const QString message = tr("Cannot start '%1'.").arg(QDir::toNativeSeparators(executablePath));
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    return PlatformResult::success();
}

void WinDesktopIntegration::raiseOverlayWindow(quintptr nativeWindow)
{
    // Topmost and in front without stealing the keyboard focus.
    if (!SetWindowPos(reinterpret_cast<HWND>(nativeWindow), HWND_TOPMOST, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW))
        qCWarning(lcPlatform) << "SetWindowPos(HWND_TOPMOST) failed, error" << GetLastError();
}

std::optional<bool> WinDesktopIntegration::isFullscreenAppActive() const
{
    QUERY_USER_NOTIFICATION_STATE state;
    if (FAILED(SHQueryUserNotificationState(&state)))
        return std::nullopt;
    return state == QUNS_BUSY || state == QUNS_RUNNING_D3D_FULL_SCREEN || state == QUNS_PRESENTATION_MODE;
}

} // namespace ergo
