#include "core/WindowLimits.h"

#include <cmath>

namespace ergo {

namespace {
constexpr int kNoLimit = (1 << 24) - 1; // = QWIDGETSIZE_MAX (core does not use QtWidgets)
}

WindowLimits windowLimitsFor(const QSize &available)
{
    WindowLimits l;
    if (!available.isValid() || available.isEmpty()) {
        l.minimum = WindowLimits::kMinimumSize;
        l.initial = WindowLimits::kInitialSize;
        l.maximum = QSize(kNoLimit, kNoLimit);
        return l;
    }
    l.maximum = QSize(int(std::floor(available.width() * WindowLimits::kMaxScreenFraction)),
                      int(std::floor(available.height() * WindowLimits::kMaxScreenFraction)));
    l.minimum = WindowLimits::kMinimumSize.boundedTo(l.maximum);
    l.initial = l.bounded(WindowLimits::kInitialSize);
    return l;
}

} // namespace ergo
