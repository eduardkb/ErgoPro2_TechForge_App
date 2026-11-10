#pragma once

#include <QList>
#include <QMetaObject>
#include <QObject>

class QScreen;

namespace ergo {

class BreakWindow;
class IDesktopIntegration;

// Shows one BreakWindow per screen (the full content on the primary screen)
// and keeps them in sync while screens are added or removed (B.7).
class BreakWindowManager : public QObject {
    Q_OBJECT
public:
    explicit BreakWindowManager(IDesktopIntegration &desktop, QObject *parent = nullptr);
    ~BreakWindowManager() override;

    void showAll(qint64 remainingMs);
    void hideAll();
    bool isShown() const { return m_shown; }
    void setRemaining(qint64 ms);
    void setPostponeMinutes(int minutes);

signals:
    void dismissClicked();
    void postponeClicked();

private:
    void rebuild();
    BreakWindow *createFor(QScreen *screen, bool main);

    IDesktopIntegration &m_desktop;
    QList<BreakWindow *> m_windows;
    QList<QMetaObject::Connection> m_screenConnections;
    bool m_shown = false;
    qint64 m_remainingMs = 0;
    int m_postponeMinutes = 10;
};

} // namespace ergo
