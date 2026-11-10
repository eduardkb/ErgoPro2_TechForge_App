#include "ui/MainWindow.h"

#include "core/LogCategories.h"
#include "ui/AppContext.h"
#include "ui/TabIcons.h"
#include "ui/Theme.h"
#include "ui/tabs/AutoClickTab.h"
#include "ui/tabs/GeneralTab.h"
#include "ui/tabs/KeyControlTab.h"
#include "ui/widgets/FeaturePage.h"
#include "ui/widgets/PlaceholderPage.h"

#include <QApplication>
#include <QCloseEvent>
#include <QTabWidget>
#include <QTimer>

namespace ergo {

MainWindow::MainWindow(AppContext &context, QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("ErgoPro2 v%1").arg(QApplication::applicationVersion()));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app/app.svg")));
    setMinimumSize(720, 560);
    resize(820, 620);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setUsesScrollButtons(false);

    m_general = new GeneralTab(context, m_tabs);
    const QString later = tr("Coming in a later version.");

    // Tab order is fixed by the specification (B.1.1). Function tabs show the
    // availability banner (B.1.4) above their content.
    m_tabs->addTab(m_general, tr("General"));
    m_tabs->addTab(new FeaturePage(Feature::BreakTimerActivity, context.availability,
                                   new PlaceholderPage(QStringLiteral(":/icons/ui/break.svg"), tr("Break Timer"),
                                                       tr("Reminds you to rest after a period of continuous work.\n%1")
                                                           .arg(later))),
                   tr("Break Timer"));
    m_tabs->addTab(new FeaturePage(Feature::AutoClick, context.availability,
                                   new AutoClickTab(context.autoclick, context.settings, context.availability)),
                   tr("Auto Click"));
    m_tabs->addTab(new FeaturePage(Feature::KeyControl, context.availability,
                                   new KeyControlTab(context.keycontrol, context.input)),
                   tr("Key Control"));
    m_tabs->addTab(new PlaceholderPage(QStringLiteral(":/icons/ui/statistics.svg"), tr("Statistics"),
                                       tr("Coming soon."), m_tabs),
                   tr("Statistics"));

    setCentralWidget(m_tabs);

    // Icons on the tabs: accent color, black on the selected tab (1.4.1).
    m_tabs->setIconSize(QSize(18, 18));
    updateTabIcons();
    connect(&Theme::instance(), &Theme::changed, this, &MainWindow::updateTabIcons);

    connect(m_general, &GeneralTab::exitRequested, this, &MainWindow::exitRequested);
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
