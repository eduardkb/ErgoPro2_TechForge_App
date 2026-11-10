#pragma once

#include <QList>
#include <QString>

#include <memory>

namespace ergo {

// Result of a Key Control operation (core cannot use platform types).
struct OpResult {
    bool ok = true;
    QString message;

    static OpResult success() { return {}; }
    static OpResult failure(const QString &message) { return {false, message}; }
};

// What actions can do. Implemented by the application (click injector,
// AutoClick); faked in unit tests.
class KeyActionContext {
public:
    virtual ~KeyActionContext() = default;
    virtual OpResult leftClick() = 0;
    virtual OpResult rightClick() = 0;
    virtual OpResult doubleLeftClick() = 0;
    // A Key Control click happened: AutoClick treats it like a manual click (B.4 rule 4).
    virtual void notifyKeyControlClick() = 0;
    virtual OpResult toggleAutoClick() = 0;
    // Drag Lock (stage 6, B.5.3): hold the left button / release it again.
    virtual OpResult toggleDragLock() = 0;
    // Any other hotkey press releases an active drag lock first ("any key releases").
    virtual void releaseDragLock() = 0;
};

// One action type (B.5.2, pluggable). Stable ids are stored in settings.json.
class IKeyAction {
public:
    virtual ~IKeyAction() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    // Hidden actions are registered (so stored bindings stay valid) but not offered.
    virtual bool isVisible() const { return true; }
    virtual OpResult execute(KeyActionContext &context) = 0;
};

class KeyActionRegistry {
public:
    // Registers the built-in actions: left-click, right-click, double-left-click,
    // toggle-autoclick and drag-lock (stage 6).
    KeyActionRegistry();

    void add(std::unique_ptr<IKeyAction> action);
    IKeyAction *find(const QString &id) const;
    QList<IKeyAction *> visibleActions() const;
    QList<IKeyAction *> allActions() const;

    static inline const QString LeftClick = QStringLiteral("left-click");
    static inline const QString RightClick = QStringLiteral("right-click");
    static inline const QString DoubleLeftClick = QStringLiteral("double-left-click");
    static inline const QString ToggleAutoClick = QStringLiteral("toggle-autoclick");
    static inline const QString DragLock = QStringLiteral("drag-lock");

private:
    QList<std::shared_ptr<IKeyAction>> m_actions;
};

} // namespace ergo
