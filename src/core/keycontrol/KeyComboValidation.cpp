#include "core/keycontrol/KeyComboValidation.h"

#include <QCoreApplication>

namespace ergo {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::KeyComboValidation", text);
}

struct Reserved {
    Modifiers modifiers;
    int key;
    const char *reason;
};

// Never-capturable or system-critical combinations (B.5.1, REVIEW 1.3).
const Reserved kWindowsReserved[] = {
    {Modifier::Control | Modifier::Alt, Qt::Key_Delete, QT_TRANSLATE_NOOP("ergo::KeyComboValidation",
                                                                         "Ctrl+Alt+Del is handled by Windows itself "
                                                                         "and cannot be captured.")},
    {Modifier::Control | Modifier::Shift, Qt::Key_Escape,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Ctrl+Shift+Esc opens the Task Manager.")},
    {Modifier::Control, Qt::Key_Escape, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Ctrl+Esc opens the Start menu.")},
    {Modifier::Alt, Qt::Key_Tab, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Alt+Tab switches between windows.")},
    {Modifier::Alt | Modifier::Shift, Qt::Key_Tab,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Alt+Shift+Tab switches between windows.")},
    {Modifier::Alt, Qt::Key_Escape, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Alt+Esc switches between windows.")},
    {Modifier::Alt, Qt::Key_F4, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Alt+F4 closes windows.")},
};

const Reserved kMacReserved[] = {
    {Modifier::Meta, Qt::Key_Tab, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⌘Tab switches between apps.")},
    {Modifier::Meta | Modifier::Shift, Qt::Key_Tab,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⇧⌘Tab switches between apps.")},
    {Modifier::Meta, Qt::Key_Space, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⌘Space opens Spotlight.")},
    {Modifier::Control, Qt::Key_Space,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⌃Space switches the input source.")},
    {Modifier::Meta | Modifier::Alt, Qt::Key_Escape,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⌥⌘Esc opens Force Quit Applications.")},
    {Modifier::Control | Modifier::Meta, Qt::Key_Q, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⌃⌘Q locks the screen.")},
    {Modifier::Meta | Modifier::Shift, Qt::Key_3,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⇧⌘3 takes a screenshot.")},
    {Modifier::Meta | Modifier::Shift, Qt::Key_4,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⇧⌘4 takes a screenshot.")},
    {Modifier::Meta | Modifier::Shift, Qt::Key_5,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⇧⌘5 opens the screenshot tool.")},
    {Modifier::Meta | Modifier::Shift, Qt::Key_Q, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "⇧⌘Q logs out.")},
};

const Reserved kLinuxReserved[] = {
    {Modifier::Control | Modifier::Alt, Qt::Key_Delete,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Ctrl+Alt+Del is handled by the system.")},
    {Modifier::Control | Modifier::Alt, Qt::Key_Backspace,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Ctrl+Alt+Backspace can terminate the graphical session.")},
    {Modifier::Alt, Qt::Key_Tab, QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Alt+Tab switches between windows.")},
    {Modifier::Alt | Modifier::Shift, Qt::Key_Tab,
     QT_TRANSLATE_NOOP("ergo::KeyComboValidation", "Alt+Shift+Tab switches between windows.")},
};

template <size_t N>
const char *findReserved(const Reserved (&table)[N], const KeyCombo &combo)
{
    for (const Reserved &r : table) {
        if (r.key == combo.key && r.modifiers == combo.modifiers && !combo.keypad)
            return r.reason;
    }
    return nullptr;
}

bool isPlainTypingKey(int key)
{
    // Keys that type characters or edit text; without a modifier they would no
    // longer work in other applications.
    if ((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9))
        return true;
    switch (key) {
    case Qt::Key_Space: case Qt::Key_Return: case Qt::Key_Enter: case Qt::Key_Tab:
    case Qt::Key_Backspace: case Qt::Key_Delete: case Qt::Key_Escape: case Qt::Key_Insert:
    case Qt::Key_Left: case Qt::Key_Right: case Qt::Key_Up: case Qt::Key_Down:
    case Qt::Key_Home: case Qt::Key_End: case Qt::Key_PageUp: case Qt::Key_PageDown:
    case Qt::Key_Minus: case Qt::Key_Equal: case Qt::Key_BracketLeft: case Qt::Key_BracketRight:
    case Qt::Key_Backslash: case Qt::Key_Semicolon: case Qt::Key_Apostrophe: case Qt::Key_Comma:
    case Qt::Key_Period: case Qt::Key_Slash: case Qt::Key_QuoteLeft: case Qt::Key_Plus:
    case Qt::Key_Asterisk: case Qt::Key_CapsLock: case Qt::Key_NumLock: case Qt::Key_ScrollLock:
        return true;
    default:
        return false;
    }
}

} // namespace

ComboCheck validateCombo(const KeyCombo &combo, OsStyle os)
{
    ComboCheck check;
    if (combo.key == 0) {
        check.result = ComboCheck::Result::Rejected;
        check.message = tr("This key cannot be recognized (e.g. Fn / Globe or a media key). Choose another key.");
        return check;
    }
    if (isModifierKey(combo.key)) {
        check.result = ComboCheck::Result::Rejected;
        check.message = tr("A modifier key alone (Shift, Ctrl, Alt, Cmd/Win) cannot be used. Hold it and press "
                           "another key.");
        return check;
    }

    const char *reason = nullptr;
    switch (os) {
    case OsStyle::Windows:
        reason = findReserved(kWindowsReserved, combo);
        if (!reason && (combo.modifiers & Modifier::Meta))
            reason = QT_TRANSLATE_NOOP("ergo::KeyComboValidation",
                                       "Windows reserves most Win+key shortcuts for itself. Use Ctrl, Alt or "
                                       "Shift instead.");
        break;
    case OsStyle::MacOS:
        reason = findReserved(kMacReserved, combo);
        break;
    case OsStyle::Linux:
        reason = findReserved(kLinuxReserved, combo);
        if (!reason && combo.modifiers == (Modifier::Control | Modifier::Alt) && combo.key >= Qt::Key_F1
            && combo.key <= Qt::Key_F12)
            reason = QT_TRANSLATE_NOOP("ergo::KeyComboValidation",
                                       "Ctrl+Alt+F1…F12 switch to the text consoles.");
        break;
    }
    if (reason) {
        check.result = ComboCheck::Result::Rejected;
        check.message = tr("This combination is reserved by the operating system: %1").arg(tr(reason));
        return check;
    }

    const bool functionKey = combo.key >= Qt::Key_F1 && combo.key <= Qt::Key_F24;
    if (combo.modifiers == Modifier::None && !functionKey && !combo.keypad && isPlainTypingKey(combo.key)) {
        check.result = ComboCheck::Result::Warning;
        check.message = tr("This key will no longer type or work in other applications while the hotkey is "
                           "enabled. Prefer a function key (F1–F24), a keypad key or a combination with a "
                           "modifier.");
    }
    return check;
}

} // namespace ergo
