#pragma once

#include "core/keycontrol/KeyAction.h"
#include "core/keycontrol/KeyControlEngine.h"

#include <QObject>

#include <memory>

namespace ergo {

class AutoClickController;
class FeatureAvailabilityService;
class InputService;
class SettingsStore;
struct Platform;

// GUI-thread side of Key Control (B.5): connects the engine to the input layer
// (exclusive key registration, key events), the click injector, AutoClick and
// the settings. Persists every change to keycontrol.actions.
class KeyControlController : public QObject {
    Q_OBJECT
public:
    KeyControlController(SettingsStore &settings, FeatureAvailabilityService &availability, InputService &input,
                         AutoClickController &autoclick, Platform &platform, QObject *parent = nullptr);
    ~KeyControlController() override;

    void start();
    void shutdown();

    OsStyle osStyle() const { return m_os; }
    QString displayText(const KeyCombo &combo) const { return comboToDisplayText(combo, m_os); }
    QString modifiersText(Modifiers modifiers) const { return modifiersToDisplayText(modifiers, m_os); }
    QString actionName(const QString &actionId) const;
    // Actions without a hotkey yet (one hotkey per action); the hotkey being
    // modified (exceptId) keeps its own action.
    QList<IKeyAction *> selectableActions(const QString &exceptId = {}) const;
    bool isAvailable() const;

    const QList<KeyBinding> &bindings() const { return m_engine->bindings(); }
    BindingState state(const QString &id) const { return m_engine->state(id); }
    ComboCheck check(const KeyCombo &combo, const QString &exceptId = {}) const
    {
        return m_engine->check(combo, exceptId);
    }

    OpResult add(const KeyCombo &combo, const QString &actionId);
    OpResult modify(const QString &id, const KeyCombo &combo, const QString &actionId);
    OpResult setEnabled(const QString &id, bool enabled);
    OpResult resetBinding(const QString &id);
    OpResult remove(const QString &id);

    // K-14: no hotkey is active while the Add/Modify dialog is open.
    void setPaused(bool paused);

signals:
    void changed();
    void actionFailed(const QString &message);

private:
    class Registrar;
    class Context;

    void loadFromSettings();
    void save();
    void updateAvailability();
    void onInputEvent(const InputEvent &event);

    SettingsStore &m_settings;
    FeatureAvailabilityService &m_availability;
    InputService &m_input;
    AutoClickController &m_autoclick;
    Platform &m_platform;
    OsStyle m_os;
    KeyActionRegistry m_actions;
    std::unique_ptr<Registrar> m_registrar;
    std::unique_ptr<Context> m_context;
    std::unique_ptr<KeyControlEngine> m_engine;
    bool m_saving = false;
    bool m_started = false;
};

} // namespace ergo
