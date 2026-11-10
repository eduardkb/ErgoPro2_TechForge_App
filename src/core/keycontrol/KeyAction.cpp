#include "core/keycontrol/KeyAction.h"

#include <QCoreApplication>

namespace ergo {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::KeyAction", text);
}

class LeftClickAction final : public IKeyAction {
public:
    QString id() const override { return KeyActionRegistry::LeftClick; }
    QString displayName() const override { return tr("Left mouse click"); }
    OpResult execute(KeyActionContext &c) override
    {
        const OpResult r = c.leftClick();
        if (r.ok)
            c.notifyKeyControlClick();
        return r;
    }
};

class RightClickAction final : public IKeyAction {
public:
    QString id() const override { return KeyActionRegistry::RightClick; }
    QString displayName() const override { return tr("Right mouse click"); }
    OpResult execute(KeyActionContext &c) override
    {
        const OpResult r = c.rightClick();
        if (r.ok)
            c.notifyKeyControlClick();
        return r;
    }
};

class DoubleLeftClickAction final : public IKeyAction {
public:
    QString id() const override { return KeyActionRegistry::DoubleLeftClick; }
    QString displayName() const override { return tr("Double left click"); }
    OpResult execute(KeyActionContext &c) override
    {
        const OpResult r = c.doubleLeftClick();
        if (r.ok)
            c.notifyKeyControlClick();
        return r;
    }
};

class ToggleAutoClickAction final : public IKeyAction {
public:
    QString id() const override { return KeyActionRegistry::ToggleAutoClick; }
    QString displayName() const override { return tr("Turn Auto Click on/off"); }
    OpResult execute(KeyActionContext &c) override { return c.toggleAutoClick(); }
};

// Hold the left button until released (B.5.3); pressing the hotkey again releases it.
class DragLockAction final : public IKeyAction {
public:
    QString id() const override { return KeyActionRegistry::DragLock; }
    QString displayName() const override { return tr("Drag lock (left button)"); }
    OpResult execute(KeyActionContext &c) override { return c.toggleDragLock(); }
};

} // namespace

KeyActionRegistry::KeyActionRegistry()
{
    add(std::make_unique<LeftClickAction>());
    add(std::make_unique<RightClickAction>());
    add(std::make_unique<DoubleLeftClickAction>());
    add(std::make_unique<ToggleAutoClickAction>());
    add(std::make_unique<DragLockAction>());
}

void KeyActionRegistry::add(std::unique_ptr<IKeyAction> action)
{
    if (!action || find(action->id()))
        return;
    m_actions.append(std::shared_ptr<IKeyAction>(std::move(action)));
}

IKeyAction *KeyActionRegistry::find(const QString &id) const
{
    for (const auto &a : m_actions) {
        if (a->id() == id)
            return a.get();
    }
    return nullptr;
}

QList<IKeyAction *> KeyActionRegistry::visibleActions() const
{
    QList<IKeyAction *> list;
    for (const auto &a : m_actions) {
        if (a->isVisible())
            list << a.get();
    }
    return list;
}

QList<IKeyAction *> KeyActionRegistry::allActions() const
{
    QList<IKeyAction *> list;
    for (const auto &a : m_actions)
        list << a.get();
    return list;
}

} // namespace ergo
