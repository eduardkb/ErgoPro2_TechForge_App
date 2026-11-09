#pragma once

#include "core/settings/SettingsSchema.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>

namespace ergo {

// Result of SettingsStore::load(), so the UI can inform the user.
struct SettingsLoadResult {
    enum class Status {
        Loaded,           // file read; invalid values (if any) replaced by defaults
        CreatedDefaults,  // no file yet; defaults written
        RecoveredCorrupt, // file could not be parsed; backed up, defaults used
        ReadError         // file exists but cannot be read; defaults used, saving disabled
    };
    Status status = Status::Loaded;
    int invalidValues = 0;  // number of values replaced by their default
    QString backupPath;     // RecoveredCorrupt only (empty if the backup failed)
    QString message;        // human-readable description for the user (empty when all is fine)
};

// Persistent JSON settings (implementation_stages.md B.2).
//
// - Every value is validated against its SettingDefinition.
// - setValue() applies immediately, emits valueChanged() and schedules an
//   atomic save (QSaveFile) debounced by saveDelayMs (default 300 ms).
// - Unknown keys found in the file are preserved when saving.
class SettingsStore : public QObject {
    Q_OBJECT
public:
    static constexpr int kSchemaVersion = 1;
    static constexpr int kDefaultSaveDelayMs = 300;

    SettingsStore(const SettingsSchema &schema, const QString &filePath, QObject *parent = nullptr);
    ~SettingsStore() override;

    SettingsLoadResult load();

    QVariant value(const QString &key) const;
    int intValue(const QString &key) const;
    bool boolValue(const QString &key) const;
    QString stringValue(const QString &key) const;

    // Returns false (and logs a warning) for unknown keys or invalid values.
    bool setValue(const QString &key, const QVariant &value);

    // Restores every known setting to its default and saves.
    void resetToDefaults();

    // Writes immediately if there are unsaved changes. Returns false on failure.
    bool saveNow();
    bool hasPendingSave() const { return m_dirty; }

    void setSaveDelayMs(int ms) { m_saveTimer.setInterval(ms); }
    QString filePath() const { return m_filePath; }
    const SettingsSchema &schema() const { return m_schema; }

signals:
    void valueChanged(const QString &key, const QVariant &value);
    void saveFailed(const QString &message);

private:
    void scheduleSave();
    void applyDefaults();
    bool backupCorruptFile(QString *backupPath);

    const SettingsSchema &m_schema;
    QString m_filePath;
    QHash<QString, QVariant> m_values;
    QJsonObject m_rawDocument; // last loaded document, keeps unknown keys
    QTimer m_saveTimer;
    bool m_dirty = false;
    bool m_savingDisabled = false;
};

} // namespace ergo
