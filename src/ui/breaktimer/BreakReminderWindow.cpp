#include "ui/breaktimer/BreakReminderWindow.h"

#include "core/LogCategories.h"
#include "core/breaktimer/BreakReminder.h"

#include <QGuiApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QScreen>
#include <QVBoxLayout>

namespace ergo {

namespace {
constexpr int kMargin = 12;

QString countdownText(qint64 ms)
{
    const qint64 s = (ms + 999) / 1000; // 0.2 s left still reads 0:01
    return QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
}
} // namespace

BreakReminderWindow::BreakReminderWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
{
    setObjectName(QStringLiteral("breakReminder"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true); // never steals the keyboard focus
    // 2.5.0: macOS hides a tool window while its application is not active,
    // and ErgoPro2 almost never is; keep it visible.
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setWindowTitle(tr("Break reminder"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(2);
    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("cardTitle"));
    QFont f = m_title->font();
    f.setPointSizeF(f.pointSizeF() * 1.3);
    f.setBold(true);
    m_title->setFont(f);
    m_hint = new QLabel(this);
    m_hint->setObjectName(QStringLiteral("hint"));
    layout->addWidget(m_title);
    layout->addWidget(m_hint);
    setFixedWidth(280);
}

void BreakReminderWindow::showReminder(qint64 remainingMs, bool paused, const QRect &trayIcon)
{
    m_title->setText(paused ? tr("Break in %1 – paused").arg(countdownText(remainingMs))
                            : tr("Break in %1").arg(countdownText(remainingMs)));
    m_hint->setText(paused ? tr("Continues when you use the mouse or keyboard. Click to hide.")
                           : tr("Click to hide this reminder."));
    if (isVisible())
        return;
    adjustSize();
    QScreen *screen = nullptr;
    if (trayIcon.isValid())
        screen = QGuiApplication::screenAt(trayIcon.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qCWarning(lcUi) << "Break reminder: no screen to show it on";
        return;
    }
    const QPoint pos = breakReminderPosition(screen->geometry(), screen->availableGeometry(), size(), kMargin, trayIcon);
    move(pos);
    show();
    raise();
    qCInfo(lcUi) << "Break reminder shown at" << pos << "on" << screen->name() << "-" << remainingMs / 1000
                 << "s until the break";
}

void BreakReminderWindow::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    emit clicked();
}

} // namespace ergo
