#pragma once

class QWidget;

namespace ergo {

// The mouse wheel / trackpad scroll never changes the value of a slider, spin
// box or drop-down (1.7.5, owner request): the wheel event is passed on to the
// parent widgets, so the page scrolls instead. Values change only by clicking,
// dragging, typing or opening the drop-down. Also stops the wheel from giving
// the widget the keyboard focus.
void blockWheelValueChanges(QWidget *widget);

} // namespace ergo
