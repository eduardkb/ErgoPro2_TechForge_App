#include "ui/keycontrol/KeyControlController.h"

#include "core/LogCategories.h"
#include "core/settings/KeyControlSettings.h"
#include "core/settings/SettingsStore.h"
#include "platform/PlatformFactory.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/InputService.h"
#include "ui/autoclick/AutoClickController.h"

#include <QJsonArray>

namespace ergo {

namespace {

OpResult toOp(const PlatformResult &r)
{
    return r.ok() ? OpResult::success() : OpResult::failure(r.message);
}

OsStyle styleFor(const QString &osName)
{
    if (osName == QLatin1String("macOS"))
        return OsStyle::MacOS;
    if (osName == QLatin1String("Windows"))
        return OsStyle::Windows;
    return OsStyle::Linux;
}

} // namespace

// Exclusive key registration through the shared input layer.
class KeyControlController::Registrar final : public IKeyRegistrar {
public:
    explicit Registrar(InputService &input) : m_input(input) {}
    OpResult registerKey(const KeyCombo &combo) override { return toOp(m_input.registerKey(combo)); }
    OpResult unregisterKey(const KeyCombo &combo) override { return toOp(m_input.unregisterKey(combo)); }
    bool isRegistered(const KeyCombo &combo) const override { return m_input.isKeyRegistered(combo); }

private:
    InputService &m_input;
};

// What actions do: clicks at the pointer via the injector, AutoClick hooks.
class KeyControlController::Context final : public KeyActionContext {
public:
    Context(Platform &platform, InputService &input, AutoClickController &autoclick)
        : m_platform(platform)
        , m_input(input)
        , m_autoclick(autoclick)
    {
    }
    OpResult leftClick() override { return toOp(m_platform.clickInjector->click(MouseButton::Left)); }
    OpResult rightClick() override { return toOp(m_platform.clickInjector->click(MouseButton::Right)); }
    OpResult doubleLeftClick() override { return toOp(m_platform.clickInjector->doubleClick(MouseButton::Left)); }
    void notifyKeyControlClick() override { m_autoclick.notifyExternalClick(m_input.stats().pointer); }
    OpResult toggleAutoClick() override
    {
        m_autoclick.toggle();
        return OpResult::success();
    }

private:
    Platform &m_platform;
    InputService &m_input;
    AutoClickController &m_autoclick;
};

KeyControlController::KeyControlController(SettingsStore &settings, FeatureAvailabilityService &availability,
                                           InputService &input, AutoClickController &autoclick, Platform &platform,
                                           QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_availability(availability)
    , m_input(input)
    , m_autoclick(autoclick)
    , m_platform(platform)
    , m_os(styleFor(platform.osName))
    , m_registrar(std::make_unique<Registrar>(input))
    , m_context(std::make_unique<Context>(platform, input, autoclick))
    , m_engine(std::make_unique<KeyControlEngine>(*m_registrar, m_actions, *m_context, m_os))
{
    m_engine->setInputAvailable(false); // until start() checks the input layer
}

KeyControlController::~KeyControlController()
{
    shutdown();
}

void KeyControlController::start()
{
    if (m_started)
        return;
    m_started = true;
    connect(&m_input.dispatcher(), &InputDispatcher::inputEvent, this, &KeyControlController::onInputEvent);
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key == KeyControlSettings::Actions && !m_saving)
            loadFromSettings(); // e.g. "Reset all settings": the list is cleared and keys released
    });
    connect(&m_availability, &FeatureAvailabilityService::changed, this, &KeyControlController::updateAvailability);
    connect(&m_input, &InputService::statusChanged, this, [this] {
        updateAvailability();
        // After a hook reinstall (watchdog, resume, unlock) make sure every key is registered (B.5.4).
        if (const int restored = m_engine->reapply())
            qCInfo(lcApp) << "Key Control: re-registered" << restored << "hotkey(s)";
        emit changed();
    });
    updateAvailability();
    loadFromSettings();
}

void KeyControlController::shutdown()
{
    if (!m_started)
        return;
    m_engine->releaseAll();
    m_started = false;
    qCInfo(lcApp) << "Key Control stopped; all hotkeys released";
}

QString KeyControlController::actionName(const QString &actionId) const
{
    const IKeyAction *action = m_actions.find(actionId);
    return action ? action->displayName() : actionId;
}

bool KeyControlController::isAvailable() const
{
    return m_availability.status(Feature::KeyControl).available && m_input.stats().running;
}

void KeyControlController::updateAvailability()
{
    const bool available = isAvailable();
    if (available == m_engine->isInputAvailable())
        return;
    m_engine->setInputAvailable(available);
    qCInfo(lcApp) << "Key Control is" << (available ? "available" : "unavailable");
    emit changed();
}

void KeyControlController::loadFromSettings()
{
    QStringList problems;
    const QList<KeyBinding> list = KeyBindingStore::fromJson(
        m_settings.value(KeyControlSettings::Actions).value<QJsonArray>(), m_actions, m_os, &problems);
    for (const QString &p : std::as_const(problems))
        qCWarning(lcApp).noquote() << "Key Control:" << p;
    m_engine->load(list);
    int active = 0;
    for (const KeyBinding &b : list) {
        const BindingState s = m_engine->state(b.id);
        if (s.status == BindingStatus::Active)
            ++active;
        else if (s.status == BindingStatus::Error && b.isValid())
            qCWarning(lcApp).noquote() << "Key Control:" << displayText(b.combo) << "not registered:" << s.reason;
    }
    qCInfo(lcApp) << "Key Control: loaded" << list.size() << "hotkey(s)," << active << "active";
    emit changed();
}

void KeyControlController::save()
{
    m_saving = true;
    if (!m_settings.setValue(KeyControlSettings::Actions, QVariant::fromValue(KeyBindingStore::toJson(bindings()))))
        qCCritical(lcApp) << "Key Control: the hotkey list could not be stored";
    m_saving = false;
    emit changed();
}

OpResult KeyControlController::add(const KeyCombo &combo, const QString &actionId)
{
    const OpResult r = m_engine->add(combo, actionId);
    if (r.ok) {
        qCInfo(lcApp).noquote() << "Hotkey added:" << displayText(combo) << "->" << actionName(actionId);
        save();
    } else {
        qCWarning(lcApp).noquote() << "Hotkey" << displayText(combo) << "not added:" << r.message;
    }
    return r;
}

OpResult KeyControlController::modify(const QString &id, const KeyCombo &combo, const QString &actionId)
{
    const OpResult r = m_engine->modify(id, combo, actionId);
    if (r.ok) {
        qCInfo(lcApp).noquote() << "Hotkey modified:" << displayText(combo) << "->" << actionName(actionId);
        save();
    } else {
        qCWarning(lcApp).noquote() << "Hotkey not modified:" << r.message;
    }
    return r;
}

OpResult KeyControlController::setEnabled(const QString &id, bool enabled)
{
    const OpResult r = m_engine->setEnabled(id, enabled);
    if (const KeyBinding *b = m_engine->binding(id))
        qCInfo(lcApp).noquote() << "Hotkey" << displayText(b->combo) << (enabled ? "enabled" : "disabled");
    save(); // the enabled flag is stored even if the registration failed
    return r;
}

OpResult KeyControlController::resetBinding(const QString &id)
{
    const OpResult r = m_engine->resetBinding(id);
    if (const KeyBinding *b = m_engine->binding(id))
        qCInfo(lcApp).noquote() << "Hotkey binding reset:" << displayText(b->combo) << (r.ok ? "OK" : r.message);
    emit changed();
    return r;
}

OpResult KeyControlController::remove(const QString &id)
{
    const KeyBinding *b = m_engine->binding(id);
    const QString text = b && b->isValid() ? displayText(b->combo) : id;
    const OpResult r = m_engine->remove(id);
    if (r.ok) {
        qCInfo(lcApp).noquote() << "Hotkey deleted:" << text;
        save();
    }
    return r;
}

void KeyControlController::setPaused(bool paused)
{
    m_engine->setPaused(paused);
    qCInfo(lcApp) << "Key Control hotkeys" << (paused ? "paused (hotkey dialog open)" : "resumed");
    emit changed();
}

void KeyControlController::onInputEvent(const InputEvent &event)
{
    if (!event.isKey())
        return;
    const auto fired = m_engine->onKeyEvent(event);
    if (!fired)
        return;
    const KeyBinding *b = m_engine->binding(*fired);
    const OpResult result = m_engine->lastActionResult();
    if (result.ok) {
        qCInfo(lcApp).noquote() << "Hotkey" << (b ? displayText(b->combo) : *fired) << "->"
                                << (b ? actionName(b->actionId) : QString());
    } else {
        qCWarning(lcApp).noquote() << "Hotkey action failed:" << result.message;
        emit actionFailed(result.message);
    }
}

} // namespace ergo
