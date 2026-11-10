#include "ui/autoclick/AutoClickOverlay.h"

#include "core/Clock.h"

#include <QLineF>
#include <QPainter>

#include <algorithm>

namespace ergo {

AutoClickOverlay::AutoClickOverlay()
    : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                           | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus
                           | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    setFixedSize(kSize, kSize);
    m_animation.setInterval(16); // ~60 fps while counting down
    connect(&m_animation, &QTimer::timeout, this, [this] {
        update();
        if (progress() >= 1.0) {
            m_animation.stop();
            hide(); // the click happens now
        }
    });
}

void AutoClickOverlay::setCountdown(bool active, const QPointF &position, bool positionKnown, qint64 startMs,
                                    qint64 deadlineMs)
{
    if (!active || !positionKnown) {
        m_animation.stop();
        hide();
        return;
    }
    m_startMs = startMs;
    m_deadlineMs = std::max(deadlineMs, startMs + 1);
    // Centered on the pointer tip (where the click will happen).
    move(int(position.x()) - kSize / 2, int(position.y()) - kSize / 2);
    if (!isVisible())
        show();
    m_animation.start();
    update();
}

double AutoClickOverlay::progress() const
{
    // Grows from when it appears (startMs) until the click (deadlineMs).
    return std::clamp(double(monotonicNowMs() - m_startMs) / double(m_deadlineMs - m_startMs), 0.0, 1.0);
}

void AutoClickOverlay::paintEvent(QPaintEvent *)
{
    // Orange X growing from the center outward; a thin white outline keeps it
    // visible on orange or dark backgrounds.
    const double arm = 1.5 + progress() * (kMaxArm - 1.5);
    const QPointF c(kSize / 2.0, kSize / 2.0);
    const QLineF a(c.x() - arm, c.y() - arm, c.x() + arm, c.y() + arm);
    const QLineF b(c.x() - arm, c.y() + arm, c.x() + arm, c.y() - arm);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPen outline(QColor(255, 255, 255, 220), 5.0);
    outline.setCapStyle(Qt::RoundCap);
    p.setPen(outline);
    p.drawLine(a);
    p.drawLine(b);
    QPen cross(QColor(0xF9, 0x73, 0x16), 3.0); // orange
    cross.setCapStyle(Qt::RoundCap);
    p.setPen(cross);
    p.drawLine(a);
    p.drawLine(b);
}

} // namespace ergo
