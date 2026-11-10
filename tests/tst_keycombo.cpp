#include "core/input/KeyCombo.h"

#include <QTest>

#include <thread>

using namespace ergo;

namespace {

InputEvent key(InputEventType type, int k, Modifiers mods, quint32 native = 0, bool repeat = false)
{
    InputEvent e;
    e.type = type;
    e.key = k;
    e.modifiers = mods;
    e.nativeKeyCode = native;
    e.autoRepeat = repeat;
    return e;
}

} // namespace

class TestKeyCombo : public QObject {
    Q_OBJECT
private slots:
    void exactModifierMatch()
    {
        const KeyCombo ctrlF9{Qt::Key_F9, Modifier::Control, false};
        QVERIFY(ctrlF9.matches(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::Control)));
        QVERIFY(!ctrlF9.matches(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::None)));
        QVERIFY(!ctrlF9.matches(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::Control | Modifier::Shift)));
        QVERIFY(!ctrlF9.matches(key(InputEventType::KeyDown, Qt::Key_F8, Modifier::Control)));

        InputEvent mouse;
        mouse.type = InputEventType::MouseDown;
        QVERIFY(!ctrlF9.matches(mouse));
    }

    void keypadIsDistinct()
    {
        const KeyCombo num7{Qt::Key_7, Modifier::None, true};
        InputEvent e = key(InputEventType::KeyDown, Qt::Key_7, Modifier::None);
        QVERIFY(!num7.matches(e));
        e.keypad = true;
        QVERIFY(num7.matches(e));
    }

    void validityAndText()
    {
        QVERIFY((KeyCombo{Qt::Key_F9, Modifier::None, false}.isValid()));
        QVERIFY(!(KeyCombo{Qt::Key_Shift, Modifier::Control, false}.isValid())); // modifier alone
        QVERIFY(!KeyCombo{}.isValid());
        QCOMPARE((KeyCombo{Qt::Key_F9, Modifier::Control | Modifier::Shift, false}.toString()),
                 QStringLiteral("Ctrl+Shift+F9"));
    }

    void registryAddRemove()
    {
        KeySuppressionRegistry r;
        const KeyCombo f9{Qt::Key_F9, Modifier::None, false};
        QVERIFY(r.isEmpty());
        QVERIFY(r.add(f9));
        QVERIFY(!r.add(f9)); // duplicate
        QVERIFY(!r.add(KeyCombo{Qt::Key_Control, Modifier::None, false})); // invalid
        QVERIFY(r.contains(f9));
        QCOMPARE(r.combos().size(), 1);
        QVERIFY(r.remove(f9));
        QVERIFY(!r.remove(f9));
        QVERIFY(r.isEmpty());
    }

    void swallowDownRepeatAndUp()
    {
        KeySuppressionRegistry r;
        r.add(KeyCombo{Qt::Key_F9, Modifier::None, false});

        QVERIFY(r.shouldSwallow(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::None, 101)));
        QVERIFY(r.shouldSwallow(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::None, 101, true))); // repeat
        QVERIFY(r.shouldSwallow(key(InputEventType::KeyUp, Qt::Key_F9, Modifier::None, 101)));
        QCOMPARE(r.swallowedCount(), quint64(3));

        // Other keys and other modifier combinations pass.
        QVERIFY(!r.shouldSwallow(key(InputEventType::KeyDown, Qt::Key_F8, Modifier::None, 100)));
        QVERIFY(!r.shouldSwallow(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::Shift, 101)));
        QVERIFY(!r.shouldSwallow(key(InputEventType::KeyUp, Qt::Key_F9, Modifier::Shift, 101)));
    }

    void keyUpSwallowedEvenAfterUnregister()
    {
        // Apps must never see a key-up without its key-down.
        KeySuppressionRegistry r;
        const KeyCombo f9{Qt::Key_F9, Modifier::None, false};
        r.add(f9);
        QVERIFY(r.shouldSwallow(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::None, 101)));
        r.remove(f9);
        QVERIFY(r.shouldSwallow(key(InputEventType::KeyUp, Qt::Key_F9, Modifier::None, 101)));
        // Afterwards F9 passes again.
        QVERIFY(!r.shouldSwallow(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::None, 101)));
    }

    void keyUpOfUnswallowedDownPasses()
    {
        // Pressed before the combo was registered: the release must reach the app.
        KeySuppressionRegistry r;
        QVERIFY(!r.shouldSwallow(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::None, 101)));
        r.add(KeyCombo{Qt::Key_F9, Modifier::None, false});
        QVERIFY(!r.shouldSwallow(key(InputEventType::KeyUp, Qt::Key_F9, Modifier::None, 101)));
    }

    void syntheticKeysAreNeverSwallowed()
    {
        KeySuppressionRegistry r;
        r.add(KeyCombo{Qt::Key_F9, Modifier::None, false});
        InputEvent e = key(InputEventType::KeyDown, Qt::Key_F9, Modifier::None, 101);
        e.synthetic = true;
        QVERIFY(!r.shouldSwallow(e));
    }

    void concurrentUseIsSafe()
    {
        KeySuppressionRegistry r;
        std::atomic<bool> run{true};
        std::thread hook([&] {
            while (run.load()) {
                r.shouldSwallow(key(InputEventType::KeyDown, Qt::Key_F9, Modifier::None, 101));
                r.shouldSwallow(key(InputEventType::KeyUp, Qt::Key_F9, Modifier::None, 101));
            }
        });
        for (int i = 0; i < 2000; ++i) {
            r.add(KeyCombo{Qt::Key_F9, Modifier::None, false});
            r.remove(KeyCombo{Qt::Key_F9, Modifier::None, false});
        }
        run.store(false);
        hook.join();
        QVERIFY(r.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestKeyCombo)
#include "tst_keycombo.moc"
