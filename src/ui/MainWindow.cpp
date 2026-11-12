#include "ui/MainWindow.h"

#include "core/LogCategories.h"
#include "core/WindowLimits.h"
#include "ui/AppContext.h"
#include "ui/TabIcons.h"
#include "ui/Theme.h"
#include "ui/tabs/AutoClickTab.h"
#include "ui/tabs/BreakTimerTab.h"
#include "ui/tabs/GeneralTab.h"
#include "ui/tabs/KeyControlTab.h"
#include "ui/tabs/StatisticsTab.h"
#include "ui/widgets/FeaturePage.h"

#include <QApplication>
#include <QCloseEvent>
#include <QScreen>
#include <QTabWidget>
#include <QTimer>
#include <QWindow>

namespace ergo {

MainWindow::MainWindow(AppContext &context, QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("ErgoPro2 v%1").arg(QApplication::applicationVersion()));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app/app.svg")));
    // Resizable, but no maximize / full screen (1.7.8, owner request): only the
    // minimize and close buttons. Sizes: applySizeLimits().
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint
                   | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    applySizeLimits(QGuiApplication::primaryScreen());
    resize(windowLimitsFor(QGuiApplication::primaryScreen()
                               ? QGuiApplication::primaryScreen()->availableGeometry().size()
                               : QSize())
               .initial);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setUsesScrollButtons(false);

    m_general = new GeneralTab(context, m_tabs);

    // Tab order is fixed by the specification (B.1.1). Function tabs show the
    // availability banner (B.1.4) above their content.
    m_tabs->addTab(m_general, tr("General"));
    m_tabs->addTab(new FeaturePage(Feature::BreakTimerActivity, context.availability,
                                   new BreakTimerTab(context.breaktimer, context.settings)),
                   tr("Break Timer"));
    m_tabs->addTab(new FeaturePage(Feature::AutoClick, context.availability,
                                   new AutoClickTab(context.autoclick, context.settings, context.availability)),
                   tr("Auto Click"));
    m_tabs->addTab(new FeaturePage(Feature::KeyControl, context.availability,
                                   new KeyControlTab(context.keycontrol, context.input)),
                   tr("Key Control"));
    m_tabs->addTab(new FeaturePage(Feature::Statistics, context.availability,
                                   new StatisticsTab(context.statistics, context.settings)),
                   tr("Statistics"));

    setCentralWidget(m_tabs);

    // Icons on the tabs: accent color, black on the selected tab (1.4.1).
    m_tabs->setIconSize(QSize(18, 18));
    updateTabIcons();
    connect(&Theme::instance(), &Theme::changed, this, &MainWindow::updateTabIcons);

    connect(m_general, &GeneralTab::exitRequested, this, &MainWindow::exitRequested);
}

void MainWindow::applySizeLimits(QScreen *screen)
{
    const QSize available = screen ? screen->availableGeometry().size() : QSize();
    const WindowLimits limits = windowLimitsFor(available);
    setMinimumSize(limits.minimum);
    setMaximumSize(limits.maximum);
    if (const QSize bounded = limits.bounded(size()); bounded != size())
        resize(bounded);
    qCInfo(lcUi).noquote() << QStringLiteral("Main window size limits for screen %1 (%2x%3 usable): min %4x%5, max %6x%7")
                                  .arg(screen ? screen->name() : QStringLiteral("?"))
                                  .arg(available.width())
                                  .arg(available.height())
                                  .arg(limits.minimum.width())
                                  .arg(limits.minimum.height())
                                  .arg(limits.maximum.width())
                                  .arg(limits.maximum.height());
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QWindow *window = windowHandle();
    if (!window || window == m_trackedWindow)
        return;
    // Recalculate the limits when the window moves to another screen or the
    // screen's usable area changes (resolution, Dock / task bar).
    m_trackedWindow = window;
    const auto track = [this](QScreen *screen) {
        disconnect(m_screenConnection);
        if (screen)
            m_screenConnection = connect(screen, &QScreen::availableGeometryChanged, this,
                                         [this, screen] { applySizeLimits(screen); });
        applySizeLimits(screen);
    };
    connect(window, &QWindow::screenChanged, this, track);
    track(window->screen());
}

void MainWindow::updateTabIcons()
{
    // Same order as the tabs (B.1.1).
    static const char *const kIcons[] = {":/icons/ui/general.svg", ":/icons/ui/break.svg", ":/icons/ui/autoclick.svg",
                                         ":/icons/ui/keyboard.svg", ":/icons/ui/statistics.svg"};
    const QColor normal = Theme::instance().colors().accent;
    for (int i = 0; i < m_tabs->count() && i < int(std::size(kIcons)); ++i)
        m_tabs->setTabIcon(i, makeTabIcon(QString::fromLatin1(kIcons[i]), normal, Qt::black));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_exiting) {
        event->accept();
        return;
    }
    if (m_sessionEnding && m_sessionEnding()) {
        qCInfo(lcUi) << "Accepting window close because the OS session is ending";
        event->accept();
        return;
    }
    // The app only exits through the Exit button or the tray menu (B.1.2).
    event->ignore();
    if (m_hideToTray) {
        emit hideToTrayRequested();
    } else {
        qCWarning(lcUi) << "No system tray available: minimizing instead of hiding";
        showMinimized();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() != QEvent::WindowStateChange || m_exiting)
        return;

    if (windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
        // Maximize is not allowed (1.7.8); e.g. a window manager shortcut.
        qCInfo(lcUi) << "Main window maximize / full screen is not allowed: restoring the normal size";
        QTimer::singleShot(0, this, [this] { setWindowState(windowState() & ~(Qt::WindowMaximized | Qt::WindowFullScreen)); });
        return;
    }

    const auto *change = static_cast<QWindowStateChangeEvent *>(event);
    const bool wasMinimized = change->oldState() & Qt::WindowMinimized;
    const bool isMinimized = windowState() & Qt::WindowMinimized;

    if (isMinimized && !wasMinimized && m_hideToTray) {
        // Hide the window while it stays minimized; showMainWindow() restores it
        // with showNormal(). Do NOT clear the minimized state here: on macOS that
        // request is lost while the minimize animation runs, so AppKit kept the
        // window minimized in the Dock while Qt thought it was hidden and normal
        // (blank window ignoring the close button - fixed in 1.1.2).
        qCDebug(lcUi) << "Main window minimized: hiding it to the tray";
        QTimer::singleShot(0, this, [this] { emit hideToTrayRequested(); });
    } else if (wasMinimized && !isMinimized && !isVisible()) {
        // The OS un-minimized the hidden window by itself (macOS: click on the
        // Dock thumbnail). Show it through the normal path so Qt paints it and
        // handles its close button again.
        // Checked again after the event: showMainWindow()'s own showNormal() also
        // leaves the minimized state shortly before the window becomes visible.
        QTimer::singleShot(0, this, [this] {
            if (isVisible())
                return;
            qCInfo(lcUi) << "Hidden main window was restored by the OS (e.g. Dock thumbnail): showing it";
            emit showRequested();
        });
    }
}

} // namespace ergo
