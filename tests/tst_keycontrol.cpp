#include "core/keycontrol/KeyAction.h"
#include "core/keycontrol/KeyBinding.h"
#include "core/keycontrol/KeyComboText.h"
#include "core/keycontrol/KeyComboValidation.h"
#include "core/keycontrol/KeyControlEngine.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

using namespace ergo;

namespace {

KeyCombo combo(int key, Modifiers mods = Modifier::None, bool keypad = false)
{
    return KeyCombo{key, mods, keypad};
}

InputEvent keyEvent(InputEventType type, const KeyCombo &c, bool repeat = false)
{
    InputEvent e;
    e.type = type;
    e.key = c.key;
    e.modifiers = c.modifiers;
    e.keypad = c.keypad;
    e.autoRepeat = repeat;
    return e;
}

// Fake input layer: records registrations, can refuse combos.
struct FakeRegistrar : IKeyRegistrar {
    QSet<KeyCombo> registered;
    QSet<KeyCombo> refuse;
    int registerCalls = 0;
    OpResult registerKey(const KeyCombo &c) override
    {
        ++registerCalls;
        if (refuse.contains(c))
            return OpResult::failure(QStringLiteral("grabbed by another application"));
        registered.insert(c);
        return OpResult::success();
    }
    OpResult unregisterKey(const KeyCombo &c) override
    {
        registered.remove(c);
        return OpResult::success();
    }
    bool isRegistered(const KeyCombo &c) const override { return registered.contains(c); }
};

struct FakeContext : KeyActionContext {
    int left = 0, right = 0, doubles = 0, notified = 0, toggles = 0;
    OpResult leftClick() override { ++left; return {}; }
    OpResult rightClick() override { ++right; return {}; }
    OpResult doubleLeftClick() override { ++doubles; return {}; }
    void notifyKeyControlClick() override { ++notified; }
    OpResult toggleAutoClick() override { ++toggles; return {}; }
};

struct EngineFixture {
    FakeRegistrar registrar;
    KeyActionRegistry actions;
    FakeContext context;
    KeyControlEngine engine{registrar, actions, context, OsStyle::MacOS};
};

} // namespace

class TestKeyControl : public QObject {
    Q_OBJECT
private slots:
    // ---------------------------------------------------------------- text
    void portableTextRoundTrip_data()
    {
        QTest::addColumn<int>("key");
        QTest::addColumn<int>("mods");
        QTest::addColumn<bool>("keypad");
        QTest::addColumn<QString>("text");
        QTest::newRow("F9") << int(Qt::Key_F9) << 0 << false << "F9";
        QTest::newRow("ctrl shift F9") << int(Qt::Key_F9) << int(Modifier::Control | Modifier::Shift) << false
                                       << "Ctrl+Shift+F9";
        QTest::newRow("all mods") << int(Qt::Key_A)
                                  << int(Modifier::Meta | Modifier::Alt | Modifier::Shift | Modifier::Control)
                                  << false << "Ctrl+Alt+Shift+Meta+A";
        QTest::newRow("keypad 7") << int(Qt::Key_7) << 0 << true << "Num+7";
        QTest::newRow("slash") << int(Qt::Key_Slash) << int(Modifier::Alt) << false << "Alt+Slash";
        QTest::newRow("plus") << int(Qt::Key_Plus) << 0 << true << "Num+Plus";
        QTest::newRow("F24") << int(Qt::Key_F24) << 0 << false << "F24";
    }

    void portableTextRoundTrip()
    {
        QFETCH(int, key);
        QFETCH(int, mods);
        QFETCH(bool, keypad);
        QFETCH(QString, text);
        const KeyCombo c = combo(key, Modifiers(mods), keypad);
        QCOMPARE(comboToPortableText(c), text);
        const auto parsed = comboFromPortableText(text);
        QVERIFY(parsed.has_value());
        QVERIFY(*parsed == c);
    }

    void parsingIsLenient()
    {
        QVERIFY(*comboFromPortableText(QStringLiteral("shift+ctrl+f9")) == combo(Qt::Key_F9, Modifier::Control | Modifier::Shift));
        QVERIFY(*comboFromPortableText(QStringLiteral("Cmd+B")) == combo(Qt::Key_B, Modifier::Meta));
        QVERIFY(!comboFromPortableText(QStringLiteral("Hyper+F9")).has_value());
        QVERIFY(!comboFromPortableText(QStringLiteral("Ctrl+NoSuchKey")).has_value());
        QVERIFY(!comboFromPortableText(QString()).has_value());
    }

    void displayTextPerOs()
    {
        const KeyCombo c = combo(Qt::Key_F9, Modifier::Meta | Modifier::Shift);
        QCOMPARE(comboToDisplayText(c, OsStyle::MacOS), QStringLiteral("⇧⌘F9"));
        QCOMPARE(comboToDisplayText(c, OsStyle::Windows), QStringLiteral("Shift+Win+F9"));
        QCOMPARE(comboToDisplayText(c, OsStyle::Linux), QStringLiteral("Shift+Super+F9"));
        const KeyCombo all = combo(Qt::Key_K, Modifier::Control | Modifier::Alt | Modifier::Shift | Modifier::Meta);
        QCOMPARE(comboToDisplayText(all, OsStyle::MacOS), QStringLiteral("⌃⌥⇧⌘K"));
        QCOMPARE(comboToDisplayText(combo(Qt::Key_7, Modifier::None, true), OsStyle::Windows), QStringLiteral("Num 7"));
        QCOMPARE(comboToDisplayText(combo(Qt::Key_Return), OsStyle::MacOS), QStringLiteral("↩"));
    }

    // ---------------------------------------------------------------- validation
    void allowedCombos()
    {
        for (OsStyle os : {OsStyle::MacOS, OsStyle::Windows, OsStyle::Linux}) {
            QCOMPARE(validateCombo(combo(Qt::Key_F9), os).result, ComboCheck::Result::Ok);
            QCOMPARE(validateCombo(combo(Qt::Key_F24), os).result, ComboCheck::Result::Ok);
            QCOMPARE(validateCombo(combo(Qt::Key_5, Modifier::None, true), os).result, ComboCheck::Result::Ok);
            QCOMPARE(validateCombo(combo(Qt::Key_K, Modifier::Control | Modifier::Alt), os).result,
                     ComboCheck::Result::Ok);
        }
    }

    void plainTypingKeysNeedConfirmation()
    {
        for (int key : {int(Qt::Key_A), int(Qt::Key_7), int(Qt::Key_Space), int(Qt::Key_Return), int(Qt::Key_Slash)}) {
            const ComboCheck c = validateCombo(combo(key), OsStyle::Linux);
            QCOMPARE(c.result, ComboCheck::Result::Warning);
            QVERIFY(c.accepted());
            QVERIFY(c.message.contains(QStringLiteral("no longer type")));
        }
        // With a modifier the same key is fine.
        QCOMPARE(validateCombo(combo(Qt::Key_A, Modifier::Control), OsStyle::Linux).result, ComboCheck::Result::Ok);
    }

    void modifierOnlyAndUnknownAreRejected()
    {
        QCOMPARE(validateCombo(combo(Qt::Key_Shift), OsStyle::MacOS).result, ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(Qt::Key_Control, Modifier::Shift), OsStyle::Windows).result,
                 ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(0), OsStyle::MacOS).result, ComboCheck::Result::Rejected);
    }

    void reservedCombosPerOs()
    {
        // macOS
        QCOMPARE(validateCombo(combo(Qt::Key_Tab, Modifier::Meta), OsStyle::MacOS).result, ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(Qt::Key_Space, Modifier::Meta), OsStyle::MacOS).result, ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(Qt::Key_Escape, Modifier::Meta | Modifier::Alt), OsStyle::MacOS).result,
                 ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(Qt::Key_Q, Modifier::Control | Modifier::Meta), OsStyle::MacOS).result,
                 ComboCheck::Result::Rejected);
        QVERIFY(validateCombo(combo(Qt::Key_Tab, Modifier::Meta), OsStyle::MacOS).message.contains(QStringLiteral("reserved")));
        // Windows
        QCOMPARE(validateCombo(combo(Qt::Key_Delete, Modifier::Control | Modifier::Alt), OsStyle::Windows).result,
                 ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(Qt::Key_Escape, Modifier::Control | Modifier::Shift), OsStyle::Windows).result,
                 ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(Qt::Key_Tab, Modifier::Alt), OsStyle::Windows).result, ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(Qt::Key_L, Modifier::Meta), OsStyle::Windows).result, ComboCheck::Result::Rejected);
        // Linux
        QCOMPARE(validateCombo(combo(Qt::Key_F2, Modifier::Control | Modifier::Alt), OsStyle::Linux).result,
                 ComboCheck::Result::Rejected);
        QCOMPARE(validateCombo(combo(Qt::Key_Backspace, Modifier::Control | Modifier::Alt), OsStyle::Linux).result,
                 ComboCheck::Result::Rejected);
        // Reserved on one OS only.
        QCOMPARE(validateCombo(combo(Qt::Key_L, Modifier::Meta), OsStyle::MacOS).result, ComboCheck::Result::Ok);
        QCOMPARE(validateCombo(combo(Qt::Key_Tab, Modifier::Meta), OsStyle::Linux).result, ComboCheck::Result::Ok);
    }

    // ---------------------------------------------------------------- actions
    void actionRegistry()
    {
        KeyActionRegistry r;
        QVERIFY(r.find(KeyActionRegistry::LeftClick));
        QVERIFY(r.find(KeyActionRegistry::RightClick));
        QVERIFY(r.find(KeyActionRegistry::DoubleLeftClick));
        QVERIFY(r.find(KeyActionRegistry::ToggleAutoClick));
        QVERIFY(r.find(KeyActionRegistry::DragLock));        // registered (stage 6) ...
        QCOMPARE(r.visibleActions().size(), 4);              // ... but not offered yet
        QVERIFY(!r.find(QStringLiteral("nope")));
        FakeContext c;
        QVERIFY(!r.find(KeyActionRegistry::DragLock)->execute(c).ok);
    }

    // ---------------------------------------------------------------- store
    void storeRoundTripAndInvalidEntries()
    {
        KeyActionRegistry actions;
        const QJsonArray stored{
            QJsonObject{{"id", "a"}, {"keyCombo", "Ctrl+Shift+F9"}, {"action", "left-click"}, {"enabled", true}},
            QJsonObject{{"id", "b"}, {"keyCombo", "Hyper+X"}, {"action", "left-click"}, {"enabled", true}},
            QJsonObject{{"id", "c"}, {"keyCombo", "F10"}, {"action", "teleport"}, {"enabled", false}},
            QJsonObject{{"id", "d"}, {"keyCombo", "Ctrl+Shift+F9"}, {"action", "right-click"}},
            QJsonObject{{"id", "e"}, {"keyCombo", "Meta+Tab"}, {"action", "right-click"}},
            QJsonValue(42),
        };
        QStringList problems;
        const QList<KeyBinding> list = KeyBindingStore::fromJson(stored, actions, OsStyle::MacOS, &problems);
        QCOMPARE(list.size(), 6); // nothing is dropped
        QVERIFY(list[0].isValid());
        QVERIFY(list[1].loadError.contains("Unknown key combination"));
        QVERIFY(list[2].loadError.contains("Unknown action"));
        QVERIFY(list[3].loadError.contains("twice"));
        QVERIFY(list[4].loadError.contains("reserved"));
        QVERIFY(!list[5].isValid());
        QCOMPARE(problems.size(), 5);

        // Written back: valid entries normalized, invalid ones unchanged.
        const QJsonArray written = KeyBindingStore::toJson(list);
        QCOMPARE(written.size(), 6);
        QCOMPARE(written[0].toObject().value("keyCombo").toString(), QStringLiteral("Ctrl+Shift+F9"));
        QCOMPARE(written[1].toObject(), stored[1].toObject());
        QCOMPARE(written[5], QJsonValue(42));
        QCOMPARE(list[3].enabled, true); // missing "enabled" defaults to true
    }

    // ---------------------------------------------------------------- engine
    void addRegistersAndFiresOncePerPress()
    {
        EngineFixture f;
        const KeyCombo f9 = combo(Qt::Key_F9);
        QString id;
        QVERIFY(f.engine.add(f9, KeyActionRegistry::LeftClick, &id).ok);
        QVERIFY(f.registrar.registered.contains(f9));
        QCOMPARE(f.engine.state(id).status, BindingStatus::Active);

        QCOMPARE(f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, f9)), std::optional<QString>(id));
        QVERIFY(!f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, f9, true)).has_value()); // repeat
        QVERIFY(!f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, f9, true)).has_value());
        QVERIFY(!f.engine.onKeyEvent(keyEvent(InputEventType::KeyUp, f9)).has_value());
        QCOMPARE(f.context.left, 1);
        QCOMPARE(f.context.notified, 1); // AutoClick is told about the click
        f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, f9));
        QCOMPARE(f.context.left, 2); // next press fires again
    }

    void modifiersMustMatchExactly()
    {
        EngineFixture f;
        f.engine.add(combo(Qt::Key_F9, Modifier::Control), KeyActionRegistry::RightClick);
        f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, combo(Qt::Key_F9)));
        f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, combo(Qt::Key_F9, Modifier::Control | Modifier::Shift)));
        QCOMPARE(f.context.right, 0);
        f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, combo(Qt::Key_F9, Modifier::Control)));
        QCOMPARE(f.context.right, 1);
    }

    void syntheticKeysNeverFire()
    {
        EngineFixture f;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick);
        InputEvent e = keyEvent(InputEventType::KeyDown, combo(Qt::Key_F9));
        e.synthetic = true;
        QVERIFY(!f.engine.onKeyEvent(e).has_value());
    }

    void allActionsDispatch()
    {
        EngineFixture f;
        f.engine.add(combo(Qt::Key_F1), KeyActionRegistry::LeftClick);
        f.engine.add(combo(Qt::Key_F2), KeyActionRegistry::RightClick);
        f.engine.add(combo(Qt::Key_F3), KeyActionRegistry::DoubleLeftClick);
        f.engine.add(combo(Qt::Key_F4), KeyActionRegistry::ToggleAutoClick);
        for (int k : {int(Qt::Key_F1), int(Qt::Key_F2), int(Qt::Key_F3), int(Qt::Key_F4)})
            f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, combo(k)));
        QCOMPARE(f.context.left, 1);
        QCOMPARE(f.context.right, 1);
        QCOMPARE(f.context.doubles, 1);
        QCOMPARE(f.context.toggles, 1);
        QCOMPARE(f.context.notified, 3); // toggling Auto Click is not a click
    }

    void duplicatesAndReservedAreRejected()
    {
        EngineFixture f;
        QVERIFY(f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick).ok);
        const OpResult dup = f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::RightClick);
        QVERIFY(!dup.ok);
        QVERIFY(dup.message.contains(QStringLiteral("already assigned")));
        const OpResult reserved = f.engine.add(combo(Qt::Key_Tab, Modifier::Meta), KeyActionRegistry::LeftClick);
        QVERIFY(!reserved.ok);
        QCOMPARE(f.engine.bindings().size(), 1);
    }

    // ---------------------------------------------------------------- one hotkey per action (1.4.1)
    void sameActionCannotBeAssignedTwice()
    {
        EngineFixture f;
        QVERIFY(f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick).ok);
        const OpResult r = f.engine.add(combo(Qt::Key_F10), KeyActionRegistry::LeftClick);
        QVERIFY(!r.ok);
        QVERIFY(r.message.contains(QStringLiteral("already assigned")));
        QCOMPARE(f.engine.bindings().size(), 1);
        QVERIFY(!f.registrar.registered.contains(combo(Qt::Key_F10)));
    }

    void availableActionsExcludeAssignedOnes()
    {
        EngineFixture f;
        QCOMPARE(f.engine.availableActionIds().size(), 4); // all visible actions
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        QStringList free = f.engine.availableActionIds();
        QCOMPARE(free.size(), 3);
        QVERIFY(!free.contains(KeyActionRegistry::LeftClick));
        // When modifying that hotkey, its own action stays selectable.
        QVERIFY(f.engine.availableActionIds(id).contains(KeyActionRegistry::LeftClick));
        QVERIFY(!f.engine.availableActionIds().contains(KeyActionRegistry::DragLock)); // hidden
    }

    void disabledHotkeyStillOccupiesItsAction()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::RightClick, &id);
        f.engine.setEnabled(id, false);
        QVERIFY(!f.engine.availableActionIds().contains(KeyActionRegistry::RightClick));
        QVERIFY(!f.engine.add(combo(Qt::Key_F10), KeyActionRegistry::RightClick).ok);
    }

    void deletingAHotkeyFreesItsAction()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        f.engine.remove(id);
        QVERIFY(f.engine.availableActionIds().contains(KeyActionRegistry::LeftClick));
        QVERIFY(f.engine.add(combo(Qt::Key_F10), KeyActionRegistry::LeftClick).ok);
    }

    void modifyCannotTakeAnotherHotkeysAction()
    {
        EngineFixture f;
        QString a, b;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &a);
        f.engine.add(combo(Qt::Key_F10), KeyActionRegistry::RightClick, &b);
        QVERIFY(!f.engine.modify(b, combo(Qt::Key_F10), KeyActionRegistry::LeftClick).ok);
        QCOMPARE(f.engine.binding(b)->actionId, KeyActionRegistry::RightClick);
        // Changing only the key while keeping the own action is fine.
        QVERIFY(f.engine.modify(a, combo(Qt::Key_F11), KeyActionRegistry::LeftClick).ok);
    }

    void storedDuplicateActionsAreErrors()
    {
        KeyActionRegistry actions;
        const QJsonArray stored{
            QJsonObject{{"id", "a"}, {"keyCombo", "F9"}, {"action", "left-click"}},
            QJsonObject{{"id", "b"}, {"keyCombo", "F10"}, {"action", "left-click"}},
        };
        const QList<KeyBinding> list = KeyBindingStore::fromJson(stored, actions, OsStyle::MacOS);
        QVERIFY(list[0].isValid());
        QVERIFY(!list[1].isValid());
        QVERIFY(list[1].loadError.contains(QStringLiteral("already assigned")));
    }

    void registrationFailureIsReportedAndNotKept()
    {
        EngineFixture f;
        f.registrar.refuse.insert(combo(Qt::Key_F8));
        const OpResult r = f.engine.add(combo(Qt::Key_F8), KeyActionRegistry::LeftClick);
        QVERIFY(!r.ok);
        QVERIFY(r.message.contains(QStringLiteral("another application")));
        QVERIFY(f.engine.bindings().isEmpty());
    }

    void disableReleasesAndEnableRegistersAgain()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        QVERIFY(f.engine.setEnabled(id, false).ok);
        QVERIFY(!f.registrar.registered.contains(combo(Qt::Key_F9))); // released to the OS
        QCOMPARE(f.engine.state(id).status, BindingStatus::Disabled);
        QVERIFY(!f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, combo(Qt::Key_F9))).has_value());
        QVERIFY(f.engine.setEnabled(id, true).ok);
        QVERIFY(f.registrar.registered.contains(combo(Qt::Key_F9)));
        QCOMPARE(f.engine.state(id).status, BindingStatus::Active);
    }

    void resetUnregistersAndRegistersAgain()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        const int before = f.registrar.registerCalls;
        QVERIFY(f.engine.resetBinding(id).ok);
        QCOMPARE(f.registrar.registerCalls, before + 1);
        QVERIFY(f.registrar.registered.contains(combo(Qt::Key_F9)));
    }

    void modifyChangesKeyAndAction()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        QVERIFY(f.engine.modify(id, combo(Qt::Key_F10, Modifier::Shift), KeyActionRegistry::RightClick).ok);
        QVERIFY(!f.registrar.registered.contains(combo(Qt::Key_F9)));
        QVERIFY(f.registrar.registered.contains(combo(Qt::Key_F10, Modifier::Shift)));
        f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, combo(Qt::Key_F10, Modifier::Shift)));
        QCOMPARE(f.context.right, 1);
        // Keeping the same key while changing only the action is allowed (no self-duplicate).
        QVERIFY(f.engine.modify(id, combo(Qt::Key_F10, Modifier::Shift), KeyActionRegistry::LeftClick).ok);
    }

    void failedModifyKeepsTheWorkingBinding()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        f.registrar.refuse.insert(combo(Qt::Key_F8));
        QVERIFY(!f.engine.modify(id, combo(Qt::Key_F8), KeyActionRegistry::LeftClick).ok);
        QVERIFY(f.engine.binding(id)->combo == combo(Qt::Key_F9));
        QCOMPARE(f.engine.state(id).status, BindingStatus::Active);
        QVERIFY(f.registrar.registered.contains(combo(Qt::Key_F9)));
    }

    void removeReleases()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        QVERIFY(f.engine.remove(id).ok);
        QVERIFY(f.registrar.registered.isEmpty());
        QVERIFY(f.engine.bindings().isEmpty());
    }

    void pauseReleasesEverythingAndResumeRestores()
    {
        EngineFixture f;
        QString a, b;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &a);
        f.engine.add(combo(Qt::Key_F10), KeyActionRegistry::RightClick, &b);
        f.engine.setEnabled(b, false);
        f.engine.setPaused(true); // Add/Modify dialog open (K-14)
        QVERIFY(f.registrar.registered.isEmpty());
        QCOMPARE(f.engine.state(a).status, BindingStatus::Paused);
        QVERIFY(!f.engine.onKeyEvent(keyEvent(InputEventType::KeyDown, combo(Qt::Key_F9))).has_value());
        f.engine.setPaused(false);
        QVERIFY(f.registrar.registered.contains(combo(Qt::Key_F9)));
        QVERIFY(!f.registrar.registered.contains(combo(Qt::Key_F10))); // stays disabled
        QCOMPARE(f.engine.state(b).status, BindingStatus::Disabled);
    }

    void unavailableInputReleasesAndRestores()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        f.engine.setInputAvailable(false);
        QCOMPARE(f.engine.state(id).status, BindingStatus::Unavailable);
        QVERIFY(f.registrar.registered.isEmpty());
        f.engine.setInputAvailable(true);
        QCOMPARE(f.engine.state(id).status, BindingStatus::Active);
    }

    void reapplyAfterHookReinstall()
    {
        EngineFixture f;
        QString id;
        f.engine.add(combo(Qt::Key_F9), KeyActionRegistry::LeftClick, &id);
        f.registrar.registered.clear(); // the input layer lost the registration
        QCOMPARE(f.engine.reapply(), 1);
        QVERIFY(f.registrar.registered.contains(combo(Qt::Key_F9)));
        QCOMPARE(f.engine.state(id).status, BindingStatus::Active);
        QCOMPARE(f.engine.reapply(), 0); // nothing lost: nothing to do
    }

    void loadShowsInvalidEntriesAsErrors()
    {
        EngineFixture f;
        const QJsonArray stored{
            QJsonObject{{"id", "ok"}, {"keyCombo", "F9"}, {"action", "left-click"}, {"enabled", true}},
            QJsonObject{{"id", "bad"}, {"keyCombo", "Hyper+X"}, {"action", "left-click"}},
            QJsonObject{{"id", "off"}, {"keyCombo", "F10"}, {"action", "right-click"}, {"enabled", false}},
        };
        f.engine.load(KeyBindingStore::fromJson(stored, f.actions, OsStyle::MacOS));
        QCOMPARE(f.engine.state("ok").status, BindingStatus::Active);
        QCOMPARE(f.engine.state("bad").status, BindingStatus::Error);
        QVERIFY(!f.engine.state("bad").reason.isEmpty());
        QCOMPARE(f.engine.state("off").status, BindingStatus::Disabled);
        QCOMPARE(f.registrar.registered.size(), 1);
        // Loading again (e.g. "Reset all settings") releases the old registrations.
        f.engine.load({});
        QVERIFY(f.registrar.registered.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestKeyControl)
#include "tst_keycontrol.moc"
