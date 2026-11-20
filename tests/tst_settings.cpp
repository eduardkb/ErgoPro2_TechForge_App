#include "core/settings/AutoClickSettings.h"
#include "core/settings/BreakTimerSettings.h"
#include "core/settings/GeneralSettings.h"
#include "core/settings/KeyControlSettings.h"
#include "core/settings/SettingsRegistry.h"
#include "core/settings/SettingsSchema.h"
#include "core/settings/SettingsStore.h"
#include "core/settings/StatisticsSettings.h"

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

    void autoClickSettings()
    {
        const SettingDefinition *timer = m_schema.find(AutoClickSettings::TriggerTimerMs);
        const SettingDefinition *distance = m_schema.find(AutoClickSettings::TriggerDistancePx);
        const SettingDefinition *jitter = m_schema.find(AutoClickSettings::JitterTolerancePx);
        QVERIFY(timer && distance && jitter);
        QVERIFY(m_schema.find(AutoClickSettings::Enabled));
        QVERIFY(m_schema.find(AutoClickSettings::ShowVisualFeedback));
        QCOMPARE(timer->defaultValue.toInt(), 200);
        QCOMPARE(distance->defaultValue.toInt(), 15);
        QCOMPARE(jitter->defaultValue.toInt(), 3);
        QVERIFY(SettingsSchema::validate(*timer, 100));
        QVERIFY(SettingsSchema::validate(*timer, 3000));
        QVERIFY(!SettingsSchema::validate(*timer, 99));
        QVERIFY(!SettingsSchema::validate(*timer, 3001));
        QVERIFY(!SettingsSchema::validate(*distance, 4));
        QVERIFY(!SettingsSchema::validate(*distance, 201));
        QVERIFY(!SettingsSchema::validate(*jitter, 0));
        QVERIFY(!SettingsSchema::validate(*jitter, 11));
        QCOMPARE(m_schema.find(AutoClickSettings::Enabled)->defaultValue.toBool(), false);
    }

    // 1.8.0 (requirements trace G-a1): defaults and ranges of the Break Timer,
    // Drag Lock and Statistics keys (B.2; ranges changed in 1.7.6 / 1.7.7).
    void integerRanges_data()
    {
        QTest::addColumn<QString>("key");
        QTest::addColumn<int>("minimum");
        QTest::addColumn<int>("maximum");
        QTest::addColumn<int>("defaultValue");
        namespace BT = BreakTimerSettings;
        QTest::newRow("break after working") << BT::MaxWorkMin << 20 << 120 << 50;
        QTest::newRow("reset after a pause") << BT::IdleResetMin << 2 << 15 << 5;
        QTest::newRow("break length") << BT::BreakMin << 3 << 15 << 4;
        QTest::newRow("postpone") << BT::PostponeMin << 3 << 20 << 10;
        QTest::newRow("activity pause") << BT::ActivityPauseS << 5 << 60 << 20;
        QTest::newRow("drag lock timeout") << KeyControlSettings::DragLockTimeoutS << 1 << 60 << 10;
        QTest::newRow("statistics retention") << StatisticsSettings::RetentionDays << 1 << 360 << 90;
        // 2.2.0: log problems card (system-only values).
        QTest::newRow("log viewer days") << GeneralSettings::LogViewerDays << 3 << 30 << 7;
        QTest::newRow("log viewer refresh") << GeneralSettings::LogViewerRefreshMinutes << 3 << 120 << 5;
    }

    // 2.2.0 (owner decision): out-of-range values of the log viewer settings
    // become the nearest limit (all other settings fall back to the default).
    void logViewerValuesAreClampedToTheirLimits()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        writeRaw(path, R"({"schemaVersion":2,"general":{"logViewerDays":50,"logViewerRefreshMinutes":1,
                           "logRetentionDays":500}})");
        SettingsStore store(m_schema, path);
        const SettingsLoadResult result = store.load();
        QCOMPARE(store.intValue(GeneralSettings::LogViewerDays), 30);
        QCOMPARE(store.intValue(GeneralSettings::LogViewerRefreshMinutes), 3);
        QCOMPARE(store.intValue(GeneralSettings::LogRetentionDays), 30); // others: default as before
        QCOMPARE(result.invalidValues, 1);
        // The file now holds the clamped values.
        const QJsonObject general = readJson(path).value(QStringLiteral("general")).toObject();
        QCOMPARE(general.value(QStringLiteral("logViewerDays")).toInt(), 30);
        QCOMPARE(general.value(QStringLiteral("logViewerRefreshMinutes")).toInt(), 3);

        // Not a whole number: default.
        writeRaw(path, R"({"schemaVersion":2,"general":{"logViewerDays":"many","logViewerRefreshMinutes":7.5}})");
        SettingsStore again(m_schema, path);
        again.load();
        QCOMPARE(again.intValue(GeneralSettings::LogViewerDays), 7);
        QCOMPARE(again.intValue(GeneralSettings::LogViewerRefreshMinutes), 5);
        // Runtime values are clamped the same way.
        QVERIFY(again.setValue(GeneralSettings::LogViewerDays, 99));
        QCOMPARE(again.intValue(GeneralSettings::LogViewerDays), 30);
    }

    void integerRanges()
    {
        QFETCH(QString, key);
        QFETCH(int, minimum);
        QFETCH(int, maximum);
        QFETCH(int, defaultValue);
        const SettingDefinition *def = m_schema.find(key);
        QVERIFY2(def, qPrintable(key));
        QCOMPARE(def->defaultValue.toInt(), defaultValue);
        QVERIFY(SettingsSchema::validate(*def, minimum));
        QVERIFY(SettingsSchema::validate(*def, maximum));
        if (def->clampToRange) {
            // 2.2.0: these become the nearest limit (logViewerValuesAreClampedToTheirLimits).
            QCOMPARE(SettingsSchema::validate(*def, minimum - 1)->toInt(), minimum);
            QCOMPARE(SettingsSchema::validate(*def, maximum + 1)->toInt(), maximum);
            return;
        }
        QVERIFY(!SettingsSchema::validate(*def, minimum - 1));
        QVERIFY(!SettingsSchema::validate(*def, maximum + 1));

        // A stored value outside the range is replaced by the default at load.
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        const QStringList parts = key.split(QLatin1Char('.'));
        QJsonObject section;
        section.insert(parts.at(1), maximum + 1);
        QJsonObject root;
        root.insert(QStringLiteral("schemaVersion"), 1);
        root.insert(parts.at(0), section);
        writeRaw(path, QJsonDocument(root).toJson());
        SettingsStore store(m_schema, path);
        const SettingsLoadResult result = store.load();
        QCOMPARE(result.invalidValues, 1);
        QCOMPARE(store.intValue(key), defaultValue);
    }

    void booleanDefaults()
    {
        const SettingDefinition *enabled = m_schema.find(BreakTimerSettings::Enabled);
        const SettingDefinition *fullscreen = m_schema.find(BreakTimerSettings::SuppressWhenFullscreen);
        QVERIFY(enabled && fullscreen);
        QCOMPARE(enabled->defaultValue.toBool(), true);
        QCOMPARE(fullscreen->defaultValue.toBool(), false);
        // 2.0.0: the older ~/Applications copy is offered for removal until kept.
        const SettingDefinition *olderCopyKept = m_schema.find(GeneralSettings::OlderCopyKept);
        QVERIFY(olderCopyKept);
        QCOMPARE(olderCopyKept->defaultValue.toBool(), false);
    }

    // 1.8.2 (owner request): the upper-case keys were renamed; an old file is
    // converted at load (schemaVersion 1 -> 2) without losing a value.
    void oldUpperCaseKeysAreMigrated()
    {
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        writeRaw(path, R"({"schemaVersion":1,
            "autoclick":{"AUTOCLICK_TRIGGER_TIMER":450,"AUTOCLICK_TRIGGER_DISTANCE":30,"enabled":true},
            "draglock":{"DRAGLOCK_TIMEOUT":25},
            "breaktimer":{"BREAKTIMER_IDLE_RESET":7,"BREAKTIMER_MAX_WORK_TIME":35,"BREAKTIMER_BREAK_TIME":6,
                          "BREAKTIMER_POSTPONE_TIME":12,"BREAKTIMER_ACTIVITY_PAUSE":30,
                          "maxWorkMinutes":40}})");
        SettingsStore store(m_schema, path);
        const SettingsLoadResult result = store.load();
        QCOMPARE(result.status, SettingsLoadResult::Status::Loaded);
        QCOMPARE(result.migratedKeys, 8);
        QCOMPARE(result.invalidValues, 0);
        QCOMPARE(store.intValue(QStringLiteral("autoclick.triggerTimeMs")), 450);
        QCOMPARE(store.intValue(QStringLiteral("autoclick.triggerDistancePx")), 30);
        QCOMPARE(store.boolValue(QStringLiteral("autoclick.enabled")), true);
        QCOMPARE(store.intValue(QStringLiteral("draglock.timeoutSeconds")), 25);
        QCOMPARE(store.intValue(QStringLiteral("breaktimer.idleResetMinutes")), 7);
        QCOMPARE(store.intValue(QStringLiteral("breaktimer.maxWorkMinutes")), 40); // the new key wins
        QCOMPARE(store.intValue(QStringLiteral("breaktimer.breakMinutes")), 6);
        QCOMPARE(store.intValue(QStringLiteral("breaktimer.postponeMinutes")), 12);
        QCOMPARE(store.intValue(QStringLiteral("breaktimer.activityPauseSeconds")), 30);

        // Rewritten file: new schema, no upper-case key left.
        const QJsonObject json = readJson(path);
        QCOMPARE(json.value(QStringLiteral("schemaVersion")).toInt(), 2);
        const QByteArray text = QJsonDocument(json).toJson();
        QVERIFY2(!text.contains("AUTOCLICK_") && !text.contains("BREAKTIMER_") && !text.contains("DRAGLOCK_"),
                 text.constData());

        // Loading the converted file again changes nothing.
        SettingsStore again(m_schema, path);
        QCOMPARE(again.load().migratedKeys, 0);
        QCOMPARE(again.intValue(QStringLiteral("breaktimer.breakMinutes")), 6);
    }

    void noUpperCaseKeysInTheSchema()
    {
        for (const auto &d : m_schema.definitions())
            QVERIFY2(d.name() != d.name().toUpper() || d.name().size() < 2, qPrintable(d.key));
    }

    void startupModeSettings()
    {
        const SettingDefinition *autoMode = m_schema.find(GeneralSettings::StartupAutoMode);
        const SettingDefinition *manualMode = m_schema.find(GeneralSettings::StartupManualMode);
        QVERIFY(autoMode);
        QVERIFY(manualMode);
        QCOMPARE(autoMode->defaultValue.toString(), GeneralSettings::StartupTray);
        QCOMPARE(manualMode->defaultValue.toString(), GeneralSettings::StartupWindow);
        QVERIFY(SettingsSchema::validate(*autoMode, GeneralSettings::StartupWindow));
        QVERIFY(!SettingsSchema::validate(*autoMode, QStringLiteral("minimized")));

        // A stage 0 settings file (without the new keys) gets the defaults.
        QTemporaryDir dir;
        const QString path = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
        writeRaw(path, R"({"schemaVersion":1,"general":{"logLevel":"info","logRetentionDays":30,"trayHintShown":true}})");
        SettingsStore store(m_schema, path);
        const SettingsLoadResult result = store.load();
        QCOMPARE(result.invalidValues, 0);
        QCOMPARE(store.stringValue(GeneralSettings::StartupAutoMode), GeneralSettings::StartupTray);
        QCOMPARE(readJson(path).value(QStringLiteral("general")).toObject()
                     .value(QStringLiteral("startupManualMode")).toString(), GeneralSettings::StartupWindow);
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
