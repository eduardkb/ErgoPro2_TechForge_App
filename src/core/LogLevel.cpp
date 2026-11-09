#include "core/LogLevel.h"

#include <array>

namespace ergo {

namespace {

struct SelectionInfo {
    LogLevelSelection selection;
    const char *id;
};

constexpr std::array<SelectionInfo, 5> kSelections{{
    {LogLevelSelection::InfoOnly, "info"},
    {LogLevelSelection::InfoWarning, "info_warning"},
    {LogLevelSelection::InfoWarningError, "info_warning_error"},
    {LogLevelSelection::InfoWarningErrorCritical, "info_warning_error_critical"},
    {LogLevelSelection::All, "info_warning_error_critical_debug"},
}};

} // namespace

bool isLevelEnabled(LogLevelSelection selection, LogLevel level)
{
    switch (level) {
    case LogLevel::Info:
        return true;
    case LogLevel::Warning:
        return selection >= LogLevelSelection::InfoWarning;
    case LogLevel::Error:
        return selection >= LogLevelSelection::InfoWarningError;
    case LogLevel::Critical:
        return selection >= LogLevelSelection::InfoWarningErrorCritical;
    case LogLevel::Debug:
        return selection == LogLevelSelection::All;
    }
    return true;
}

QString logLevelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return QStringLiteral("DEBUG");
    case LogLevel::Info:
        return QStringLiteral("INFO");
    case LogLevel::Warning:
        return QStringLiteral("WARNING");
    case LogLevel::Error:
        return QStringLiteral("ERROR");
    case LogLevel::Critical:
        return QStringLiteral("CRITICAL");
    }
    return QStringLiteral("UNKNOWN");
}

QString logLevelSelectionId(LogLevelSelection selection)
{
    for (const auto &info : kSelections) {
        if (info.selection == selection)
            return QString::fromLatin1(info.id);
    }
    return QString::fromLatin1(kSelections[3].id);
}

std::optional<LogLevelSelection> logLevelSelectionFromId(const QString &id)
{
    for (const auto &info : kSelections) {
        if (id == QLatin1String(info.id))
            return info.selection;
    }
    return std::nullopt;
}

QStringList allLogLevelSelectionIds()
{
    QStringList ids;
    for (const auto &info : kSelections)
        ids << QString::fromLatin1(info.id);
    return ids;
}

} // namespace ergo
