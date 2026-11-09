#include "ui/InstallController.h"

#include "core/LogCategories.h"
#include "platform/PlatformFactory.h"

#include <QDir>
#include <QMessageBox>

namespace ergo {

InstallController::InstallController(Platform &platform, QObject *parent)
    : QObject(parent)
    , m_platform(platform)
{
}

InstallController::Status InstallController::status() const
{
    Status s;
    s.install = m_platform.installer->state();
    s.startsAtLogin = m_platform.autostart->isEnabled();
    s.autostartLocation = m_platform.autostart->location();
    s.location = s.install.runningFromInstalledCopy ? LocationCheck{}
                                                    : classifyLocation(s.install.currentLocation, currentLocationRoots());
    return s;
}

void InstallController::install(QWidget *parent)
{
    qCInfo(lcApp) << "Install requested";
    const PlatformResult copied = m_platform.installer->install();
    if (!copied.ok()) {
        emit changed();
        QMessageBox::warning(parent, tr("Install failed"), copied.message);
        return;
    }

    const Status s = status();
    const PlatformResult registered =
        m_platform.autostart->enable(s.install.installedExecutable, {QStringLiteral("--autostart")});
    emit changed();
    if (!registered.ok()) {
        QMessageBox::warning(parent, tr("Start at login not enabled"),
                             tr("ErgoPro2 was installed at %1, but it could not be registered to start at "
                                "login:\n%2")
                                 .arg(QDir::toNativeSeparators(s.install.installLocation), registered.message));
        return;
    }
    qCInfo(lcApp).noquote() << "Installed at" << s.install.installLocation << "- starts at login";

    if (s.install.runningFromInstalledCopy) {
        QMessageBox::information(parent, tr("Installed"),
                                 tr("ErgoPro2 is installed at %1 and will start automatically when you log in.")
                                     .arg(QDir::toNativeSeparators(s.install.installLocation)));
        return;
    }

    const auto answer = QMessageBox::question(
        parent, tr("Installed"),
        tr("ErgoPro2 was installed at %1 and will start automatically when you log in.\n\nRestart ErgoPro2 now "
           "from the installed location? (Recommended - you can then delete this copy.)")
            .arg(QDir::toNativeSeparators(s.install.installLocation)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer == QMessageBox::Yes)
        emit relaunchRequested(s.install.installedExecutable);
}

void InstallController::uninstall(QWidget *parent)
{
    const Status before = status();
    const auto answer = QMessageBox::question(
        parent, tr("Uninstall ErgoPro2"),
        tr("This stops ErgoPro2 from starting at login and removes the installed copy at %1.\n\nYour settings "
           "and log files are kept.\n\nContinue?")
            .arg(QDir::toNativeSeparators(before.install.installLocation)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;

    qCInfo(lcApp) << "Uninstall requested";
    QStringList errors;
    if (const PlatformResult r = m_platform.autostart->disable(); !r.ok())
        errors << r.message;

    bool deferred = false;
    if (const PlatformResult r = m_platform.installer->uninstall(&deferred); !r.ok())
        errors << r.message;
    emit changed();

    if (!errors.isEmpty()) {
        QMessageBox::warning(parent, tr("Uninstall incomplete"), errors.join(QLatin1Char('\n')));
        return;
    }
    if (deferred) {
        QMessageBox::information(parent, tr("Uninstalled"),
                                 tr("ErgoPro2 no longer starts at login. Because this is the installed copy, it is "
                                    "deleted right after ErgoPro2 exits.\n\nErgoPro2 will now exit. Your settings "
                                    "and log files are kept."));
        emit exitRequested();
        return;
    }
    QMessageBox::information(parent, tr("Uninstalled"),
                             tr("ErgoPro2 no longer starts at login and the installed copy was removed. Your "
                                "settings and log files are kept."));
}

} // namespace ergo
