#include "core/input/InputEvent.h"

#include <QTest>

using namespace ergo;

class TestInputEvent : public QObject {
    Q_OBJECT
private slots:
    void modifierKeys()
    {
        QVERIFY(isModifierKey(Qt::Key_Shift));
        QVERIFY(isModifierKey(Qt::Key_Control));
        QVERIFY(isModifierKey(Qt::Key_Alt));
        QVERIFY(isModifierKey(Qt::Key_Meta));
        QVERIFY(isModifierKey(Qt::Key_AltGr));
        QVERIFY(!isModifierKey(Qt::Key_F9));
        QCOMPARE(modifierForKey(Qt::Key_Control), Modifiers(Modifier::Control));
        QCOMPARE(modifierForKey(Qt::Key_A), Modifiers(Modifier::None));
    }

    void modifierText()
    {
        QCOMPARE(modifiersToString(Modifier::None), QString());
        QCOMPARE(modifiersToString(Modifier::Shift | Modifier::Control), QStringLiteral("Ctrl+Shift"));
        QCOMPARE(modifiersToString(Modifier::Meta | Modifier::Alt | Modifier::Shift | Modifier::Control),
                 QStringLiteral("Ctrl+Shift+Alt+Meta"));
    }

    void keyNames()
    {
        QCOMPARE(keyToString(Qt::Key_F9, false), QStringLiteral("F9"));
        QCOMPARE(keyToString(Qt::Key_F24, false), QStringLiteral("F24"));
        QCOMPARE(keyToString(Qt::Key_A, false), QStringLiteral("A"));
        QCOMPARE(keyToString(Qt::Key_7, true), QStringLiteral("Num 7"));
        QCOMPARE(keyToString(0, false), QStringLiteral("?"));
    }

    void describeKey()
    {
        InputEvent e;
        e.type = InputEventType::KeyDown;
        e.key = Qt::Key_F9;
        e.modifiers = Modifier::Control | Modifier::Shift;
        e.nativeKeyCode = 101;
        QCOMPARE(describeEvent(e), QStringLiteral("KeyDown Ctrl+Shift+F9 (native 101)"));

        // A modifier key does not repeat itself ("Shift+Shift").
        e.key = Qt::Key_Shift;
        e.modifiers = Modifier::Shift;
        e.nativeKeyCode = 56;
        QCOMPARE(describeEvent(e), QStringLiteral("KeyDown Shift (native 56)"));
    }

    void describeMouse()
    {
        InputEvent e;
        e.type = InputEventType::MouseDown;
        e.button = MouseButtonId::Left;
        e.position = QPointF(10.4, 20.6);
        e.synthetic = true;
        QCOMPARE(describeEvent(e), QStringLiteral("MouseDown Left at 10,21 [synthetic]"));

        e.type = InputEventType::Wheel;
        e.wheelDeltaY = -120;
        e.synthetic = false;
        QCOMPARE(describeEvent(e), QStringLiteral("Wheel dx=0 dy=-120"));
    }
};

QTEST_GUILESS_MAIN(TestInputEvent)
#include "tst_inputevent.moc"
