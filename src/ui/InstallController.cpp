#include "ui/InstallController.h"

#include "core/InstallPolicy.h"
#include "ergopro2/Version.h"
#include "core/LogCategories.h"
#include "core/settings/GeneralSettings.h"
#include "core/settings/SettingsStore.h"
#include "platform/PlatformFactory.h"

#include <QDir>
#include <QMessageBox>

namespace ergo {

InstallController::InstallController(Platform &platform, SettingsStore &settings, QObject *parent)
    : QObject(parent)
    , m_platform(platform)
    , m_settings(settings)
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
    // Install already confirmed this version: no second "installed" message after the restart (2.2.1).
    m_settings.setValue(GeneralSettings::InstallConfirmedVersion,
                        versionNumber(ERGOPRO2_VERSION_MAJOR, ERGOPRO2_VERSION_MINOR, ERGOPRO2_VERSION_PATCH));

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
    const QString question =
        before.install.managedByInstaller
            ? tr("This runs the ErgoPro2 uninstaller: it removes ErgoPro2 from %1, its Start menu entry and the "
                 "start at login. ErgoPro2 exits first.\n\nYour settings, statistics and log files are kept."
                 "\n\nContinue?")
            : tr("This stops ErgoPro2 from starting at login and removes the installed copy at %1.\n\nYour "
                 "settings and log files are kept.\n\nContinue?");
    const auto answer = QMessageBox::question(parent, tr("Uninstall ErgoPro2"),
                                              question.arg(QDir::toNativeSeparators(before.install.installLocation)),
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
    if (deferred && before.install.managedByInstaller) {
        qCInfo(lcApp) << "Exiting so that the uninstaller can remove ErgoPro2";
        emit exitRequested(); // the uninstaller shows its own progress window
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

void InstallController::checkAtStartup(QWidget *parent)
{
    runStartupChecks(parent);
    if (m_dockShownForDialog) {
        m_dockShownForDialog = false;
        m_platform.desktop->setDockIconVisible(false); // started hidden in the tray: back to the tray only
    }
}

void InstallController::runStartupChecks(QWidget *parent)
{
    const Status s = status();
    InstallStartupFacts facts;
    facts.runningFromInstalledCopy = s.install.runningFromInstalledCopy;
    facts.registerStartAtLoginOnStart = s.install.registerStartAtLoginOnStart;
    facts.startsAtLogin = s.startsAtLogin;
    const QString registered = m_platform.autostart->registeredExecutable();
    facts.startAtLoginPointsToInstalledCopy =
        !registered.isEmpty() && QDir::cleanPath(registered) == QDir::cleanPath(s.install.installedExecutable);
    facts.olderCopyExists = !s.install.olderCopy.isEmpty();
    facts.olderCopyKept = m_settings.boolValue(GeneralSettings::OlderCopyKept);
    // 2.1.0: the installer .dmg (only looked for where it can matter).
    QList<IInstaller::InstallerImage> images;
    if (facts.runningFromInstalledCopy && facts.registerStartAtLoginOnStart)
        images = m_platform.installer->installerImages();
    for (const IInstaller::InstallerImage &image : std::as_const(images))
        facts.installerImageMounted = facts.installerImageMounted || !image.mountPoint.isEmpty();
    facts.installerImageFound = !images.isEmpty();
    facts.currentVersion = versionNumber(ERGOPRO2_VERSION_MAJOR, ERGOPRO2_VERSION_MINOR, ERGOPRO2_VERSION_PATCH);
    facts.confirmedVersion = m_settings.intValue(GeneralSettings::InstallConfirmedVersion);
    const InstallStartupActions actions = decideInstallStartupActions(facts);
    qCDebug(lcApp) << "Install check at start: running from installed copy" << facts.runningFromInstalledCopy
                   << "- confirmed version" << facts.confirmedVersion << "this" << facts.currentVersion
                   << "- installer images" << images.size() << "(mounted:" << facts.installerImageMounted << ")";
    bool registeredOk = true;

    if (actions.registerStartAtLogin) {
        qCInfo(lcApp).noquote() << "Running from the installed copy" << s.install.installLocation
                                << "- registering start at login"
                                << (registered.isEmpty() ? QString() : QStringLiteral("(was: %1)").arg(registered));
        const PlatformResult r =
            m_platform.autostart->enable(s.install.installedExecutable, {QStringLiteral("--autostart")});
        emit changed();
        registeredOk = r.ok();
        if (!r.ok()) {
            showOnTop(parent, QMessageBox::Warning, tr("Start at login not enabled"),
                                 tr("ErgoPro2 is installed at %1, but it could not be registered to start at "
                                    "login:\n%2")
                                     .arg(QDir::toNativeSeparators(s.install.installLocation), r.message));
        }
    }

    confirmInstallation(parent, actions.confirmInstalled && registeredOk, s.install.installLocation,
                        actions.offerToDeleteInstaller ? images : QList<IInstaller::InstallerImage>{});
    if (actions.confirmInstalled && registeredOk)
        m_settings.setValue(GeneralSettings::InstallConfirmedVersion, facts.currentVersion);

    if (!actions.askToRemoveOlderCopy)
        return;
    qCInfo(lcApp).noquote() << "An older copy exists at" << s.install.olderCopy;
    const auto answer = showOnTop(
        parent, QMessageBox::Question, tr("Older copy of ErgoPro2"),
        tr("ErgoPro2 now runs from %1.\n\nAn older copy is still at %2. Remove it?\n\nYour settings, statistics "
           "and log files are kept.")
            .arg(QDir::toNativeSeparators(s.install.installLocation), QDir::toNativeSeparators(s.install.olderCopy)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer != QMessageBox::Yes) {
        qCInfo(lcApp) << "Older copy kept (the user will not be asked again)";
        m_settings.setValue(GeneralSettings::OlderCopyKept, true);
        return;
    }
    const PlatformResult removed = m_platform.installer->removeOlderCopy();
    emit changed();
    if (!removed.ok()) {
        showOnTop(parent, QMessageBox::Warning, tr("Older copy not removed"),
                             tr("The older copy at %1 could not be removed:\n%2\n\nYou can delete it in the "
                                "Finder.")
                                 .arg(QDir::toNativeSeparators(s.install.olderCopy), removed.message));
    }
}

// 2.1.0 (owner request): "installed" confirmation for a copy dragged into
// Applications, combined with the offer to eject the installer .dmg and move it
// to the Trash.
void InstallController::confirmInstallation(QWidget *parent, bool confirm, const QString &location,
                                            const QList<IInstaller::InstallerImage> &images)
{
    if (!confirm && images.isEmpty())
        return;
    const QString installed = tr("ErgoPro2 %1 is installed at %2 and starts automatically when you log in.")
                                  .arg(QStringLiteral(ERGOPRO2_VERSION_STRING), QDir::toNativeSeparators(location));
    if (images.isEmpty()) {
        qCInfo(lcApp) << "Installation confirmed to the user";
        showOnTop(parent, QMessageBox::Information, tr("ErgoPro2 installed"), installed);
        return;
    }
    QStringList files;
    for (const IInstaller::InstallerImage &image : images)
        files << QStringLiteral("• %1").arg(QDir::toNativeSeparators(image.imagePath));
    const QString offer = tr("The installer disk image is no longer needed. Eject it and move it to the Trash?\n\n%1")
                              .arg(files.join(QLatin1Char('\n')));
    const auto answer = showOnTop(parent, QMessageBox::Question, confirm ? tr("ErgoPro2 installed") : tr("Installer disk image"),
                                              confirm ? installed + QStringLiteral("\n\n") + offer : offer,
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (answer != QMessageBox::Yes) {
        qCInfo(lcApp) << "Installer disk image kept";
        return;
    }
    QStringList errors;
    for (const IInstaller::InstallerImage &image : images) {
        if (const PlatformResult r = m_platform.installer->removeInstallerImage(image); !r.ok())
            errors << r.message;
    }
    if (!errors.isEmpty())
        showOnTop(parent, QMessageBox::Warning, tr("Installer disk image not removed"), errors.join(QLatin1Char('\n')));
}

// 2.2.1: the start-up dialogs of a hidden (tray-only) app opened behind the
// other windows (the app had no Dock icon and was not active), so they were
// not seen. Show the Dock icon, activate the app and keep the box on top.
int InstallController::showOnTop(QWidget *parent, QMessageBox::Icon icon, const QString &title, const QString &text,
                                 QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    if (!parent && !m_dockShownForDialog) {
        m_dockShownForDialog = true;
        m_platform.desktop->setDockIconVisible(true);
    }
    QMessageBox box(icon, title, text, buttons, parent);
    box.setDefaultButton(defaultButton);
    box.setWindowFlag(Qt::WindowStaysOnTopHint, true);
    box.show();
    box.raise();
    box.activateWindow();
    m_platform.desktop->activateApplication();
    qCInfo(lcApp).noquote() << "Dialog shown:" << title;
    const int answer = box.exec();
    qCInfo(lcApp).noquote() << "Dialog answered:" << title << "->"
                            << (answer == QMessageBox::Yes ? "Yes" : answer == QMessageBox::No ? "No" : "OK");
    return answer;
}

} // namespace ergo
