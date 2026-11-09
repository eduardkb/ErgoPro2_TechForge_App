#include "ui/widgets/ToggleSwitch.h"

#include "ui/Theme.h"

#include <QPainter>
#include <QPropertyAnimation>

namespace ergo {

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QAbstractButton(parent)
    , m_animation(new QPropertyAnimation(this, "knobPosition", this))
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    m_animation->setDuration(140);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(&Theme::instance(), &Theme::changed, this, qOverload<>(&QWidget::update));
}

QSize ToggleSwitch::sizeHint() const
{
    return {44, 24};
}

void ToggleSwitch::setKnobPosition(qreal position)
{
    m_knobPosition = position;
    update();
}

void ToggleSwitch::checkStateSet()
{
    // Programmatic setChecked(): jump without animation.
    m_animation->stop();
    setKnobPosition(isChecked() ? 1.0 : 0.0);
}

void ToggleSwitch::nextCheckState()
{
    QAbstractButton::nextCheckState();
    animateTo(isChecked());
}

bool ToggleSwitch::hitButton(const QPoint &pos) const
{
    return rect().contains(pos);
}

void ToggleSwitch::animateTo(bool checked)
{
    m_animation->stop();
    m_animation->setStartValue(m_knobPosition);
    m_animation->setEndValue(checked ? 1.0 : 0.0);
    m_animation->start();
}

void ToggleSwitch::paintEvent(QPaintEvent *)
{
    const ThemeColors &c = Theme::instance().colors();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF track = QRectF(rect()).adjusted(1, 1, -1, -1);
    const qreal radius = track.height() / 2.0;

    // Track color blends from off (border) to on (accent).
    const QColor off = c.border;
    const QColor on = c.accent;
    const qreal t = m_knobPosition;
    QColor trackColor = QColor::fromRgbF(off.redF() + (on.redF() - off.redF()) * t,
                                         off.greenF() + (on.greenF() - off.greenF()) * t,
                                         off.blueF() + (on.blueF() - off.blueF()) * t);
    if (!isEnabled())
        trackColor.setAlphaF(0.45);

    p.setPen(Qt::NoPen);
    p.setBrush(trackColor);
    p.drawRoundedRect(track, radius, radius);

    const qreal margin = 3.0;
    const qreal knobSize = track.height() - 2 * margin;
    const qreal x = track.left() + margin + (track.width() - knobSize - 2 * margin) * t;
    p.setBrush(isEnabled() ? QColor(Qt::white) : QColor(220, 220, 220));
    p.drawEllipse(QRectF(x, track.top() + margin, knobSize, knobSize));

    if (hasFocus()) {
        QPen focusPen(c.accent);
        focusPen.setWidthF(1.5);
        p.setPen(focusPen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(track.adjusted(-0.5, -0.5, 0.5, 0.5), radius, radius);
    }
}

} // namespace ergo
