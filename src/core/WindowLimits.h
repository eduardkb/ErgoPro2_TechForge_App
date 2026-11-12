#pragma once

#include <QSize>

namespace ergo {

// Size limits of the main window (B.1.1, 1.7.8, owner request): the window can
// be resized but never maximized and never larger than kMaxScreenFraction of
// the usable area of its screen. The minimum is kMinimumSize, reduced on screens
// too small for it; the start size is kInitialSize, reduced to fit.
struct WindowLimits {
    static constexpr double kMaxScreenFraction = 0.8;
    static constexpr QSize kMinimumSize{720, 560};
    static constexpr QSize kInitialSize{820, 620};

    QSize minimum;
    QSize maximum;
    QSize initial;

    // 'size' moved inside [minimum, maximum].
    QSize bounded(const QSize &size) const { return size.expandedTo(minimum).boundedTo(maximum); }
};

// 'available' = the screen's usable area (QScreen::availableGeometry().size()).
// An invalid size (no screen) gives the default sizes and no real maximum.
WindowLimits windowLimitsFor(const QSize &available);

} // namespace ergo
