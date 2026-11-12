#pragma once

#include <QMainWindow>

#include <functional>

class QScreen;
class QTabWidget;
class QWindow;

namespace ergo {

class GeneralTab;
struct AppContext;

// Main window with the five tabs. Closing or minimizing hides it to the tray
// (decided by AppController through setHideToTrayEnabled()).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppContext &context, QWidget *parent = nullptr);

    // When enabled, close/minimize emit hideToTrayRequested() instead of
    // closing/minimizing. Disabled when no system tray is available.
    void setHideToTrayEnabled(bool enabled) { m_hideToTray = enabled; }

    // Allows the real close during application exit.
    void setExiting(bool exiting) { m_exiting = exiting; }

    // Returns true when a close request must be accepted because the OS
    // session is ending (logout/shutdown must never be blocked).
    void setSessionEndingCheck(std::function<bool()> check) { m_sessionEnding = std::move(check); }

signals:
    void hideToTrayRequested();
    // The OS restored the hidden, minimized window by itself (e.g. a click on
    // its macOS Dock thumbnail): the controller must show it properly.
    void showRequested();
    void exitRequested();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void updateTabIcons();
    // Min/max size for the usable area of 'screen' (B.1.1, 1.7.8).
    void applySizeLimits(QScreen *screen);

    QTabWidget *m_tabs = nullptr;
    GeneralTab *m_general = nullptr;
    QWindow *m_trackedWindow = nullptr;
    QMetaObject::Connection m_screenConnection;
    bool m_hideToTray = true;
    bool m_exiting = false;
    std::function<bool()> m_sessionEnding;
};

} // namespace ergo
