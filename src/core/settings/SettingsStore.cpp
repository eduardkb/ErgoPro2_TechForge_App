#include "core/settings/SettingsStore.h"

#include "core/LogCategories.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>

namespace ergo {

SettingsStore::SettingsStore(const SettingsSchema &schema, const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_schema(schema)
    , m_filePath(filePath)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(kDefaultSaveDelayMs);
    connect(&m_saveTimer, &QTimer::timeout, this, [this] { saveNow(); });
    applyDefaults();
}

SettingsStore::~SettingsStore()
{
    if (m_dirty)
        saveNow();
}

void SettingsStore::applyDefaults()
{
    m_values.clear();
    for (const auto &d : m_schema.definitions())
        m_values.insert(d.key, d.defaultValue);
}

SettingsLoadResult SettingsStore::load()
{
    SettingsLoadResult result;
    applyDefaults();
    m_rawDocument = QJsonObject();
    m_savingDisabled = false;

    QFile file(m_filePath);
    if (!file.exists()) {
        qCInfo(lcSettings) << "No settings file yet, creating defaults at" << m_filePath;
        result.status = SettingsLoadResult::Status::CreatedDefaults;
        m_dirty = true;
        saveNow();
        return result;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        result.status = SettingsLoadResult::Status::ReadError;
        result.message = tr("The settings file '%1' cannot be read (%2). Default settings are used and changes "
                            "will not be saved.")
                             .arg(m_filePath, file.errorString());
        qCCritical(lcSettings).noquote() << result.message;
        m_savingDisabled = true;
        return result;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString reason = parseError.error != QJsonParseError::NoError
                                   ? parseError.errorString()
                                   : QStringLiteral("root element is not a JSON object");
        result.status = SettingsLoadResult::Status::RecoveredCorrupt;
        if (backupCorruptFile(&result.backupPath)) {
            result.message = tr("The settings file was damaged (%1). It was saved as '%2' and default settings "
                                "are used.")
                                 .arg(reason, result.backupPath);
            qCCritical(lcSettings).noquote() << result.message;
            m_dirty = true;
            saveNow();
        } else {
            result.message = tr("The settings file '%1' is damaged (%2) and could not be backed up. Default "
                                "settings are used and changes will not be saved, so the file is not lost.")
                                 .arg(m_filePath, reason);
            qCCritical(lcSettings).noquote() << result.message;
            m_savingDisabled = true;
        }
        return result;
    }

    m_rawDocument = doc.object();
    const int fileSchema = m_rawDocument.value(QStringLiteral("schemaVersion")).toInt(-1);
    if (fileSchema != kSchemaVersion) {
        qCWarning(lcSettings) << "Settings file schemaVersion is" << fileSchema << "- expected" << kSchemaVersion
                              << "; known keys are read and the file is rewritten";
        m_dirty = true;
    }

    for (const auto &d : m_schema.definitions()) {
        const QJsonValue section = m_rawDocument.value(d.section());
        const QJsonValue raw = section.isObject() ? section.toObject().value(d.name()) : QJsonValue(QJsonValue::Undefined);
        if (raw.isUndefined()) {
            qCInfo(lcSettings) << "Setting" << d.key << "missing, using default" << d.defaultValue;
            m_dirty = true;
            continue;
        }
        if (const auto v = SettingsSchema::fromJson(d, raw)) {
            m_values.insert(d.key, *v);
        } else {
            ++result.invalidValues;
            m_dirty = true;
            qCWarning(lcSettings) << "Invalid value for" << d.key << ":" << raw << "- using default"
                                  << d.defaultValue;
        }
    }

    if (result.invalidValues > 0) {
        result.message = tr("%n setting value(s) were invalid and have been reset to their defaults.", nullptr,
                            result.invalidValues);
    }
    qCInfo(lcSettings) << "Settings loaded from" << m_filePath;
    if (m_dirty)
        saveNow();
    return result;
}

bool SettingsStore::backupCorruptFile(QString *backupPath)
{
    const QFileInfo info(m_filePath);
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString target = info.dir().filePath(QStringLiteral("settings.corrupt-%1.json").arg(stamp));
    if (!QFile::rename(m_filePath, target)) {
        qCCritical(lcSettings) << "Cannot back up corrupt settings file" << m_filePath << "to" << target;
        return false;
    }
    *backupPath = target;
    return true;
}

QVariant SettingsStore::value(const QString &key) const
{
    const auto it = m_values.constFind(key);
    if (it == m_values.constEnd()) {
        qCCritical(lcSettings) << "Requested unknown setting" << key;
        return {};
    }
    return *it;
}

int SettingsStore::intValue(const QString &key) const
{
    return value(key).toInt();
}

bool SettingsStore::boolValue(const QString &key) const
{
    return value(key).toBool();
}

QString SettingsStore::stringValue(const QString &key) const
{
    return value(key).toString();
}

bool SettingsStore::setValue(const QString &key, const QVariant &newValue)
{
    const SettingDefinition *d = m_schema.find(key);
    if (!d) {
        qCCritical(lcSettings) << "Attempt to set unknown setting" << key;
        return false;
    }
    const auto validated = SettingsSchema::validate(*d, newValue);
    if (!validated) {
        qCWarning(lcSettings) << "Rejected invalid value for" << key << ":" << newValue;
        return false;
    }
    if (m_values.value(key) == *validated)
        return true;

    m_values.insert(key, *validated);
    qCDebug(lcSettings) << "Setting changed:" << key << "=" << *validated;
    emit valueChanged(key, *validated);
    scheduleSave();
    return true;
}

void SettingsStore::resetToDefaults()
{
    qCInfo(lcSettings) << "Resetting all settings to defaults";
    for (const auto &d : m_schema.definitions()) {
        if (m_values.value(d.key) != d.defaultValue) {
            m_values.insert(d.key, d.defaultValue);
            emit valueChanged(d.key, d.defaultValue);
        }
    }
    m_dirty = true;
    saveNow();
}

void SettingsStore::scheduleSave()
{
    m_dirty = true;
    m_saveTimer.start(); // restarts the debounce interval
}

bool SettingsStore::saveNow()
{
    m_saveTimer.stop();
    if (!m_dirty)
        return true;
    if (m_savingDisabled) {
        qCWarning(lcSettings) << "Saving is disabled for this session (settings file could not be read safely)";
        return false;
    }

    QJsonObject root = m_rawDocument;
    root.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    for (const auto &d : m_schema.definitions()) {
        QJsonObject section = root.value(d.section()).toObject();
        section.insert(d.name(), SettingsSchema::toJson(d, m_values.value(d.key)));
        root.insert(d.section(), section);
    }

    const QFileInfo info(m_filePath);
    if (!QDir().mkpath(info.absolutePath())) {
        const QString message = tr("Cannot create the settings folder '%1'.").arg(info.absolutePath());
        qCCritical(lcSettings).noquote() << message;
        emit saveFailed(message);
        return false;
    }

    // QSaveFile writes to a temporary file and renames it on commit (atomic).
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        const QString message = tr("Cannot write the settings file '%1': %2").arg(m_filePath, file.errorString());
        qCCritical(lcSettings).noquote() << message;
        emit saveFailed(message);
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        const QString message = tr("Cannot save the settings file '%1': %2").arg(m_filePath, file.errorString());
        qCCritical(lcSettings).noquote() << message;
        emit saveFailed(message);
        return false;
    }

    m_rawDocument = root;
    m_dirty = false;
    qCDebug(lcSettings) << "Settings saved to" << m_filePath;
    return true;
}

} // namespace ergo
