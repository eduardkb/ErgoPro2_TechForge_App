#pragma once

#include "core/input/InputEvent.h"

#include <QHash>
#include <QList>
#include <QMutex>
#include <QSet>
#include <QString>

#include <atomic>

namespace ergo {

// A key plus the exact set of modifiers that must be held (B.5.1).
struct KeyCombo {
    int key = 0;                       // Qt::Key
    Modifiers modifiers = Modifier::None;
    bool keypad = false;

    bool isValid() const { return key != 0 && !isModifierKey(key); }
    QString toString() const;

    // True if the key event is this combo. Modifiers must match exactly
    // (Ctrl+F9 does not match Ctrl+Shift+F9); the modifier that IS the key is ignored.
    bool matches(const InputEvent &event) const;

    friend bool operator==(const KeyCombo &a, const KeyCombo &b)
    {
        return a.key == b.key && a.modifiers == b.modifiers && a.keypad == b.keypad;
    }
};

inline size_t qHash(const KeyCombo &c, size_t seed = 0)
{
    return ::qHash(c.key, seed) ^ (size_t(c.modifiers) << 1) ^ (c.keypad ? 0x9e3779b9u : 0u);
}

// Thread-safe set of key combos that must not reach other applications.
// shouldSwallow() is called by the platform hook on its own thread for every
// key event and must be fast: a short mutex and an atomic fast path.
//
// Once a key-down was swallowed, its auto-repeats and its key-up are swallowed
// too (even if the combo is unregistered meanwhile), so applications never see
// a key-up without a key-down.
class KeySuppressionRegistry {
public:
    bool add(const KeyCombo &combo);    // false if already registered or invalid
    bool remove(const KeyCombo &combo); // false if not registered
    bool contains(const KeyCombo &combo) const;
    QList<KeyCombo> combos() const;
    bool isEmpty() const { return m_count.load() == 0; }

    bool shouldSwallow(const InputEvent &event);

    quint64 swallowedCount() const { return m_swallowed.load(); }

private:
    static quint32 pressKey(const InputEvent &event);

    mutable QMutex m_mutex;
    QSet<KeyCombo> m_combos;
    QSet<quint32> m_heldSwallowed; // keys whose key-down was swallowed and are still held
    std::atomic<int> m_count{0};
    std::atomic<int> m_heldCount{0};
    std::atomic<quint64> m_swallowed{0};
};

} // namespace ergo
