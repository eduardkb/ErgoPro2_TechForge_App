#include "ui/autoclick/AutoClickOverlay.h"

#include "core/Clock.h"
#include "ui/Theme.h"

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
            QTimer::singleShot(120, this, &QWidget::hide); // the click happens now
        }
    });
}

void AutoClickOverlay::setCountdown(bool active, const QPointF &position, bool positionKnown, qint64 deadlineMs,
                                    int totalMs)
{
    if (!active || !positionKnown) {
        m_animation.stop();
        hide();
        return;
    }
    m_deadlineMs = deadlineMs;
    m_totalMs = std::max(1, totalMs);
    move(int(position.x()) + kOffset, int(position.y()) + kOffset);
    if (!isVisible())
        show();
    m_animation.start();
    update();
}

double AutoClickOverlay::progress() const
{
    const qint64 remaining = m_deadlineMs - monotonicNowMs();
    return std::clamp(1.0 - double(remaining) / m_totalMs, 0.0, 1.0);
}

void AutoClickOverlay::paintEvent(QPaintEvent *)
{
    const ThemeColors &c = Theme::instance().colors();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF ring = QRectF(rect()).adjusted(4, 4, -4, -4);

    QColor track = c.accent;
    track.setAlphaF(0.25);
    p.setPen(QPen(track, 4));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(ring);

    QPen arc(c.accent, 4);
    arc.setCapStyle(Qt::RoundCap);
    p.setPen(arc);
    const int span = int(-progress() * 360 * 16); // clockwise from 12 o'clock
    p.drawArc(ring, 90 * 16, span);
}

} // namespace ergo
