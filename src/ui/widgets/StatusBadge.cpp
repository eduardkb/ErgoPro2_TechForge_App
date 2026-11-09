#include "ui/widgets/StatusBadge.h"

#include <QStyle>

namespace ergo {

namespace {

const char *stateName(StatusBadge::State state)
{
    switch (state) {
    case StatusBadge::State::Active:
        return "active";
    case StatusBadge::State::Inactive:
        return "inactive";
    case StatusBadge::State::Warning:
        return "warning";
    case StatusBadge::State::Error:
        return "error";
    }
    return "inactive";
}

} // namespace

StatusBadge::StatusBadge(QWidget *parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignCenter);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setState(State::Inactive, tr("INACTIVE"));
}

void StatusBadge::setState(State state, const QString &text)
{
    m_state = state;
    setText(text);
    setProperty("badgeState", QString::fromLatin1(stateName(state)));
    // Re-evaluate the property selectors of the stylesheet.
    style()->unpolish(this);
    style()->polish(this);
    adjustSize();
}

} // namespace ergo
