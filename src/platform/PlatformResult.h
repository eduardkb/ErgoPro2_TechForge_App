#pragma once

#include <QString>

namespace ergo {

// Result of a platform operation. Never ignore a failed result: log it and,
// when relevant to the user, show it in the UI.
struct PlatformResult {
    enum class Code { Ok, NotImplemented, NotSupported, PermissionDenied, Failed };

    Code code = Code::Ok;
    QString message;

    bool ok() const { return code == Code::Ok; }

    static PlatformResult success() { return {}; }
    static PlatformResult notImplemented(const QString &what)
    {
        return {Code::NotImplemented, QStringLiteral("%1 is not implemented yet on this OS").arg(what)};
    }
    static PlatformResult notSupported(const QString &reason) { return {Code::NotSupported, reason}; }
    static PlatformResult permissionDenied(const QString &reason) { return {Code::PermissionDenied, reason}; }
    static PlatformResult failed(const QString &reason) { return {Code::Failed, reason}; }
};

} // namespace ergo
