#include "core/keycontrol/KeyComboText.h"

#include <QStringList>

namespace ergo {

namespace {

struct NamedKey {
    int key;
    const char *portable; // name in settings.json
    const char *display;  // name shown on Windows/Linux
    const char *mac;      // name shown on macOS (nullptr = display)
};

const NamedKey kNamedKeys[] = {
    {Qt::Key_Escape, "Esc", "Esc", "⎋"},
    {Qt::Key_Tab, "Tab", "Tab", "⇥"},
    {Qt::Key_Backspace, "Backspace", "Backspace", "⌫"},
    {Qt::Key_Return, "Return", "Enter", "↩"},
    {Qt::Key_Enter, "Enter", "Enter", "⌤"},
    {Qt::Key_Insert, "Insert", "Insert", "Help"},
    {Qt::Key_Delete, "Delete", "Delete", "⌦"},
    {Qt::Key_Pause, "Pause", "Pause", nullptr},
    {Qt::Key_Print, "Print", "Print Screen", nullptr},
    {Qt::Key_Home, "Home", "Home", "↖"},
    {Qt::Key_End, "End", "End", "↘"},
    {Qt::Key_Left, "Left", "Left", "←"},
    {Qt::Key_Up, "Up", "Up", "↑"},
    {Qt::Key_Right, "Right", "Right", "→"},
    {Qt::Key_Down, "Down", "Down", "↓"},
    {Qt::Key_PageUp, "PageUp", "Page Up", "⇞"},
    {Qt::Key_PageDown, "PageDown", "Page Down", "⇟"},
    {Qt::Key_CapsLock, "CapsLock", "Caps Lock", "⇪"},
    {Qt::Key_NumLock, "NumLock", "Num Lock", nullptr},
    {Qt::Key_ScrollLock, "ScrollLock", "Scroll Lock", nullptr},
    {Qt::Key_Space, "Space", "Space", nullptr},
    {Qt::Key_Menu, "Menu", "Menu", nullptr},
    {Qt::Key_Clear, "Clear", "Clear", "⌧"},
    {Qt::Key_Minus, "Minus", "-", nullptr},
    {Qt::Key_Equal, "Equal", "=", nullptr},
    {Qt::Key_BracketLeft, "BracketLeft", "[", nullptr},
    {Qt::Key_BracketRight, "BracketRight", "]", nullptr},
    {Qt::Key_Backslash, "Backslash", "\\", nullptr},
    {Qt::Key_Semicolon, "Semicolon", ";", nullptr},
    {Qt::Key_Apostrophe, "Apostrophe", "'", nullptr},
    {Qt::Key_Comma, "Comma", ",", nullptr},
    {Qt::Key_Period, "Period", ".", nullptr},
    {Qt::Key_Slash, "Slash", "/", nullptr},
    {Qt::Key_QuoteLeft, "Grave", "`", nullptr},
    {Qt::Key_Plus, "Plus", "+", nullptr},
    {Qt::Key_Asterisk, "Asterisk", "*", nullptr},
};

const NamedKey *findNamed(int key)
{
    for (const NamedKey &n : kNamedKeys) {
        if (n.key == key)
            return &n;
    }
    return nullptr;
}

QString baseKeyName(int key, bool portable, OsStyle style)
{
    if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
        return QStringLiteral("F%1").arg(key - Qt::Key_F1 + 1);
    if ((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9))
        return QString(QChar(key));
    if (const NamedKey *n = findNamed(key)) {
        if (portable)
            return QString::fromUtf8(n->portable);
        if (style == OsStyle::MacOS && n->mac)
            return QString::fromUtf8(n->mac);
        return QString::fromUtf8(n->display);
    }
    return QString();
}

int keyFromPortableName(const QString &name)
{
    if (name.size() == 1) {
        const QChar c = name.at(0).toUpper();
        if ((c >= QLatin1Char('A') && c <= QLatin1Char('Z')) || (c >= QLatin1Char('0') && c <= QLatin1Char('9')))
            return c.unicode();
    }
    if (name.size() >= 2 && (name.at(0) == QLatin1Char('F') || name.at(0) == QLatin1Char('f'))) {
        bool ok = false;
        const int n = name.mid(1).toInt(&ok);
        if (ok && n >= 1 && n <= 35)
            return Qt::Key_F1 + n - 1;
    }
    for (const NamedKey &n : kNamedKeys) {
        if (name.compare(QString::fromUtf8(n.portable), Qt::CaseInsensitive) == 0)
            return n.key;
    }
    return 0;
}

} // namespace

QString comboToPortableText(const KeyCombo &combo)
{
    QStringList parts;
    if (combo.modifiers & Modifier::Control)
        parts << QStringLiteral("Ctrl");
    if (combo.modifiers & Modifier::Alt)
        parts << QStringLiteral("Alt");
    if (combo.modifiers & Modifier::Shift)
        parts << QStringLiteral("Shift");
    if (combo.modifiers & Modifier::Meta)
        parts << QStringLiteral("Meta");
    if (combo.keypad)
        parts << QStringLiteral("Num");
    const QString key = baseKeyName(combo.key, true, OsStyle::Linux);
    parts << (key.isEmpty() ? QStringLiteral("0x%1").arg(combo.key, 0, 16) : key);
    return parts.join(QLatin1Char('+'));
}

std::optional<KeyCombo> comboFromPortableText(const QString &text)
{
    const QStringList parts = text.trimmed().split(QLatin1Char('+'), Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return std::nullopt;
    KeyCombo combo;
    for (int i = 0; i < parts.size() - 1; ++i) {
        const QString p = parts.at(i).trimmed().toLower();
        if (p == QLatin1String("ctrl") || p == QLatin1String("control"))
            combo.modifiers |= Modifier::Control;
        else if (p == QLatin1String("alt") || p == QLatin1String("option"))
            combo.modifiers |= Modifier::Alt;
        else if (p == QLatin1String("shift"))
            combo.modifiers |= Modifier::Shift;
        else if (p == QLatin1String("meta") || p == QLatin1String("cmd") || p == QLatin1String("win")
                 || p == QLatin1String("super"))
            combo.modifiers |= Modifier::Meta;
        else if (p == QLatin1String("num") || p == QLatin1String("keypad"))
            combo.keypad = true;
        else
            return std::nullopt; // unknown modifier
    }
    const QString last = parts.last().trimmed();
    if (last.startsWith(QLatin1String("0x"))) {
        bool ok = false;
        combo.key = last.mid(2).toInt(&ok, 16);
        if (!ok)
            return std::nullopt;
    } else {
        combo.key = keyFromPortableName(last);
    }
    if (combo.key == 0)
        return std::nullopt;
    return combo;
}

QString modifiersToDisplayText(Modifiers modifiers, OsStyle style)
{
    if (style == OsStyle::MacOS) {
        // macOS order and symbols: Control, Option, Shift, Command.
        QString mods;
        if (modifiers & Modifier::Control)
            mods += QStringLiteral("⌃");
        if (modifiers & Modifier::Alt)
            mods += QStringLiteral("⌥");
        if (modifiers & Modifier::Shift)
            mods += QStringLiteral("⇧");
        if (modifiers & Modifier::Meta)
            mods += QStringLiteral("⌘");
        return mods;
    }
    QStringList parts;
    if (modifiers & Modifier::Control)
        parts << QStringLiteral("Ctrl");
    if (modifiers & Modifier::Alt)
        parts << QStringLiteral("Alt");
    if (modifiers & Modifier::Shift)
        parts << QStringLiteral("Shift");
    if (modifiers & Modifier::Meta)
        parts << (style == OsStyle::Windows ? QStringLiteral("Win") : QStringLiteral("Super"));
    return parts.join(QLatin1Char('+'));
}

QString comboToDisplayText(const KeyCombo &combo, OsStyle style)
{
    QString key = baseKeyName(combo.key, false, style);
    if (key.isEmpty())
        key = keyToString(combo.key, false);
    if (combo.keypad)
        key = QStringLiteral("Num %1").arg(key);
    const QString mods = modifiersToDisplayText(combo.modifiers, style);
    if (style == OsStyle::MacOS || mods.isEmpty())
        return mods + key;
    return mods + QLatin1Char('+') + key;
}

} // namespace ergo
