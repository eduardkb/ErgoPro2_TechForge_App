#pragma once

#include <QString>
#include <QStringList>
#include <optional>

namespace ergo {

enum class LogLevel { Debug, Info, Warning, Error, Critical };

// Cumulative log level selection offered to the user (implementation_stages.md B.3).
// The order is the owner's choice (S-06): Info is always written, then Warning,
// Error, Critical and finally Debug are added.
enum class LogLevelSelection {
    InfoOnly,
    InfoWarning,
    InfoWarningError,
    InfoWarningErrorCritical, // default
    All                       // Info + Warning + Error + Critical + Debug
};

inline constexpr LogLevelSelection kDefaultLogLevelSelection = LogLevelSelection::InfoWarningErrorCritical;

bool isLevelEnabled(LogLevelSelection selection, LogLevel level);

QString logLevelName(LogLevel level);

// Stable identifiers stored in settings.json (e.g. "info_warning_error_critical").
QString logLevelSelectionId(LogLevelSelection selection);
std::optional<LogLevelSelection> logLevelSelectionFromId(const QString &id);
QStringList allLogLevelSelectionIds();

} // namespace ergo
