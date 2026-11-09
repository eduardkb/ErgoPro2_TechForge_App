#pragma once

#include <QMainWindow>

#include <functional>

class QTabWidget;

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
    void exitRequested();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    QTabWidget *m_tabs = nullptr;
    GeneralTab *m_general = nullptr;
    bool m_hideToTray = true;
    bool m_exiting = false;
    std::function<bool()> m_sessionEnding;
};

} // namespace ergo
