#pragma once

#include "core/input/KeyCombo.h"
#include "core/keycontrol/KeyComboText.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

namespace ergo {

class KeyActionRegistry;

// One configured hotkey (B.5.2). Entries that could not be read keep their
// original JSON ('raw') and an error, so they are shown with an Error status
// and written back unchanged instead of being dropped.
struct KeyBinding {
    QString id;
    KeyCombo combo;
    QString actionId;
    bool enabled = true;
    QString loadError; // non-empty: the stored entry is invalid
    QJsonObject raw;   // original entry (kept for invalid ones)

    bool isValid() const { return loadError.isEmpty(); }
};

namespace KeyBindingStore {

QString newId();

// Reads keycontrol.actions. Invalid entries get loadError set; 'problems'
// receives one human-readable line per invalid entry (for the log).
QList<KeyBinding> fromJson(const QJsonArray &array, const KeyActionRegistry &actions, OsStyle os,
                           QStringList *problems = nullptr);

QJsonArray toJson(const QList<KeyBinding> &bindings);

} // namespace KeyBindingStore
} // namespace ergo
