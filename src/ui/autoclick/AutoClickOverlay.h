#pragma once

#include <QTimer>
#include <QWidget>

namespace ergo {

// Optional AutoClick visual feedback (S-12, B.4): a small translucent ring
// next to the pointer that fills during the countdown. Frameless, always on
// top, click-through (transparent for input) and never takes focus.
class AutoClickOverlay : public QWidget {
    Q_OBJECT
public:
    static constexpr int kSize = 34;
    static constexpr int kOffset = 14; // from the pointer, down-right

    AutoClickOverlay();

public slots:
    void setCountdown(bool active, const QPointF &position, bool positionKnown, qint64 deadlineMs, int totalMs);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double progress() const;

    QTimer m_animation;
    qint64 m_deadlineMs = 0;
    int m_totalMs = 1;
};

} // namespace ergo
