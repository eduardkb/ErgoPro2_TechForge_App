#include "core/input/KeyCombo.h"

#include <QMutexLocker>

namespace ergo {

QString KeyCombo::toString() const
{
    const QString mods = modifiersToString(modifiers);
    return (mods.isEmpty() ? QString() : mods + QLatin1Char('+')) + keyToString(key, keypad);
}

bool KeyCombo::matches(const InputEvent &event) const
{
    if (!event.isKey() || event.key != key || event.keypad != keypad)
        return false;
    const Modifiers held = event.modifiers & ~modifierForKey(event.key);
    return held == modifiers;
}

bool KeySuppressionRegistry::add(const KeyCombo &combo)
{
    if (!combo.isValid())
        return false;
    QMutexLocker lock(&m_mutex);
    if (m_combos.contains(combo))
        return false;
    m_combos.insert(combo);
    m_count.store(int(m_combos.size()));
    return true;
}

bool KeySuppressionRegistry::remove(const KeyCombo &combo)
{
    QMutexLocker lock(&m_mutex);
    if (!m_combos.remove(combo))
        return false;
    m_count.store(int(m_combos.size()));
    return true;
}

bool KeySuppressionRegistry::contains(const KeyCombo &combo) const
{
    QMutexLocker lock(&m_mutex);
    return m_combos.contains(combo);
}

QList<KeyCombo> KeySuppressionRegistry::combos() const
{
    QMutexLocker lock(&m_mutex);
    return m_combos.values();
}

quint32 KeySuppressionRegistry::pressKey(const InputEvent &event)
{
    // Prefer the native code (unique per physical key); fall back to the Qt key.
    return event.nativeKeyCode != 0 ? event.nativeKeyCode : 0x80000000u | quint32(event.key);
}

bool KeySuppressionRegistry::shouldSwallow(const InputEvent &event)
{
    if (!event.isKey() || event.synthetic)
        return false;
    // Fast path: nothing registered and no swallowed key still held.
    if (m_count.load() == 0 && m_heldCount.load() == 0)
        return false;

    QMutexLocker lock(&m_mutex);
    const quint32 id = pressKey(event);
    bool swallow = false;
    if (event.type == InputEventType::KeyDown) {
        if (m_heldSwallowed.contains(id)) {
            swallow = true; // auto-repeat of a swallowed key
        } else {
            for (const KeyCombo &combo : std::as_const(m_combos)) {
                if (combo.matches(event)) {
                    m_heldSwallowed.insert(id);
                    swallow = true;
                    break;
                }
            }
        }
    } else if (m_heldSwallowed.remove(id)) {
        swallow = true; // key-up of a swallowed key-down
    }
    m_heldCount.store(int(m_heldSwallowed.size()));
    if (swallow)
        m_swallowed.fetch_add(1);
    return swallow;
}

void KeySuppressionRegistry::setDragLockHolding(bool holding)
{
    if (holding)
        m_consumedButtons.store(0); // forget a release lost by a hook restart
    m_consumeArmed.store(holding);
    m_dragHolding.store(holding);
}

bool KeySuppressionRegistry::shouldSwallowMouse(const InputEvent &event)
{
    if (!event.isMouseButton() || event.synthetic)
        return false;
    const quint32 bit = 1u << quint32(event.button);
    if (event.type == InputEventType::MouseDown) {
        if (m_consumedButtons.load() & bit) {
            m_swallowed.fetch_add(1);
            return true; // repeated press while the consumed one is still down
        }
        bool armed = true;
        if (!m_consumeArmed.compare_exchange_strong(armed, false))
            return false;
        m_consumedButtons.fetch_or(bit);
        m_swallowed.fetch_add(1);
        return true;
    }
    // MouseUp: swallow the release of a consumed press, even after the drag
    // lock ended, so applications never see a release without a press.
    if (!(m_consumedButtons.fetch_and(~bit) & bit))
        return false;
    m_swallowed.fetch_add(1);
    return true;
}

} // namespace ergo
