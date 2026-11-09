#include "core/settings/SettingsSchema.h"

#include "core/LogCategories.h"

#include <cmath>

namespace ergo {

QString SettingDefinition::section() const
{
    return key.section(QLatin1Char('.'), 0, 0);
}

QString SettingDefinition::name() const
{
    return key.section(QLatin1Char('.'), 1);
}

SettingDefinition SettingDefinition::boolean(const QString &key, bool defaultValue)
{
    SettingDefinition d;
    d.key = key;
    d.type = SettingType::Bool;
    d.defaultValue = defaultValue;
    return d;
}

SettingDefinition SettingDefinition::integer(const QString &key, int defaultValue, int minimum, int maximum)
{
    SettingDefinition d;
    d.key = key;
    d.type = SettingType::Int;
    d.defaultValue = defaultValue;
    d.minimum = minimum;
    d.maximum = maximum;
    return d;
}

SettingDefinition SettingDefinition::enumeration(const QString &key, const QString &defaultValue,
                                                 const QStringList &allowed)
{
    SettingDefinition d;
    d.key = key;
    d.type = SettingType::Enum;
    d.defaultValue = defaultValue;
    d.allowedValues = allowed;
    return d;
}

bool SettingsSchema::add(const SettingDefinition &definition)
{
    if (definition.section().isEmpty() || definition.name().isEmpty()) {
        qCCritical(lcSettings) << "Invalid setting key (expected <section>.<name>):" << definition.key;
        return false;
    }
    if (find(definition.key)) {
        qCCritical(lcSettings) << "Duplicate setting definition:" << definition.key;
        return false;
    }
    if (!validate(definition, definition.defaultValue)) {
        qCCritical(lcSettings) << "Default value of" << definition.key << "is outside its own range";
        return false;
    }
    m_definitions.append(definition);
    return true;
}

const SettingDefinition *SettingsSchema::find(const QString &key) const
{
    for (const auto &d : m_definitions) {
        if (d.key == key)
            return &d;
    }
    return nullptr;
}

std::optional<QVariant> SettingsSchema::validate(const SettingDefinition &definition, const QVariant &value)
{
    switch (definition.type) {
    case SettingType::Bool:
        if (value.typeId() == QMetaType::Bool)
            return value;
        return std::nullopt;
    case SettingType::Int: {
        const int typeId = value.typeId();
        const bool integral = typeId == QMetaType::Int || typeId == QMetaType::UInt
                              || typeId == QMetaType::LongLong || typeId == QMetaType::ULongLong;
        const bool wholeDouble = typeId == QMetaType::Double && std::floor(value.toDouble()) == value.toDouble();
        if (!integral && !wholeDouble)
            return std::nullopt;
        const qlonglong v = wholeDouble ? static_cast<qlonglong>(value.toDouble()) : value.toLongLong();
        if (v < definition.minimum || v > definition.maximum)
            return std::nullopt;
        return QVariant(static_cast<int>(v));
    }
    case SettingType::Enum: {
        if (value.typeId() != QMetaType::QString)
            return std::nullopt;
        const QString s = value.toString();
        if (!definition.allowedValues.contains(s))
            return std::nullopt;
        return QVariant(s);
    }
    }
    return std::nullopt;
}

std::optional<QVariant> SettingsSchema::fromJson(const SettingDefinition &definition, const QJsonValue &value)
{
    switch (definition.type) {
    case SettingType::Bool:
        if (!value.isBool())
            return std::nullopt;
        return validate(definition, QVariant(value.toBool()));
    case SettingType::Int: {
        if (!value.isDouble())
            return std::nullopt;
        const double d = value.toDouble();
        if (std::floor(d) != d)
            return std::nullopt;
        if (d < definition.minimum || d > definition.maximum)
            return std::nullopt;
        return QVariant(static_cast<int>(d));
    }
    case SettingType::Enum:
        if (!value.isString())
            return std::nullopt;
        return validate(definition, QVariant(value.toString()));
    }
    return std::nullopt;
}

QJsonValue SettingsSchema::toJson(const SettingDefinition &definition, const QVariant &value)
{
    switch (definition.type) {
    case SettingType::Bool:
        return QJsonValue(value.toBool());
    case SettingType::Int:
        return QJsonValue(value.toInt());
    case SettingType::Enum:
        return QJsonValue(value.toString());
    }
    return QJsonValue();
}

} // namespace ergo
