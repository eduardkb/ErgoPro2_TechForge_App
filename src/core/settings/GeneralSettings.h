#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "general" section (implementation_stages.md B.2).
namespace GeneralSettings {

// Allowed values of the startup mode settings.
inline const QString StartupTray = QStringLiteral("tray");
inline const QString StartupWindow = QStringLiteral("window");

inline const QString LogLevel = QStringLiteral("general.logLevel");
inline const QString LogRetentionDays = QStringLiteral("general.logRetentionDays");
inline const QString TrayHintShown = QStringLiteral("general.trayHintShown");
// macOS (2.0.0): the user chose to keep an older copy in ~/Applications; not asked again.
inline const QString OlderCopyKept = QStringLiteral("general.olderCopyKept");
inline const QString StartupAutoMode = QStringLiteral("general.startupAutoMode");     // stage 1
inline const QString StartupManualMode = QStringLiteral("general.startupManualMode"); // stage 1

void registerSettings(SettingsSchema &schema);

} // namespace GeneralSettings
} // namespace ergo
