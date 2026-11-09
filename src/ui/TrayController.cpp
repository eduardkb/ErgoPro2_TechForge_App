#include "ui/TrayController.h"

#include "core/LogCategories.h"

#include <QAction>
#include <QApplication>
#include <QMenu>

namespace ergo {

TrayController::TrayController(bool monochromeIcons, QObject *parent)
    : QObject(parent)
    , m_monochrome(monochromeIcons)
{
    m_menu = new QMenu;

    QAction *showAction = m_menu->addAction(tr("Show window"));
    connect(showAction, &QAction::triggered, this, &TrayController::showWindowRequested);

    m_featureSeparator = m_menu->addSeparator();
    m_featureSeparator->setVisible(false); // shown once feature actions exist
    m_exitSeparator = m_menu->addSeparator();

    QAction *exitAction = m_menu->addAction(tr("Exit"));
    connect(exitAction, &QAction::triggered, this, &TrayController::exitRequested);

    m_tray = new QSystemTrayIcon(iconFor(m_state), this);
    m_tray->setContextMenu(m_menu);
    m_tray->setToolTip(tr("ErgoPro2 v%1").arg(QApplication::applicationVersion()));

    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            emit showWindowRequested();
    });
}

TrayController::~TrayController()
{
    m_tray->hide();
    delete m_menu; // the menu has no QObject parent (QSystemTrayIcon does not take ownership)
}

bool TrayController::isAvailable()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayController::show()
{
    m_tray->show();
}

void TrayController::setIconState(IconState state)
{
    if (state == m_state)
        return;
    m_state = state;
    m_tray->setIcon(iconFor(state));
}

QIcon TrayController::iconFor(IconState state) const
{
    QString path;
    switch (state) {
    case IconState::Normal:
        path = m_monochrome ? QStringLiteral(":/icons/tray/tray-mono.svg") : QStringLiteral(":/icons/tray/tray.svg");
        break;
    case IconState::AutoClickActive:
        path = QStringLiteral(":/icons/tray/tray-autoclick.svg");
        break;
    case IconState::DragLockActive:
        path = QStringLiteral(":/icons/tray/tray-draglock.svg");
        break;
    }
    QIcon icon(path);
    if (icon.isNull())
        qCCritical(lcUi) << "Tray icon resource missing:" << path;
    // macOS: a monochrome "template" icon adapts to light/dark menu bars.
    icon.setIsMask(m_monochrome && state == IconState::Normal);
    return icon;
}

void TrayController::addFeatureAction(QAction *action)
{
    m_menu->insertAction(m_exitSeparator, action);
    m_featureSeparator->setVisible(true);
}

void TrayController::showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon,
                                 int timeoutMs)
{
    m_tray->showMessage(title, message, icon, timeoutMs);
}

} // namespace ergo
