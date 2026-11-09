#include "platform/linux/LinuxPlatform.h"

#include "core/LogCategories.h"
#include "platform/common/FileOperations.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

#include <memory>

namespace ergo {

namespace {

const QString kInputDevices = QStringLiteral("input-devices");
const QString kUinput = QStringLiteral("uinput");
const QString kSession = QStringLiteral("graphical-session");

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::LinuxPlatform", text);
}

// Desktop Entry spec: arguments containing reserved characters are quoted.
QString quoteExecArgument(const QString &argument)
{
    static const QRegularExpression reserved(QStringLiteral("[\\s\"'\\\\><~|&;$*?#()`]"));
    if (!argument.contains(reserved))
        return argument;
    QString escaped = argument;
    escaped.replace(QLatin1String("\\"), QLatin1String("\\\\"));
    escaped.replace(QLatin1String("\""), QLatin1String("\\\""));
    escaped.replace(QLatin1String("`"), QLatin1String("\\`"));
    escaped.replace(QLatin1String("$"), QLatin1String("\\$"));
    return QStringLiteral("\"%1\"").arg(escaped);
}

QString runningSource()
{
    // When started from an AppImage, copy the AppImage file itself.
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    return appImage.isEmpty() ? QCoreApplication::applicationFilePath() : appImage;
}

QString autostartPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
        .filePath(QStringLiteral("autostart/ergopro2.desktop"));
}

bool allEventDevicesReadable(int *count)
{
    const QDir dir(QStringLiteral("/dev/input"));
    const QStringList devices = dir.entryList({QStringLiteral("event*")}, QDir::System | QDir::Files);
    *count = devices.size();
    if (devices.isEmpty())
        return false;
    for (const QString &device : devices) {
        if (!QFileInfo(dir.filePath(device)).isReadable())
            return false;
    }
    return true;
}

} // namespace

QString desktopEntry(const QString &executablePath, const QStringList &arguments, const QString &iconPath,
                     bool autostart)
{
    QStringList exec{quoteExecArgument(executablePath)};
    for (const QString &argument : arguments)
        exec << quoteExecArgument(argument);
    QString entry = QStringLiteral("[Desktop Entry]\n"
                                   "Type=Application\n"
                                   "Name=ErgoPro2\n"
                                   "GenericName=Ergonomic assistant\n"
                                   "Comment=Auto click, key control and break reminders\n"
                                   "Exec=%1\n"
                                   "Icon=%2\n"
                                   "Terminal=false\n"
                                   "Categories=Utility;Accessibility;\n")
                        .arg(exec.join(QLatin1Char(' ')), iconPath);
    if (autostart)
        entry += QStringLiteral("X-GNOME-Autostart-enabled=true\n");
    return entry;
}

// ---------------------------------------------------------------- installer

QString LinuxInstaller::installFolder()
{
    return QDir(QDir::homePath()).filePath(QStringLiteral(".local/opt/ErgoPro2"));
}

QString LinuxInstaller::menuEntryPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation))
        .filePath(QStringLiteral("ergopro2.desktop"));
}

InstallState LinuxInstaller::state() const
{
    InstallState s;
    s.installLocation = installFolder();
    s.installedExecutable = QDir(s.installLocation).filePath(QStringLiteral("ErgoPro2"));
    s.installed = QFileInfo::exists(s.installedExecutable);
    s.currentLocation = runningSource();
    s.runningFromInstalledCopy = s.installed && fileops::samePath(s.currentLocation, s.installedExecutable);
    return s;
}

PlatformResult LinuxInstaller::install()
{
    const InstallState s = state();
    if (!s.runningFromInstalledCopy) {
        if (const PlatformResult copied = fileops::copyFile(s.currentLocation, s.installedExecutable, true);
            !copied.ok())
            return copied;
        qCInfo(lcPlatform).noquote() << "Installed" << s.currentLocation << "to" << s.installedExecutable;
    }

    // Icon next to the executable, taken from the embedded resources.
    const QString iconPath = QDir(s.installLocation).filePath(QStringLiteral("ergopro2.svg"));
    fileops::removePath(iconPath);
    if (!QFile::copy(QStringLiteral(":/icons/app/app.svg"), iconPath))
        qCWarning(lcPlatform) << "Cannot write the application icon to" << iconPath;
    QFile::setPermissions(iconPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup
                                        | QFileDevice::ReadOther);

    return fileops::writeTextFile(menuEntryPath(), desktopEntry(s.installedExecutable, {}, iconPath, false));
}

PlatformResult LinuxInstaller::uninstall(bool *deferred)
{
    *deferred = false;
    if (const PlatformResult removed = fileops::removePath(menuEntryPath()); !removed.ok())
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

PlatformResult LinuxInstaller::launchInstalledCopy()
{
    const QString exe = state().installedExecutable;
    if (!QProcess::startDetached(exe, {}))
        return PlatformResult::failed(tr("Cannot start '%1'.").arg(exe));
    return PlatformResult::success();
}

// ---------------------------------------------------------------- autostart

bool LinuxAutostart::isEnabled() const
{
    return QFileInfo::exists(autostartPath());
}

QString LinuxAutostart::location() const
{
    return autostartPath();
}

PlatformResult LinuxAutostart::enable(const QString &executablePath, const QStringList &arguments)
{
    const QString icon = QDir(LinuxInstaller::installFolder()).filePath(QStringLiteral("ergopro2.svg"));
    const PlatformResult result =
        fileops::writeTextFile(autostartPath(), desktopEntry(executablePath, arguments, icon, true));
    if (result.ok())
        qCInfo(lcPlatform).noquote() << "Start at login enabled:" << autostartPath() << "->" << executablePath;
    return result;
}

PlatformResult LinuxAutostart::disable()
{
    const PlatformResult result = fileops::removePath(autostartPath());
    if (result.ok())
        qCInfo(lcPlatform) << "Start at login disabled";
    return result;
}

// ---------------------------------------------------------------- permissions

LinuxPermissions::Session LinuxPermissions::detectSession()
{
    const QString type = qEnvironmentVariable("XDG_SESSION_TYPE").toLower();
    if (type == QLatin1String("wayland"))
        return Session::Wayland;
    if (type == QLatin1String("x11"))
        return Session::X11;
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
        return Session::Wayland;
    if (qEnvironmentVariableIsSet("DISPLAY"))
        return Session::X11;
    return Session::Unknown;
}

QString LinuxPermissions::sessionDescription() const
{
    switch (detectSession()) {
    case Session::X11:
        return QStringLiteral("X11");
    case Session::Wayland:
        return QStringLiteral("Wayland");
    case Session::Unknown:
        break;
    }
    return tr("Unknown (no X11 or Wayland session detected)");
}

QList<PermissionState> LinuxPermissions::check() const
{
    const QList<Feature> all = allFeatures();
    const bool pkexecAvailable = !QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty();
    const QString relogin = tr("If you already granted access, log out and log in again.");

    switch (detectSession()) {
    case Session::X11: {
        PermissionState display;
        display.id = QStringLiteral("x11-display");
        display.displayName = tr("X11 session");
        display.granted = true;
        display.explanation = tr("ErgoPro2 uses the X11 display to observe and control the mouse and keyboard. No "
                                 "extra permission is needed.");
        display.requiredFor = all;
        return {display};
    }
    case Session::Wayland: {
        int deviceCount = 0;
        PermissionState input;
        input.id = kInputDevices;
        input.displayName = tr("Read access to input devices (/dev/input)");
        input.granted = allEventDevicesReadable(&deviceCount);
        input.explanation = tr("Wayland does not let applications observe the mouse and keyboard. ErgoPro2 reads "
                               "the input devices directly, which requires membership in the 'input' group.");
        input.howToGrant = pkexecAvailable
                               ? tr("Use 'Grant access' (asks for the administrator password), then log out and in. ")
                                     + relogin
                               : tr("Run: sudo usermod -aG input $USER, then log out and in.");
        input.fixKind = pkexecAvailable ? FixKind::RunFix : FixKind::None;
        input.fixLabel = tr("Grant access…");
        input.requiredFor = all;

        PermissionState uinput;
        uinput.id = kUinput;
        uinput.displayName = tr("Write access to /dev/uinput");
        uinput.granted = QFileInfo(QStringLiteral("/dev/uinput")).isWritable();
        uinput.explanation = tr("Needed to create a virtual mouse/keyboard for clicks and blocked hotkeys on "
                                "Wayland.");
        uinput.howToGrant = pkexecAvailable ? input.howToGrant
                                            : tr("Install a udev rule giving the 'input' group access to /dev/uinput "
                                                 "(see documentation/BUILD.md).");
        uinput.fixKind = input.fixKind;
        uinput.fixLabel = input.fixLabel;
        uinput.requiredFor = {Feature::AutoClick, Feature::KeyControl};
        return {input, uinput};
    }
    case Session::Unknown:
        break;
    }

    PermissionState session;
    session.id = kSession;
    session.displayName = tr("Graphical session (X11 or Wayland)");
    session.granted = false;
    session.explanation = tr("No X11 or Wayland session was detected, so the input features cannot work.");
    session.howToGrant = tr("Start ErgoPro2 from a desktop session.");
    session.requiredFor = all;
    return {session};
}

PlatformResult LinuxPermissions::openSettingsFor(const QString &permissionId)
{
    qCWarning(lcPlatform) << "openSettingsFor not available on Linux for" << permissionId;
    return PlatformResult::notSupported(tr("There is no settings page for this on Linux."));
}

void LinuxPermissions::runFix(const QString &permissionId, std::function<void(PlatformResult)> done)
{
    if (permissionId != kInputDevices && permissionId != kUinput) {
        done(PlatformResult::failed(tr("No automatic fix exists for this requirement.")));
        return;
    }
    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    if (pkexec.isEmpty()) {
        done(PlatformResult::notSupported(tr("pkexec is not installed; follow the manual steps in BUILD.md.")));
        return;
    }
    QString user = qEnvironmentVariable("USER");
    if (user.isEmpty())
        user = QDir::home().dirName();
    static const QRegularExpression validUser(QStringLiteral("^[a-z_][a-z0-9_.-]*\\$?$"));
    if (!validUser.match(user).hasMatch()) {
        done(PlatformResult::failed(tr("Unexpected user name '%1'; follow the manual steps in BUILD.md.").arg(user)));
        return;
    }

    // The user name is passed as $1 (never interpolated into the script).
    const QString script = QStringLiteral(
        "set -e\n"
        "usermod -aG input \"$1\"\n"
        "printf '%s\\n' 'KERNEL==\"uinput\", GROUP=\"input\", MODE=\"0660\", OPTIONS+=\"static_node=uinput\"' "
        "> /etc/udev/rules.d/99-ergopro2-uinput.rules\n"
        "udevadm control --reload-rules\n"
        "udevadm trigger --subsystem-match=misc --action=change || true\n");

    auto *process = new QProcess;
    QObject::connect(process, &QProcess::finished, process,
                     [process, done](int exitCode, QProcess::ExitStatus status) {
                         const QString err = QString::fromLocal8Bit(process->readAllStandardError()).trimmed();
                         process->deleteLater();
                         if (status != QProcess::NormalExit) {
                             done(PlatformResult::failed(tr("The fix was interrupted.")));
                         } else if (exitCode == 126 || exitCode == 127) {
                             // pkexec: dialog dismissed or authorization failed
                             done(PlatformResult::permissionDenied(tr("The administrator authorization was cancelled "
                                                                      "or failed.")));
                         } else if (exitCode != 0) {
                             done(PlatformResult::failed(tr("The fix failed (exit code %1): %2").arg(exitCode).arg(err)));
                         } else {
                             qCInfo(lcPlatform) << "Input device access fix applied (input group + uinput udev rule)";
                             done(PlatformResult::success());
                         }
                     });
    QObject::connect(process, &QProcess::errorOccurred, process, [process, done](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        process->deleteLater();
        done(PlatformResult::failed(tr("pkexec could not be started.")));
    });
    qCInfo(lcPlatform) << "Running input device access fix through pkexec for user" << user;
    process->start(pkexec, {QStringLiteral("/bin/sh"), QStringLiteral("-c"), script, QStringLiteral("ergopro2-fix"), user});
}

} // namespace ergo
