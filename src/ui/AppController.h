#pragma once

#include "core/settings/SettingsStore.h"

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <memory>

namespace ergo {

class Logger;
class MainWindow;
class SingleInstance;
class TrayController;
struct Platform;

struct LaunchOptions {
    bool autostart = false; // started by the OS at login (--autostart)
};

// Owns the main window and the tray, and coordinates show/hide/exit,
// settings side effects (logging) and user notifications.
class AppController : public QObject {
    Q_OBJECT
public:
    AppController(SettingsStore &settings, Logger &logger, Platform &platform, SingleInstance &instance,
                  const LaunchOptions &options, QObject *parent = nullptr);
    ~AppController() override;

    void start(const SettingsLoadResult &loadResult);

public slots:
    void showMainWindow();
    void hideMainWindowToTray();
    void exitApplication();

private:
    void applySetting(const QString &key);
    void reportSettingsLoad(const SettingsLoadResult &result);
    void showError(const QString &title, const QString &message);

    SettingsStore &m_settings;
    Logger &m_logger;
    Platform &m_platform;
    SingleInstance &m_instance;
    LaunchOptions m_options;

    std::unique_ptr<MainWindow> m_window;
    std::unique_ptr<TrayController> m_tray;
    QTimer m_maintenanceTimer;
    bool m_exiting = false;
};

} // namespace ergo
