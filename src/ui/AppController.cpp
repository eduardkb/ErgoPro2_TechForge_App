#include "ui/AppController.h"

#include "core/LogCategories.h"
#include "core/LogLevel.h"
#include "core/Logger.h"
#include "core/StartupMode.h"
#include "core/settings/GeneralSettings.h"
#include "platform/PlatformFactory.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/InputService.h"
#include "ui/autoclick/AutoClickController.h"
#include "ui/breaktimer/BreakTimerController.h"
#include "ui/keycontrol/DragLockController.h"
#include "ui/keycontrol/KeyControlController.h"
#include "ui/InstallController.h"
#include "ui/MainWindow.h"
#include "ui/PermissionsDialog.h"
#include "ui/SingleInstance.h"
#include "ui/TrayController.h"

#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QSignalBlocker>

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

AppController::~AppController()
{
    if (m_breaktimer)
        m_breaktimer->shutdown();
    if (m_keycontrol)
        m_keycontrol->shutdown();
    if (m_autoclick)
        m_autoclick->shutdown();
    if (m_input)
        m_input->stop();
}

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
    m_input = std::make_unique<InputService>(m_platform, *m_availability);
    m_autoclick = std::make_unique<AutoClickController>(m_settings, *m_availability, *m_input, m_platform);
    m_keycontrol =
        std::make_unique<KeyControlController>(m_settings, *m_availability, *m_input, *m_autoclick, m_platform);
    m_breaktimer = std::make_unique<BreakTimerController>(m_settings, *m_availability, *m_input, m_platform);
    m_context = std::make_unique<AppContext>(AppContext{m_settings, m_logger, m_platform, *m_availability, *m_install,
                                                        *m_input, *m_autoclick, *m_keycontrol, *m_breaktimer});

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

    // Global input layer (stage 2); waits for missing permissions by itself.
    m_input->start();
    // Function 1 – AutoClick (stage 3).
    m_autoclick->start();
    setupAutoClickTrayEntry();
    // Function 2 – Key Control (stage 4).
    m_keycontrol->start();
    // Function 3 – Break Timer (stage 5).
    m_breaktimer->start();
    setupBreakTimerTrayEntries();
    // Drag Lock (stage 6): an active drag lock is released when the break window appears (D-06).
    connect(m_breaktimer.get(), &BreakTimerController::breakWindowShown, this,
            [this] { m_keycontrol->dragLock().release(DragLockController::Reason::BreakWindow); });
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
    if (m_breaktimer)
        m_breaktimer->shutdown();
    if (m_keycontrol)
        m_keycontrol->shutdown(); // release all hotkeys
    if (m_autoclick)
        m_autoclick->shutdown();
    if (m_input)
        m_input->stop(); // release hooks and key grabs before quitting
    if (!m_settings.saveNow())
        qCCritical(lcApp) << "Settings could not be saved before exit";
    if (m_window) {
        m_window->setExiting(true);
        m_window->close();
    }
    QApplication::quit();
}

void AppController::setupAutoClickTrayEntry()
{
    if (!m_tray)
        return;
    auto *action = new QAction(tr("AutoClick on/off"), m_tray.get());
    action->setCheckable(true);
    action->setChecked(m_autoclick->isEnabled());
    connect(action, &QAction::toggled, this, [this](bool on) { m_autoclick->setEnabled(on); });
    connect(m_autoclick.get(), &AutoClickController::enabledChanged, action, [action](bool on) {
        const QSignalBlocker block(action);
        action->setChecked(on);
    });
    m_tray->addFeatureAction(action);

    // Tray icon variant while AutoClick is active (A-09) or a drag lock holds
    // the left button (D-05; has priority).
    const auto updateIcon = [this] {
        TrayController::IconState state = TrayController::IconState::Normal;
        if (m_keycontrol->isDragLockHolding())
            state = TrayController::IconState::DragLockActive;
        else if (m_autoclick->isActive())
            state = TrayController::IconState::AutoClickActive;
        m_tray->setIconState(state);
    };
    connect(m_autoclick.get(), &AutoClickController::activeChanged, this, updateIcon);
    connect(&m_keycontrol->dragLock(), &DragLockController::holdingChanged, this, updateIcon);
    updateIcon();
}

void AppController::setupBreakTimerTrayEntries()
{
    if (!m_tray)
        return;
    auto *toggle = new QAction(tr("Break Timer on/off"), m_tray.get());
    toggle->setCheckable(true);
    toggle->setChecked(m_breaktimer->isEnabled());
    connect(toggle, &QAction::toggled, this, [this](bool on) { m_breaktimer->setEnabled(on); });
    connect(m_breaktimer.get(), &BreakTimerController::enabledChanged, toggle, [toggle](bool on) {
        const QSignalBlocker block(toggle);
        toggle->setChecked(on);
    });
    m_tray->addFeatureAction(toggle);

    auto *breakNow = new QAction(tr("Start break now"), m_tray.get());
    connect(breakNow, &QAction::triggered, m_breaktimer.get(), &BreakTimerController::startBreakNow);
    // Only while the timer runs and no break is shown.
    const auto updateBreakNow = [this, breakNow] {
        const auto s = m_breaktimer->state();
        breakNow->setEnabled(s != BreakTimerEngine::State::Disabled && s != BreakTimerEngine::State::BreakShown);
    };
    connect(m_breaktimer.get(), &BreakTimerController::ticked, breakNow, updateBreakNow);
    updateBreakNow();
    m_tray->addFeatureAction(breakNow);
}

void AppController::showPermissionsWindowIfNeeded()
{
    if (!m_availability->promptAtStartup() || !m_availability->anyOsPermissionMissing())
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
