#pragma once

#include <QIcon>
#include <QObject>
#include <QSystemTrayIcon>

class QAction;
class QMenu;

namespace ergo {

// System tray / macOS menu bar icon and its menu.
//
// Menu layout:   Show window
//                ----------
//                <feature actions added by later stages via addFeatureAction()>
//                ----------
//                Exit
class TrayController : public QObject {
    Q_OBJECT
public:
    enum class IconState { Normal, AutoClickActive, DragLockActive };

    explicit TrayController(bool monochromeIcons, QObject *parent = nullptr);
    ~TrayController() override;

    static bool isAvailable();

    void show();
    void setIconState(IconState state);
    IconState iconState() const { return m_state; }

    // Extension point for later stages (AutoClick on/off, Break Timer on/off,
    // Start break now). Actions are inserted above the Exit entry.
    void addFeatureAction(QAction *action);

    void showMessage(const QString &title, const QString &message,
                     QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information, int timeoutMs = 6000);

signals:
    void showWindowRequested();
    void exitRequested();

private:
    QIcon iconFor(IconState state) const;

    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_featureSeparator = nullptr;
    QAction *m_exitSeparator = nullptr;
    bool m_monochrome = false;
    IconState m_state = IconState::Normal;
};

} // namespace ergo
