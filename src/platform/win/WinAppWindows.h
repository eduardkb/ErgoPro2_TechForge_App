#pragma once

#include "platform/PlatformInterfaces.h"

#include <QCoreApplication>

namespace ergo {

// Keep Screen Active on Windows (B.10): finds the Teams / Zoom top-level window
// by process name (EnumWindows), restores and raises it, and clicks / moves the
// pointer with SendInput tagged synthetic (never SetCursorPos, whose moves the
// hook could not tell apart from the user's).
class WinAppWindows final : public IAppWindows {
    Q_DECLARE_TR_FUNCTIONS(ergo::WinAppWindows)
public:
    QString unsupportedReason() const override { return {}; }
    Raise raise(KeepActiveApp app, QString *detail) override;
    std::optional<QRectF> mainWindowFrame(KeepActiveApp app) const override;
    PlatformResult clickAt(const QPointF &point) override;
    PlatformResult nudgePointer() override;
};

} // namespace ergo
