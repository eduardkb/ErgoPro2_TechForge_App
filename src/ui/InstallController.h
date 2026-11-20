#pragma once

#include "core/LocationClassifier.h"
#include "platform/PlatformInterfaces.h"

#include <QObject>

class QWidget;

namespace ergo {

class SettingsStore;
struct Platform;

// Install / Uninstall workflow of the General tab (B.1.3): copies the app to
// the permanent per-user folder, registers start at login (pointing to the
// installed copy, with --autostart), offers to restart from the installed copy,
// and removes everything again on Uninstall.
class InstallController : public QObject {
    Q_OBJECT
public:
    struct Status {
        InstallState install;
        bool startsAtLogin = false;
        QString autostartLocation;
        LocationCheck location; // of the running copy
    };

    InstallController(Platform &platform, SettingsStore &settings, QObject *parent = nullptr);

    Status status() const;

    // Both show their own confirmations/errors with 'parent' as dialog parent.
    void install(QWidget *parent);
    void uninstall(QWidget *parent);

    // At every start (2.0.0, macOS drag-and-drop install): when running from
    // the installed copy, register start at login for it (or move it from an
    // older copy) and offer once to remove an older copy (~/Applications).
    void checkAtStartup(QWidget *parent);

signals:
    void changed();
    // Ask the application to restart from 'executable' (new process) and exit.
    void relaunchRequested(const QString &executable);
    // Ask the application to exit (uninstall of the running installed copy).
    void exitRequested();

private:
    Platform &m_platform;
    SettingsStore &m_settings;
};

} // namespace ergo
