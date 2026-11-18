#include "core/settings/SettingsMigration.h"

#include "core/LogCategories.h"

namespace ergo {

namespace {

struct Rename {
    const char *section;
    const char *oldName;
    const char *newName;
};

constexpr Rename kRenames[] = {
    {"autoclick", "AUTOCLICK_TRIGGER_TIMER", "triggerTimeMs"},
    {"autoclick", "AUTOCLICK_TRIGGER_DISTANCE", "triggerDistancePx"},
    {"draglock", "DRAGLOCK_TIMEOUT", "timeoutSeconds"},
    {"breaktimer", "BREAKTIMER_IDLE_RESET", "idleResetMinutes"},
    {"breaktimer", "BREAKTIMER_MAX_WORK_TIME", "maxWorkMinutes"},
    {"breaktimer", "BREAKTIMER_BREAK_TIME", "breakMinutes"},
    {"breaktimer", "BREAKTIMER_POSTPONE_TIME", "postponeMinutes"},
    {"breaktimer", "BREAKTIMER_ACTIVITY_PAUSE", "activityPauseSeconds"},
};

} // namespace

int migrateRenamedSettingKeys(QJsonObject &root)
{
    int found = 0;
    for (const Rename &r : kRenames) {
        const QString sectionName = QString::fromLatin1(r.section);
        const QString oldName = QString::fromLatin1(r.oldName);
        const QString newName = QString::fromLatin1(r.newName);
        QJsonObject section = root.value(sectionName).toObject();
        if (!section.contains(oldName))
            continue;
        ++found;
        if (section.contains(newName)) {
            qCInfo(lcSettings).noquote() << QStringLiteral("Settings: %1.%2 removed (%1.%3 already set)")
                                                .arg(sectionName, oldName, newName);
        } else {
            section.insert(newName, section.value(oldName));
            qCInfo(lcSettings).noquote() << QStringLiteral("Settings: %1.%2 renamed to %1.%3")
                                                .arg(sectionName, oldName, newName);
        }
        section.remove(oldName);
        root.insert(sectionName, section);
    }
    return found;
}

} // namespace ergo
