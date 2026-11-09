#pragma once

#include <QLabel>

namespace ergo {

// Small pill-shaped status label ("ACTIVE", "INACTIVE", ...). Its colors come
// from the stylesheet via the "badgeState" property.
class StatusBadge : public QLabel {
    Q_OBJECT
public:
    enum class State { Active, Inactive, Warning, Error };

    explicit StatusBadge(QWidget *parent = nullptr);

    void setState(State state, const QString &text);
    State state() const { return m_state; }

private:
    State m_state = State::Inactive;
};

} // namespace ergo
