#pragma once

#include <QTimer>
#include <QWidget>

namespace ergo {

// Optional AutoClick visual feedback (S-12, B.4, changed in 1.4.1/1.4.2): an
// orange X centered on the pointer tip. It appears once the pointer has been
// completely still for half the trigger time and grows outward until the
// click, up to about the size of the mouse cursor. Frameless, always
// on top, click-through (transparent for input) and never takes focus.
class AutoClickOverlay : public QWidget {
    Q_OBJECT
public:
    static constexpr int kSize = 32;       // window, centered on the pointer
    static constexpr double kMaxArm = 10;  // final half-size of the X (=> 20 x 20 px)

    AutoClickOverlay();

public slots:
    void setCountdown(bool active, const QPointF &position, bool positionKnown, qint64 startMs, qint64 deadlineMs);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double progress() const;

    QTimer m_animation;
    qint64 m_startMs = 0;
    qint64 m_deadlineMs = 1;
};

} // namespace ergo
