#pragma once

#include "core/input/KeyCombo.h"

#include <QString>

#include <optional>

namespace ergo {

// Operating system flavor for display text and reserved-combo rules.
enum class OsStyle { MacOS, Windows, Linux };

// Portable, OS-independent text stored in settings.json, e.g.
// "Ctrl+Shift+F9", "Meta+Num+7", "Alt+Slash". Modifier order on output:
// Ctrl, Alt, Shift, Meta; any order is accepted when parsing.
QString comboToPortableText(const KeyCombo &combo);
std::optional<KeyCombo> comboFromPortableText(const QString &text);

// Text shown to the user: "⌃⌥⇧⌘F9" on macOS, "Ctrl+Alt+Shift+Win+F9" on
// Windows, "Ctrl+Alt+Shift+Super+F9" on Linux.
QString comboToDisplayText(const KeyCombo &combo, OsStyle style);

// Only the modifiers, in the same style ("⌃⇧" / "Ctrl+Shift"); empty for none.
QString modifiersToDisplayText(Modifiers modifiers, OsStyle style);

} // namespace ergo
