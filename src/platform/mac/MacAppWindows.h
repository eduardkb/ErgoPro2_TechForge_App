#pragma once

#include "platform/PlatformInterfaces.h"

#include <QCoreApplication>

namespace ergo {

// Keep Screen Active on macOS (B.10): NSRunningApplication + Accessibility API
// to restore / raise the Teams or Zoom window, CGWindowList for its frame,
// CGEventPost (tagged synthetic) for the click and the pointer nudge.
class MacAppWindows final : public IAppWindows {
    Q_DECLARE_TR_FUNCTIONS(ergo::MacAppWindows)
public:
    QString unsupportedReason() const override { return {}; }
    Raise raise(KeepActiveApp app, QString *detail) override;
    std::optional<QRectF> mainWindowFrame(KeepActiveApp app) const override;
    PlatformResult clickAt(const QPointF &point) override;
    PlatformResult nudgePointer() override;
};

} // namespace ergo
