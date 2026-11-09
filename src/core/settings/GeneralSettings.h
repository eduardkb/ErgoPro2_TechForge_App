#pragma once

#include <QString>

namespace ergo {

class SettingsSchema;

// Keys of the "general" section (implementation_stages.md B.2, stage 0 keys).
namespace GeneralSettings {

inline const QString LogLevel = QStringLiteral("general.logLevel");
inline const QString LogRetentionDays = QStringLiteral("general.logRetentionDays");
inline const QString TrayHintShown = QStringLiteral("general.trayHintShown");

void registerSettings(SettingsSchema &schema);

} // namespace GeneralSettings
} // namespace ergo
