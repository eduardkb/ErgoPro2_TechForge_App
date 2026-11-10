#include "core/keycontrol/KeyControlEngine.h"

#include <QCoreApplication>

namespace ergo {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::KeyControlEngine", text);
}

} // namespace

KeyControlEngine::KeyControlEngine(IKeyRegistrar &registrar, const KeyActionRegistry &actions,
                                   KeyActionContext &context, OsStyle os)
    : m_registrar(registrar)
    , m_actions(actions)
    , m_context(context)
    , m_os(os)
{
}

KeyControlEngine::~KeyControlEngine()
{
    releaseAll();
}

void KeyControlEngine::releaseAll()
{
    for (KeyBinding &b : m_bindings)
        release(b);
}

void KeyControlEngine::load(const QList<KeyBinding> &bindings)
{
    releaseAll();
    m_bindings = bindings;
    m_states.clear();
    for (KeyBinding &b : m_bindings)
        apply(b);
}

KeyBinding *KeyControlEngine::findBinding(const QString &id)
{
    for (KeyBinding &b : m_bindings) {
        if (b.id == id)
            return &b;
    }
    return nullptr;
}

const KeyBinding *KeyControlEngine::binding(const QString &id) const
{
    for (const KeyBinding &b : m_bindings) {
        if (b.id == id)
            return &b;
    }
    return nullptr;
}

BindingState KeyControlEngine::state(const QString &id) const
{
    return m_states.value(id);
}

void KeyControlEngine::release(KeyBinding &b)
{
    if (!m_registered.remove(b.id))
        return;
    m_registrar.unregisterKey(b.combo); // a failure only means the key was not registered
}

void KeyControlEngine::apply(KeyBinding &b)
{
    BindingState &s = m_states[b.id];
    s.reason.clear();
    if (!b.isValid()) {
        release(b);
        s.status = BindingStatus::Error;
        s.reason = b.loadError;
        return;
    }
    if (!b.enabled) {
        release(b);
        s.status = BindingStatus::Disabled;
        return;
    }
    if (m_paused) {
        release(b);
        s.status = BindingStatus::Paused;
        return;
    }
    if (!m_inputAvailable) {
        release(b);
        s.status = BindingStatus::Unavailable;
        s.reason = tr("Key Control is not available (see the message at the top).");
        return;
    }
    if (!m_registered.contains(b.id)) {
        const OpResult r = m_registrar.registerKey(b.combo);
        if (!r.ok) {
            s.status = BindingStatus::Error;
            s.reason = r.message;
            return;
        }
        m_registered.insert(b.id);
    }
    s.status = BindingStatus::Active;
}

ComboCheck KeyControlEngine::check(const KeyCombo &combo, const QString &exceptId) const
{
    ComboCheck c = validateCombo(combo, m_os);
    if (!c.accepted())
        return c;
    for (const KeyBinding &b : m_bindings) {
        if (b.id != exceptId && b.isValid() && b.combo == combo) {
            const IKeyAction *action = m_actions.find(b.actionId);
            c.result = ComboCheck::Result::Rejected;
            c.message = tr("%1 is already assigned to \"%2\".")
                            .arg(comboToDisplayText(combo, m_os), action ? action->displayName() : b.actionId);
            return c;
        }
    }
    return c;
}

OpResult KeyControlEngine::checkAction(const QString &actionId, const QString &exceptId) const
{
    const IKeyAction *action = m_actions.find(actionId);
    if (!action)
        return OpResult::failure(tr("Unknown action."));
    for (const KeyBinding &b : m_bindings) {
        if (b.id != exceptId && b.isValid() && b.actionId == actionId)
            return OpResult::failure(tr("\"%1\" is already assigned to %2. Each action can have only one hotkey.")
                                         .arg(action->displayName(), comboToDisplayText(b.combo, m_os)));
    }
    return OpResult::success();
}

QStringList KeyControlEngine::availableActionIds(const QString &exceptId) const
{
    QStringList ids;
    for (const IKeyAction *a : m_actions.visibleActions()) {
        if (checkAction(a->id(), exceptId).ok)
            ids << a->id();
    }
    return ids;
}

OpResult KeyControlEngine::add(const KeyCombo &combo, const QString &actionId, QString *newId)
{
    const ComboCheck c = check(combo);
    if (!c.accepted())
        return OpResult::failure(c.message);
    if (const OpResult a = checkAction(actionId, {}); !a.ok)
        return a;
    KeyBinding b;
    b.id = KeyBindingStore::newId();
    b.combo = combo;
    b.actionId = actionId;
    b.enabled = true;
    m_bindings.append(b);
    KeyBinding &added = m_bindings.last();
    apply(added);
    const BindingState s = state(added.id);
    if (s.status == BindingStatus::Error) {
        // Registration refused (e.g. grabbed by another application): do not keep it.
        const QString reason = s.reason;
        m_states.remove(added.id);
        m_bindings.removeLast();
        return OpResult::failure(reason);
    }
    if (newId)
        *newId = added.id;
    return OpResult::success();
}

OpResult KeyControlEngine::modify(const QString &id, const KeyCombo &combo, const QString &actionId)
{
    KeyBinding *b = findBinding(id);
    if (!b)
        return OpResult::failure(tr("Unknown hotkey."));
    const ComboCheck c = check(combo, id);
    if (!c.accepted())
        return OpResult::failure(c.message);
    if (const OpResult a = checkAction(actionId, id); !a.ok)
        return a;

    const KeyBinding previous = *b;
    release(*b);
    b->combo = combo;
    b->actionId = actionId;
    b->loadError.clear();
    b->raw = {};
    apply(*b);
    if (state(id).status == BindingStatus::Error) {
        const QString reason = state(id).reason;
        release(*b);
        *b = previous; // keep the working binding
        apply(*b);
        return OpResult::failure(reason);
    }
    return OpResult::success();
}

OpResult KeyControlEngine::setEnabled(const QString &id, bool enabled)
{
    KeyBinding *b = findBinding(id);
    if (!b)
        return OpResult::failure(tr("Unknown hotkey."));
    b->enabled = enabled;
    apply(*b); // disabling releases the key to the OS (K-03)
    const BindingState s = state(id);
    return s.status == BindingStatus::Error ? OpResult::failure(s.reason) : OpResult::success();
}

OpResult KeyControlEngine::resetBinding(const QString &id)
{
    KeyBinding *b = findBinding(id);
    if (!b)
        return OpResult::failure(tr("Unknown hotkey."));
    release(*b);
    apply(*b); // registers again from scratch (K-02a)
    const BindingState s = state(id);
    return s.status == BindingStatus::Error ? OpResult::failure(s.reason) : OpResult::success();
}

OpResult KeyControlEngine::remove(const QString &id)
{
    for (int i = 0; i < m_bindings.size(); ++i) {
        if (m_bindings[i].id == id) {
            release(m_bindings[i]);
            m_states.remove(id);
            m_bindings.removeAt(i);
            return OpResult::success();
        }
    }
    return OpResult::failure(tr("Unknown hotkey."));
}

void KeyControlEngine::setPaused(bool paused)
{
    if (paused == m_paused)
        return;
    m_paused = paused;
    for (KeyBinding &b : m_bindings)
        apply(b);
}

void KeyControlEngine::setInputAvailable(bool available)
{
    if (available == m_inputAvailable)
        return;
    m_inputAvailable = available;
    for (KeyBinding &b : m_bindings)
        apply(b);
}

int KeyControlEngine::reapply()
{
    int count = 0;
    for (KeyBinding &b : m_bindings) {
        // A registration lost by the input layer is registered again.
        if (m_registered.contains(b.id) && !m_registrar.isRegistered(b.combo)) {
            m_registered.remove(b.id);
            ++count;
        }
        apply(b);
    }
    return count;
}

std::optional<QString> KeyControlEngine::onKeyEvent(const InputEvent &event)
{
    if (m_paused || !m_inputAvailable || event.type != InputEventType::KeyDown || event.synthetic || event.autoRepeat)
        return std::nullopt;
    for (const KeyBinding &b : m_bindings) {
        if (!b.isValid() || !b.enabled || state(b.id).status != BindingStatus::Active || !b.combo.matches(event))
            continue;
        IKeyAction *action = m_actions.find(b.actionId);
        m_lastActionResult = action ? action->execute(m_context) : OpResult::failure(tr("Unknown action."));
        return b.id;
    }
    return std::nullopt;
}

} // namespace ergo
