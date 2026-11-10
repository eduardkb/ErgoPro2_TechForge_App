#include "core/input/InputEvent.h"

#include <QStringList>

namespace ergo {

bool isModifierKey(int key)
{
    return modifierForKey(key) != Modifier::None || key == Qt::Key_AltGr;
}

Modifiers modifierForKey(int key)
{
    switch (key) {
    case Qt::Key_Shift:
        return Modifier::Shift;
    case Qt::Key_Control:
        return Modifier::Control;
    case Qt::Key_Alt:
        return Modifier::Alt;
    case Qt::Key_Meta:
        return Modifier::Meta;
    default:
        return Modifier::None;
    }
}

QString modifiersToString(Modifiers modifiers)
{
    QStringList parts;
    if (modifiers & Modifier::Control)
        parts << QStringLiteral("Ctrl");
    if (modifiers & Modifier::Shift)
        parts << QStringLiteral("Shift");
    if (modifiers & Modifier::Alt)
        parts << QStringLiteral("Alt");
    if (modifiers & Modifier::Meta)
        parts << QStringLiteral("Meta");
    return parts.join(QLatin1Char('+'));
}

QString keyToString(int key, bool keypad)
{
    if (key == 0)
        return QStringLiteral("?");
    // QKeySequence lives in QtGui, which core must not use; a compact name
    // table covers the keys shown in diagnostics.
    QString name;
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
        name = QStringLiteral("F%1").arg(key - Qt::Key_F1 + 1);
    else if ((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9))
        name = QChar(key);
    else {
        switch (key) {
        case Qt::Key_Escape: name = QStringLiteral("Esc"); break;
        case Qt::Key_Tab: name = QStringLiteral("Tab"); break;
        case Qt::Key_Backspace: name = QStringLiteral("Backspace"); break;
        case Qt::Key_Return: name = QStringLiteral("Return"); break;
        case Qt::Key_Enter: name = QStringLiteral("Enter"); break;
        case Qt::Key_Insert: name = QStringLiteral("Insert"); break;
        case Qt::Key_Delete: name = QStringLiteral("Delete"); break;
        case Qt::Key_Pause: name = QStringLiteral("Pause"); break;
        case Qt::Key_Print: name = QStringLiteral("Print"); break;
        case Qt::Key_Home: name = QStringLiteral("Home"); break;
        case Qt::Key_End: name = QStringLiteral("End"); break;
        case Qt::Key_Left: name = QStringLiteral("Left"); break;
        case Qt::Key_Up: name = QStringLiteral("Up"); break;
        case Qt::Key_Right: name = QStringLiteral("Right"); break;
        case Qt::Key_Down: name = QStringLiteral("Down"); break;
        case Qt::Key_PageUp: name = QStringLiteral("PageUp"); break;
        case Qt::Key_PageDown: name = QStringLiteral("PageDown"); break;
        case Qt::Key_Shift: name = QStringLiteral("Shift"); break;
        case Qt::Key_Control: name = QStringLiteral("Ctrl"); break;
        case Qt::Key_Meta: name = QStringLiteral("Meta"); break;
        case Qt::Key_Alt: name = QStringLiteral("Alt"); break;
        case Qt::Key_AltGr: name = QStringLiteral("AltGr"); break;
        case Qt::Key_CapsLock: name = QStringLiteral("CapsLock"); break;
        case Qt::Key_NumLock: name = QStringLiteral("NumLock"); break;
        case Qt::Key_ScrollLock: name = QStringLiteral("ScrollLock"); break;
        case Qt::Key_Space: name = QStringLiteral("Space"); break;
        case Qt::Key_Menu: name = QStringLiteral("Menu"); break;
        case Qt::Key_Plus: name = QStringLiteral("+"); break;
        case Qt::Key_Minus: name = QStringLiteral("-"); break;
        case Qt::Key_Asterisk: name = QStringLiteral("*"); break;
        case Qt::Key_Slash: name = QStringLiteral("/"); break;
        case Qt::Key_Period: name = QStringLiteral("."); break;
        case Qt::Key_Comma: name = QStringLiteral(","); break;
        case Qt::Key_Equal: name = QStringLiteral("="); break;
        case Qt::Key_Semicolon: name = QStringLiteral(";"); break;
        case Qt::Key_Apostrophe: name = QStringLiteral("'"); break;
        case Qt::Key_BracketLeft: name = QStringLiteral("["); break;
        case Qt::Key_BracketRight: name = QStringLiteral("]"); break;
        case Qt::Key_Backslash: name = QStringLiteral("\\"); break;
        case Qt::Key_QuoteLeft: name = QStringLiteral("`"); break;
        default: name = QStringLiteral("Key 0x%1").arg(key, 0, 16); break;
        }
    }
    return keypad ? QStringLiteral("Num %1").arg(name) : name;
}

QString mouseButtonToString(MouseButtonId button)
{
    switch (button) {
    case MouseButtonId::None: return QStringLiteral("none");
    case MouseButtonId::Left: return QStringLiteral("Left");
    case MouseButtonId::Right: return QStringLiteral("Right");
    case MouseButtonId::Middle: return QStringLiteral("Middle");
    case MouseButtonId::Back: return QStringLiteral("Back");
    case MouseButtonId::Forward: return QStringLiteral("Forward");
    case MouseButtonId::Other: return QStringLiteral("Other");
    }
    return QStringLiteral("?");
}

QString eventTypeToString(InputEventType type)
{
    switch (type) {
    case InputEventType::MouseMove: return QStringLiteral("MouseMove");
    case InputEventType::MouseDown: return QStringLiteral("MouseDown");
    case InputEventType::MouseUp: return QStringLiteral("MouseUp");
    case InputEventType::Wheel: return QStringLiteral("Wheel");
    case InputEventType::KeyDown: return QStringLiteral("KeyDown");
    case InputEventType::KeyUp: return QStringLiteral("KeyUp");
    }
    return QStringLiteral("?");
}

QString describeEvent(const InputEvent &e)
{
    QString text = eventTypeToString(e.type);
    if (e.isKey()) {
        // Do not repeat a modifier that is the key itself ("Shift+Shift").
        const Modifiers mods = e.modifiers & ~modifierForKey(e.key);
        const QString m = modifiersToString(mods);
        text += QLatin1Char(' ') + (m.isEmpty() ? QString() : m + QLatin1Char('+')) + keyToString(e.key, e.keypad);
        text += QStringLiteral(" (native %1)").arg(e.nativeKeyCode);
        if (e.autoRepeat)
            text += QStringLiteral(" repeat");
    } else if (e.isMouseButton()) {
        text += QLatin1Char(' ') + mouseButtonToString(e.button);
        const QString m = modifiersToString(e.modifiers);
        if (!m.isEmpty())
            text += QStringLiteral(" with ") + m;
        text += QStringLiteral(" at %1,%2").arg(qRound(e.position.x())).arg(qRound(e.position.y()));
    } else if (e.type == InputEventType::Wheel) {
        text += QStringLiteral(" dx=%1 dy=%2").arg(e.wheelDeltaX).arg(e.wheelDeltaY);
    } else {
        text += QStringLiteral(" %1,%2").arg(qRound(e.position.x())).arg(qRound(e.position.y()));
    }
    if (e.synthetic)
        text += QStringLiteral(" [synthetic]");
    return text;
}

} // namespace ergo
