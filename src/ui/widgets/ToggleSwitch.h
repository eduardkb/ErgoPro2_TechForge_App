#pragma once

#include <QAbstractButton>

class QPropertyAnimation;

namespace ergo {

// Modern on/off switch (checkable). Emits toggled(bool) like any button.
class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal knobPosition READ knobPosition WRITE setKnobPosition)
public:
    explicit ToggleSwitch(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    qreal knobPosition() const { return m_knobPosition; }
    void setKnobPosition(qreal position);

protected:
    void paintEvent(QPaintEvent *event) override;
    void checkStateSet() override;
    void nextCheckState() override;
    bool hitButton(const QPoint &pos) const override;

private:
    void animateTo(bool checked);

    qreal m_knobPosition = 0.0; // 0 = off, 1 = on
    QPropertyAnimation *m_animation = nullptr;
};

} // namespace ergo
