#pragma once

#include <QJsonValue>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <optional>

namespace ergo {

enum class SettingType { Bool, Int, Enum };

// Definition of one setting: key, type, default and allowed range.
// Keys are "<section>.<name>" (e.g. "general.logRetentionDays"); the section
// becomes a JSON object in settings.json.
struct SettingDefinition {
    QString key;
    SettingType type = SettingType::Int;
    QVariant defaultValue;
    int minimum = 0;               // Int only
    int maximum = 0;               // Int only
    QStringList allowedValues;     // Enum only

    QString section() const;
    QString name() const;

    static SettingDefinition boolean(const QString &key, bool defaultValue);
    static SettingDefinition integer(const QString &key, int defaultValue, int minimum, int maximum);
    static SettingDefinition enumeration(const QString &key, const QString &defaultValue, const QStringList &allowed);
};

// Registry of all known settings. Each stage registers its own keys in one
// place (see SettingsRegistry.cpp).
class SettingsSchema {
public:
    // Adds a definition. Returns false (and logs) for duplicates or invalid definitions.
    bool add(const SettingDefinition &definition);

    const SettingDefinition *find(const QString &key) const;
    const QList<SettingDefinition> &definitions() const { return m_definitions; }

    // Validates a runtime value. Returns the normalized value, or nullopt if invalid.
    static std::optional<QVariant> validate(const SettingDefinition &definition, const QVariant &value);

    // Validates a value read from JSON. Returns the normalized value, or nullopt if invalid.
    static std::optional<QVariant> fromJson(const SettingDefinition &definition, const QJsonValue &value);

    static QJsonValue toJson(const SettingDefinition &definition, const QVariant &value);

private:
    QList<SettingDefinition> m_definitions;
};

} // namespace ergo
