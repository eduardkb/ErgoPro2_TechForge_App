#include "ui/widgets/WheelBlocker.h"

#include <QEvent>
#include <QObject>
#include <QWidget>

namespace ergo {

namespace {

class WheelFilter final : public QObject {
public:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() != QEvent::Wheel)
            return false;
        // Swallowed here but not accepted: Qt then delivers the wheel event to
        // the parent widgets (e.g. the scroll area of the tab).
        event->ignore();
        return true;
    }
};

WheelFilter &filter()
{
    static WheelFilter instance; // lives until the end of the program
    return instance;
}

} // namespace

void blockWheelValueChanges(QWidget *widget)
{
    if (!widget)
        return;
    widget->installEventFilter(&filter());
    if (widget->focusPolicy() == Qt::WheelFocus)
        widget->setFocusPolicy(Qt::StrongFocus);
}

} // namespace ergo
