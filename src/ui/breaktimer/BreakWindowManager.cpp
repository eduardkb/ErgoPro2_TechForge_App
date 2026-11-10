#include "ui/breaktimer/BreakWindowManager.h"

#include "core/LogCategories.h"
#include "platform/PlatformInterfaces.h"
#include "ui/breaktimer/BreakWindow.h"

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>

namespace ergo {

BreakWindowManager::BreakWindowManager(IDesktopIntegration &desktop, QObject *parent)
    : QObject(parent)
    , m_desktop(desktop)
{
}

BreakWindowManager::~BreakWindowManager()
{
    hideAll();
}

BreakWindow *BreakWindowManager::createFor(QScreen *screen, bool main)
{
    auto *w = new BreakWindow(main);
    w->setPostponeMinutes(m_postponeMinutes);
    w->setRemaining(m_remainingMs);
    connect(w, &BreakWindow::dismissClicked, this, &BreakWindowManager::dismissClicked);
    connect(w, &BreakWindow::postponeClicked, this, &BreakWindowManager::postponeClicked);
    w->winId(); // create the native window to place it on its screen
    w->windowHandle()->setScreen(screen);
    w->setGeometry(screen->geometry());
    if (m_desktop.overlayUsesFullScreenState())
        w->showFullScreen(); // X11/Windows: full-screen state keeps panels/taskbar below
    else
        w->show(); // macOS: screen-saver level window, no separate Space
    w->raise();
    m_desktop.raiseOverlayWindow(quintptr(w->winId()));
    return w;
}

void BreakWindowManager::rebuild()
{
    for (BreakWindow *w : std::as_const(m_windows)) {
        w->allowClose();
        w->close();
        w->deleteLater();
    }
    m_windows.clear();
    QScreen *primary = QGuiApplication::primaryScreen();
    for (QScreen *screen : QGuiApplication::screens())
        m_windows << createFor(screen, screen == primary);
    if (!m_windows.isEmpty())
        m_windows.first()->activateWindow();
    qCInfo(lcUi) << "Break window shown on" << m_windows.size() << "screen(s)";
}

void BreakWindowManager::showAll(qint64 remainingMs)
{
    m_remainingMs = remainingMs;
    if (m_shown)
        return;
    m_shown = true;
    rebuild();
    // Screens connected or disconnected during the break get/lose their overlay.
    m_screenConnections << connect(qApp, &QGuiApplication::screenAdded, this, [this] { rebuild(); });
    m_screenConnections << connect(qApp, &QGuiApplication::screenRemoved, this, [this] { rebuild(); });
    m_screenConnections << connect(qApp, &QGuiApplication::primaryScreenChanged, this, [this] { rebuild(); });
}

void BreakWindowManager::hideAll()
{
    for (const auto &c : std::as_const(m_screenConnections))
        disconnect(c);
    m_screenConnections.clear();
    for (BreakWindow *w : std::as_const(m_windows)) {
        w->allowClose();
        w->close();
        w->deleteLater();
    }
    if (m_shown)
        qCInfo(lcUi) << "Break window closed";
    m_windows.clear();
    m_shown = false;
}

void BreakWindowManager::setRemaining(qint64 ms)
{
    m_remainingMs = ms;
    for (BreakWindow *w : std::as_const(m_windows))
        w->setRemaining(ms);
}

void BreakWindowManager::setPostponeMinutes(int minutes)
{
    m_postponeMinutes = minutes;
    for (BreakWindow *w : std::as_const(m_windows))
        w->setPostponeMinutes(minutes);
}

} // namespace ergo
