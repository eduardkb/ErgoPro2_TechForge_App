#pragma once

#include "core/input/KeyCombo.h"
#include "core/keycontrol/KeyComboText.h"

#include <QString>

namespace ergo {

// Result of checking a key combination for use as a hotkey (B.5.1).
struct ComboCheck {
    enum class Result {
        Ok,
        Warning, // allowed after explicit confirmation (plain typing key)
        Rejected // never allowed (reserved by the OS, modifier only, unknown key)
    };
    Result result = Result::Ok;
    QString message;

    bool accepted() const { return result != Result::Rejected; }
};

// Allowed: F1-F24, numeric keypad keys, and any key with at least one modifier.
// A plain key that normally types or edits text needs confirmation.
// Combinations the OS never lets applications capture (or that would break
// the system) are rejected with an explanation. Duplicates are checked by
// KeyControlEngine.
ComboCheck validateCombo(const KeyCombo &combo, OsStyle os);

} // namespace ergo
