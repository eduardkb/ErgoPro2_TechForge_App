#include "ui/breaktimer/MicroBreakWindow.h"

#include "core/LogCategories.h"
#include "core/breaktimer/BreakReminder.h"

#include <QGuiApplication>
#include <QHBoxLayout>
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

MicroBreakWindow::MicroBreakWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
{
    setObjectName(QStringLiteral("microBreak"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true); // never steals the keyboard focus
    // macOS: a tool window hides while its application is not active, and
    // ErgoPro2 almost never is; keep it visible.
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setWindowTitle(tr("Micro-break"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 12, 8);
    layout->setSpacing(4);
    auto *title = new QLabel(tr("Time for a micro-break"), this);
    title->setObjectName(QStringLiteral("cardTitle"));
    QFont f = title->font();
    f.setPointSizeF(f.pointSizeF() * 1.3);
    f.setBold(true);
    title->setFont(f);
    m_hint = new QLabel(this);
    m_hint->setObjectName(QStringLiteral("hint"));
    m_hint->setWordWrap(true);
    m_countdown = new QLabel(this);
    m_countdown->setObjectName(QStringLiteral("valueLabel"));
    auto *bottom = new QHBoxLayout;
    auto *clickHint = new QLabel(tr("Click to hide."), this);
    clickHint->setObjectName(QStringLiteral("hint"));
    bottom->addWidget(clickHint);
    bottom->addStretch(1);
    bottom->addWidget(m_countdown); // bottom-right corner (owner request)
    layout->addWidget(title);
    layout->addWidget(m_hint);
    layout->addLayout(bottom);
    setFixedWidth(300);
}

void MicroBreakWindow::showMicroBreak(qint64 remainingMs, int lengthSeconds, const QRect &trayIcon)
{
    m_hint->setText(tr("Take %n seconds: look away from the screen and relax your hands and shoulders.", nullptr,
                       lengthSeconds));
    m_countdown->setText(countdownText(remainingMs));
    if (isVisible()) {
        raise(); // stays on top of everything until it closes
        return;
    }
    adjustSize();
    QScreen *screen = nullptr;
    if (trayIcon.isValid())
        screen = QGuiApplication::screenAt(trayIcon.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qCWarning(lcUi) << "Micro-break: no screen to show the window on";
        return;
    }
    const QPoint pos = breakReminderPosition(screen->geometry(), screen->availableGeometry(), size(), kMargin, trayIcon);
    move(pos);
    show();
    raise();
    qCInfo(lcUi) << "Micro-break window shown at" << pos << "on" << screen->name() << "for" << remainingMs / 1000
                 << "s";
}

void MicroBreakWindow::mousePressEvent(QMouseEvent *event)
{
    event->accept();
    emit clicked();
}

} // namespace ergo
