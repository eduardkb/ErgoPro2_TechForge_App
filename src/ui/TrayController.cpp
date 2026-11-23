#include "ui/TrayController.h"

#include "core/LogCategories.h"

#include <QAction>
#include <QColor>
#include <QApplication>
#include <QGuiApplication>
#include <QMenu>
#include <QPainter>
#include <QStyleHints>

#include <algorithm>

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

    // A tinted monochrome icon (badge shown) follows the light / dark menu bar.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        if (m_keepActiveBadge && m_monochrome)
            refreshIcon();
    });
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
    refreshIcon();
}

void TrayController::setKeepActiveBadge(bool shown)
{
    if (shown == m_keepActiveBadge)
        return;
    m_keepActiveBadge = shown;
    qCInfo(lcUi) << "Tray icon: Keep screen active badge" << (shown ? "shown" : "removed");
    refreshIcon();
}

void TrayController::refreshIcon()
{
    const QIcon base = iconFor(m_state);
    m_tray->setIcon(m_keepActiveBadge ? withBadge(base, base.isMask()) : base);
}

// 2.3.1: the base icon with a green dot and a white ring in the bottom-right
// corner, rendered for the usual tray sizes (1x and 2x). A macOS template
// (mask) icon cannot carry color, so it is drawn in the menu bar's text color
// (black in light mode, white in dark mode) instead (owner decision).
QIcon TrayController::withBadge(const QIcon &base, bool tintMask) const
{
    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    QIcon icon;
    for (int size : {16, 18, 20, 22, 24, 32, 36, 40, 44, 48, 64}) {
        // Exactly size x size pixels (device pixel ratio 1), so the badge
        // coordinates below are pixels.
        QPixmap pixmap = base.pixmap(QSize(size, size), 1.0);
        if (pixmap.isNull())
            continue;
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        if (tintMask) {
            p.setCompositionMode(QPainter::CompositionMode_SourceIn);
            p.fillRect(pixmap.rect(), dark ? Qt::white : Qt::black);
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        }
        const qreal d = size * 0.42; // badge diameter
        const qreal ring = std::max<qreal>(1.0, size * 0.07);
        const QRectF badge(size - d, size - d, d, d);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(badge);
        p.setBrush(QColor(0x22, 0xa4, 0x47)); // green
        p.drawEllipse(badge.adjusted(ring, ring, -ring, -ring));
        p.end();
        icon.addPixmap(pixmap);
    }
    icon.setIsMask(false);
    return icon;
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

QRect TrayController::iconGeometry() const
{
    return m_tray ? m_tray->geometry() : QRect();
}

} // namespace ergo
