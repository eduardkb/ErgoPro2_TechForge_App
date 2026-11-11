#pragma once

#include <QWidget>

class QLabel;
class QSlider;
class QSpinBox;

namespace ergo {

// "Label  [-----o------] [ 30 ] days" row for numeric parameters.
// Slider and spin box stay in sync; valueChanged() is emitted immediately on
// every user change (the settings store debounces disk writes). The mouse wheel
// does not change the value; it scrolls the page (1.7.5).
class ParameterRow : public QWidget {
    Q_OBJECT
public:
    ParameterRow(const QString &label, int minimum, int maximum, const QString &unit,
                 QWidget *parent = nullptr);

    int value() const;
    // Sets the value without emitting valueChanged().
    void setValue(int value);

    void setHint(const QString &hint);

signals:
    void valueChanged(int value);

private:
    void onUserValue(int value);

    QSlider *m_slider = nullptr;
    QSpinBox *m_spin = nullptr;
    QLabel *m_hint = nullptr;
    bool m_updating = false;
};

} // namespace ergo
