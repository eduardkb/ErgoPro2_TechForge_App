#include "ui/AppController.h"

#include "core/LogCategories.h"
#include "core/LogLevel.h"
#include "core/Logger.h"
#include "core/StartupMode.h"
#include "core/settings/GeneralSettings.h"
#include "platform/PlatformFactory.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/InstallController.h"
#include "ui/MainWindow.h"
#include "ui/PermissionsDialog.h"
#include "ui/SingleInstance.h"
#include "ui/TrayController.h"

#include <QApplication>
#include <QMessageBox>

namespace ergo {

namespace {
constexpr int kMaintenanceIntervalMs = 60 * 60 * 1000; // hourly log retention check
}

AppController::AppController(SettingsStore &settings, Logger &logger, Platform &platform, SingleInstance &instance,
                             const LaunchOptions &options, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_logger(logger)
    , m_platform(platform)
    , m_instance(instance)
    , m_options(options)
{
}

AppController::~AppController() = default;

void AppController::start(const SettingsLoadResult &loadResult)
{
    // Logging options from settings.
    applySetting(GeneralSettings::LogLevel);
    applySetting(GeneralSettings::LogRetentionDays);
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) { applySetting(key); });
    connect(&m_settings, &SettingsStore::saveFailed, this, [this](const QString &message) {
        showError(tr("Settings not saved"), message);
    });

    // The app keeps running in the tray when the window is closed.
    QApplication::setQuitOnLastWindowClosed(false);

    m_availability = std::make_unique<FeatureAvailabilityService>(*m_platform.permissions);
    m_install = std::make_unique<InstallController>(m_platform);
    connect(m_install.get(), &InstallController::relaunchRequested, this, &AppController::relaunch);
    connect(m_install.get(), &InstallController::exitRequested, this, &AppController::exitApplication);
    m_context = std::make_unique<AppContext>(AppContext{m_settings, m_logger, m_platform, *m_availability, *m_install});

    m_window = std::make_unique<MainWindow>(*m_context);
    connect(m_window.get(), &MainWindow::hideToTrayRequested, this, &AppController::hideMainWindowToTray);
    connect(m_window.get(), &MainWindow::showRequested, this, &AppController::showMainWindow);
    connect(m_window.get(), &MainWindow::exitRequested, this, &AppController::exitApplication);
    m_window->setSessionEndingCheck([this] { return m_platform.desktop->isSessionEnding(); });

    if (TrayController::isAvailable()) {
        m_tray = std::make_unique<TrayController>(m_platform.desktop->prefersMonochromeTrayIcon());
        connect(m_tray.get(), &TrayController::showWindowRequested, this, &AppController::showMainWindow);
        connect(m_tray.get(), &TrayController::exitRequested, this, &AppController::exitApplication);
        m_tray->show();
    } else {
        qCWarning(lcUi) << "No system tray available on this desktop; the window will minimize instead of hiding";
        m_window->setHideToTrayEnabled(false);
    }

    connect(&m_instance, &SingleInstance::activationRequested, this, &AppController::showMainWindow);

    m_maintenanceTimer.setInterval(kMaintenanceIntervalMs);
    connect(&m_maintenanceTimer, &QTimer::timeout, this, [this] { m_logger.cleanupOldFiles(); });
    m_maintenanceTimer.start();

    // Startup behavior (W-02): depends on how the app was started.
    const StartupWindowMode mode = decideStartupWindowMode(
        m_options.autostart, m_settings.stringValue(GeneralSettings::StartupAutoMode),
        m_settings.stringValue(GeneralSettings::StartupManualMode), static_cast<bool>(m_tray));
    qCInfo(lcApp) << "Started" << (m_options.autostart ? "automatically at login (--autostart)" : "manually") << "-"
                  << (mode == StartupWindowMode::ShowWindow ? "showing the window" : "hidden in the tray");
    if (mode == StartupWindowMode::ShowWindow)
        showMainWindow();
    else
        m_platform.desktop->setDockIconVisible(false);

    showPermissionsWindowIfNeeded();
    reportSettingsLoad(loadResult);
}

void AppController::applySetting(const QString &key)
{
    if (key == GeneralSettings::LogLevel) {
        const QString id = m_settings.stringValue(key);
        const auto selection = logLevelSelectionFromId(id);
        if (!selection) {
            qCCritical(lcApp) << "Unknown log level id in settings:" << id;
            return;
        }
        m_logger.setSelection(*selection);
        qCInfo(lcApp) << "Log level set to" << id;
    } else if (key == GeneralSettings::LogRetentionDays) {
        m_logger.setRetentionDays(m_settings.intValue(key));
        m_logger.cleanupOldFiles();
        qCInfo(lcApp) << "Log retention set to" << m_settings.intValue(key) << "day(s)";
    }
}

void AppController::showMainWindow()
{
    if (!m_window)
        return;
    m_platform.desktop->setDockIconVisible(true);
    m_window->showNormal();
    m_window->raise();
    m_window->activateWindow();
    m_platform.desktop->activateApplication();
}

void AppController::hideMainWindowToTray()
{
    if (!m_window || !m_tray)
        return;
    m_window->hide();
    m_platform.desktop->setDockIconVisible(false);
    qCDebug(lcUi) << "Main window hidden to tray";

    if (!m_settings.boolValue(GeneralSettings::TrayHintShown)) {
        m_tray->showMessage(tr("ErgoPro2"), tr("ErgoPro2 is still running in the tray. Use the tray menu to "
                                               "open the window or exit."));
        m_settings.setValue(GeneralSettings::TrayHintShown, true);
    }
}

void AppController::exitApplication()
{
    if (m_exiting)
        return;
    m_exiting = true;
    qCInfo(lcApp) << "Exit requested by the user";
    if (!m_settings.saveNow())
        qCCritical(lcApp) << "Settings could not be saved before exit";
    if (m_window) {
        m_window->setExiting(true);
        m_window->close();
    }
    QApplication::quit();
}

void AppController::showPermissionsWindowIfNeeded()
{
    if (!m_availability->promptAtStartup() || !m_availability->anyRequiredPermissionMissing())
        return;
    qCInfo(lcUi) << "Required permissions are missing; showing the permissions window";
    m_permissionsDialog = new PermissionsDialog(*m_availability, m_window->isVisible() ? m_window.get() : nullptr);
    m_permissionsDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_permissionsDialog, &PermissionsDialog::restartRequested, this,
            [this] { relaunch(QApplication::applicationFilePath()); });
    m_permissionsDialog->show();
    m_permissionsDialog->raise();
    m_permissionsDialog->activateWindow();
    m_platform.desktop->activateApplication();
}

void AppController::relaunch(const QString &executable)
{
    qCInfo(lcApp).noquote() << "Restarting from" << executable;
    if (!m_settings.saveNow())
        qCCritical(lcApp) << "Settings could not be saved before restarting";
    // The new process must be able to become the primary instance.
    m_instance.release();
    const PlatformResult started = m_platform.desktop->launchApplication(executable, {});
    if (!started.ok()) {
        if (m_instance.acquire() != SingleInstance::Result::Primary)
            qCCritical(lcApp) << "Could not re-acquire the single-instance lock after a failed restart";
        showError(tr("Restart failed"), started.message);
        return;
    }
    exitApplication();
}

void AppController::reportSettingsLoad(const SettingsLoadResult &result)
{
    if (result.message.isEmpty())
        return;
    const bool severe = result.status == SettingsLoadResult::Status::RecoveredCorrupt
                        || result.status == SettingsLoadResult::Status::ReadError;
    if (severe)
        showError(tr("Settings problem"), result.message);
    else if (m_tray)
        m_tray->showMessage(tr("Settings"), result.message, QSystemTrayIcon::Warning);
}

void AppController::showError(const QString &title, const QString &message)
{
    qCWarning(lcUi).noquote() << "Shown to user:" << title << "-" << message;
    QMessageBox::warning(m_window && m_window->isVisible() ? m_window.get() : nullptr, title, message);
}

} // namespace ergo
