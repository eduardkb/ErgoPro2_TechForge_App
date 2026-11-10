#pragma once

#include "core/input/InputEvent.h"
#include "core/keycontrol/KeyAction.h"
#include "core/keycontrol/KeyBinding.h"
#include "core/keycontrol/KeyComboValidation.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

#include <optional>

namespace ergo {

// Registers key combos with the input layer (implemented by InputService).
class IKeyRegistrar {
public:
    virtual ~IKeyRegistrar() = default;
    virtual OpResult registerKey(const KeyCombo &combo) = 0;
    virtual OpResult unregisterKey(const KeyCombo &combo) = 0;
    virtual bool isRegistered(const KeyCombo &combo) const = 0;
};

enum class BindingStatus { Active, Disabled, Paused, Unavailable, Error };

struct BindingState {
    BindingStatus status = BindingStatus::Disabled;
    QString reason; // Error / Unavailable: why
};

// Key Control logic (B.5): keeps the bindings, registers enabled ones as
// exclusive keys, fires the action once per key press and tracks a status per
// binding. Pure (no Qt event loop, no OS calls) so it is unit-testable.
class KeyControlEngine {
public:
    KeyControlEngine(IKeyRegistrar &registrar, const KeyActionRegistry &actions, KeyActionContext &context,
                     OsStyle os);
    ~KeyControlEngine();

    // Replaces all bindings (registrations of the old ones are released).
    void load(const QList<KeyBinding> &bindings);
    const QList<KeyBinding> &bindings() const { return m_bindings; }
    const KeyBinding *binding(const QString &id) const;
    BindingState state(const QString &id) const;

    // Validation incl. duplicates (another binding than exceptId uses the combo).
    ComboCheck check(const KeyCombo &combo, const QString &exceptId = {}) const;

    // One hotkey per action (1.4.1): visible actions not yet used by another
    // listed hotkey (disabled ones included). exceptId: the hotkey being
    // modified keeps its own action selectable.
    QStringList availableActionIds(const QString &exceptId = {}) const;

    OpResult add(const KeyCombo &combo, const QString &actionId, QString *newId = nullptr);
    OpResult modify(const QString &id, const KeyCombo &combo, const QString &actionId);
    OpResult setEnabled(const QString &id, bool enabled);
    OpResult resetBinding(const QString &id); // unregister + register again ("repair")
    OpResult remove(const QString &id);

    // K-14: while the Add/Modify dialog is open every key is released.
    void setPaused(bool paused);
    bool isPaused() const { return m_paused; }

    // False while Key Control is unavailable (permissions, hook not running).
    void setInputAvailable(bool available);
    bool isInputAvailable() const { return m_inputAvailable; }

    // Makes sure every active binding is registered (after a hook reinstall,
    // resume or unlock, B.5.4). Returns the number of re-registered keys.
    int reapply();

    // Fires the action of a matching enabled binding on key-down (no repeats).
    // Returns the binding id that fired, if any.
    std::optional<QString> onKeyEvent(const InputEvent &event);
    OpResult lastActionResult() const { return m_lastActionResult; }

    void releaseAll();

private:
    KeyBinding *findBinding(const QString &id);
    OpResult checkAction(const QString &actionId, const QString &exceptId) const;
    void apply(KeyBinding &binding);
    void release(KeyBinding &binding);

    IKeyRegistrar &m_registrar;
    const KeyActionRegistry &m_actions;
    KeyActionContext &m_context;
    OsStyle m_os;
    QList<KeyBinding> m_bindings;
    QHash<QString, BindingState> m_states;
    QSet<QString> m_registered; // binding ids whose combo is registered
    bool m_paused = false;
    bool m_inputAvailable = true;
    OpResult m_lastActionResult;
};

} // namespace ergo
