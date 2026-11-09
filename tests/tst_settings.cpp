#include "core/settings/GeneralSettings.h"
#include "core/settings/SettingsRegistry.h"
#include "core/settings/SettingsSchema.h"
#include "core/settings/SettingsStore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace ergo;

namespace {

QJsonObject readJson(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

void writeRaw(const QString &path, const QByteArray &bytes)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(bytes);
}

} // namespace

class TestSettings : public QObject {
    Q_OBJECT
private:
    SettingsSchema m_schema;

private slots:
    void initTestCase() { registerAllSettings(m_schema); }

    void schemaHasStage0Keys()
    {
        QVERIFY(m_schema.find(GeneralSettings::LogLevel));
        QVERIFY(m_schema.find(GeneralSettings::LogRetentionDays));
        QVERIFY(m_schema.find(GeneralSettings::TrayHintShown));
    }

    void schemaRejectsDuplicatesAndBadDefaults()
    {
        SettingsSchema schema;
        QVERIFY(schema.add(SettingDefinition::integer(QStringLiteral("a.b"), 5, 1, 10)));
        QVERIFY(!schema.add(SettingDefinition::integer(QStringLiteral("a.b"), 5, 1, 10)));
        QVERIFY(!schema.add(SettingDefinition::integer(QStringLiteral("a.c"), 50, 1, 10)));
        QVERIFY(!schema.add(SettingDefinition::boolean(QStringLiteral("nosection"), true)));
    }

    void validation()
    {
        const SettingDefinition *days = m_schema.find(GeneralSettings::LogRetentionDays);
        QVERIFY(SettingsSchema::validate(*days, 1));
        QVERIFY(SettingsSchema::validate(*days, 180));
        QVERIFY(!SettingsSchema::validate(*days, 0));
        QVERIFY(!SettingsSchema::validate(*days, 181));
        QVERIFY(!SettingsSchema::validate(*days, QStringLiteral("30")));
        QVERIFY(!SettingsSchema::validate(*days, 2.5));
        QVERIFY(!SettingsSchema::validate(*days, true));

        const SettingDefinition *level = m_schema.find(GeneralSettings::LogLevel);
        QVERIFY(SettingsSchema::validate(*level, QStringLiteral("info")));
        QVERIFY(!SettingsSchema::validate(*level, QStringLiteral("verbose")));
        QVERIFY(!SettingsSchema::validate(*level, 3));

        const SettingDefinition *hint = m_schema.find(GeneralSettings::TrayHintShown);
        QVERIFY(SettingsSchema::validate(*hint, true));
        QVERIFY(!SettingsSchema::validate(*hint, 1));
    }

    void createsDefaultsWhenMissing()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("sub/settings.json"));
        SettingsStore store(m_schema, path);
        const SettingsLoadResult result = store.load();
        QCOMPARE(result.status, SettingsLoadResult::Status::CreatedDefaults);
        QVERIFY(QFile::exists(path));

        const QJsonObject json = readJson(path);
        QCOMPARE(json.value(QStringLiteral("schemaVersion")).toInt(), SettingsStore::kSchemaVersion);
        QCOMPARE(json.value(QStringLiteral("general")).toObject().value(QStringLiteral("logRetentionDays")).toInt(), 30);
        QCOMPARE(store.stringValue(GeneralSettings::LogLevel), QStringLiteral("info_warning_error_critical"));
    }

    void saveIsDebouncedAndReloads()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        {
            SettingsStore store(m_schema, path);
            store.load();
            store.setSaveDelayMs(50);
            QSignalSpy changed(&store, &SettingsStore::valueChanged);

            QVERIFY(store.setValue(GeneralSettings::LogRetentionDays, 12));
            QVERIFY(store.setValue(GeneralSettings::LogRetentionDays, 14));
            QCOMPARE(changed.count(), 2);
            QVERIFY(store.hasPendingSave());
            // Not written yet (debounce).
            QCOMPARE(readJson(path).value(QStringLiteral("general")).toObject()
                         .value(QStringLiteral("logRetentionDays")).toInt(), 30);
            QTRY_VERIFY_WITH_TIMEOUT(!store.hasPendingSave(), 2000);
            QCOMPARE(readJson(path).value(QStringLiteral("general")).toObject()
                         .value(QStringLiteral("logRetentionDays")).toInt(), 14);

            // Same value: no signal, no save.
            QVERIFY(store.setValue(GeneralSettings::LogRetentionDays, 14));
            QCOMPARE(changed.count(), 2);
            QVERIFY(!store.hasPendingSave());
        }
        SettingsStore reloaded(m_schema, path);
        QCOMPARE(reloaded.load().status, SettingsLoadResult::Status::Loaded);
        QCOMPARE(reloaded.intValue(GeneralSettings::LogRetentionDays), 14);
    }

    void pendingChangesAreSavedOnDestruction()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        {
            SettingsStore store(m_schema, path);
            store.load();
            store.setSaveDelayMs(60000);
            QVERIFY(store.setValue(GeneralSettings::TrayHintShown, true));
        }
        QCOMPARE(readJson(path).value(QStringLiteral("general")).toObject()
                     .value(QStringLiteral("trayHintShown")).toBool(), true);
    }

    void rejectsInvalidAndUnknown()
    {
        QTemporaryDir dir;
        SettingsStore store(m_schema, QDir(dir.path()).filePath(QStringLiteral("settings.json")));
        store.load();
        QVERIFY(!store.setValue(GeneralSettings::LogRetentionDays, 500));
        QVERIFY(!store.setValue(QStringLiteral("general.doesNotExist"), 1));
        QCOMPARE(store.intValue(GeneralSettings::LogRetentionDays), 30);
    }

    void invalidValuesFallBackToDefaults()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        writeRaw(path, R"({"schemaVersion":1,"general":{"logLevel":"bogus","logRetentionDays":999,
                         "trayHintShown":true},"future":{"keep":"me"}})");
        SettingsStore store(m_schema, path);
        const SettingsLoadResult result = store.load();
        QCOMPARE(result.status, SettingsLoadResult::Status::Loaded);
        QCOMPARE(result.invalidValues, 2);
        QVERIFY(!result.message.isEmpty());
        QCOMPARE(store.stringValue(GeneralSettings::LogLevel), QStringLiteral("info_warning_error_critical"));
        QCOMPARE(store.intValue(GeneralSettings::LogRetentionDays), 30);
        QCOMPARE(store.boolValue(GeneralSettings::TrayHintShown), true);

        // File rewritten with corrected values; unknown sections preserved.
        const QJsonObject json = readJson(path);
        QCOMPARE(json.value(QStringLiteral("general")).toObject().value(QStringLiteral("logRetentionDays")).toInt(), 30);
        QCOMPARE(json.value(QStringLiteral("future")).toObject().value(QStringLiteral("keep")).toString(),
                 QStringLiteral("me"));
    }

    void corruptFileIsBackedUp()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        writeRaw(path, "{ this is not json");
        SettingsStore store(m_schema, path);
        const SettingsLoadResult result = store.load();
        QCOMPARE(result.status, SettingsLoadResult::Status::RecoveredCorrupt);
        QVERIFY(!result.backupPath.isEmpty());
        QVERIFY(QFile::exists(result.backupPath));
        QVERIFY(QFileInfo(result.backupPath).fileName().startsWith(QStringLiteral("settings.corrupt-")));
        QVERIFY(!result.message.isEmpty());

        QFile backup(result.backupPath);
        QVERIFY(backup.open(QIODevice::ReadOnly));
        QCOMPARE(backup.readAll(), QByteArray("{ this is not json"));

        // A fresh default file was written.
        QCOMPARE(readJson(path).value(QStringLiteral("schemaVersion")).toInt(), SettingsStore::kSchemaVersion);
    }

    void nonObjectRootIsCorrupt()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        writeRaw(path, "[1,2,3]");
        SettingsStore store(m_schema, path);
        QCOMPARE(store.load().status, SettingsLoadResult::Status::RecoveredCorrupt);
    }

    void resetToDefaults()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        SettingsStore store(m_schema, path);
        store.load();
        store.setValue(GeneralSettings::LogRetentionDays, 99);
        QSignalSpy changed(&store, &SettingsStore::valueChanged);
        store.resetToDefaults();
        QCOMPARE(store.intValue(GeneralSettings::LogRetentionDays), 30);
        QCOMPARE(changed.count(), 1);
        QVERIFY(!store.hasPendingSave());
        QCOMPARE(readJson(path).value(QStringLiteral("general")).toObject()
                     .value(QStringLiteral("logRetentionDays")).toInt(), 30);
    }
};

QTEST_GUILESS_MAIN(TestSettings)
#include "tst_settings.moc"
