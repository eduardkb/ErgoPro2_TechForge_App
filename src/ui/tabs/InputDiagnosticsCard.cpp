#include "ui/tabs/InputDiagnosticsCard.h"

#include "core/LogCategories.h"
#include "ui/InputService.h"
#include "ui/widgets/StatusBadge.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

namespace ergo {

InputDiagnosticsCard::InputDiagnosticsCard(InputService &input, QWidget *parent)
    : CardWidget(tr("Input diagnostics"),
                 tr("Live view of the global mouse/keyboard hook, for testing. Nothing is recorded or stored."),
                 parent)
    , m_input(input)
{
    m_toggle = new QToolButton(this);
    m_toggle->setCheckable(true);
    m_toggle->setCursor(Qt::PointingHandCursor);
    m_toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setHeaderWidget(m_toggle);

    m_body = new QWidget(this);
    auto *bodyLayout = new QVBoxLayout(m_body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(10);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(8);
    grid->setColumnStretch(1, 1);

    auto *hookName = new QLabel(tr("Hook"), m_body);
    hookName->setObjectName(QStringLiteral("valueLabel"));
    hookName->setMinimumWidth(170);
    auto *hookValue = new QWidget(m_body);
    auto *hookRow = new QHBoxLayout(hookValue);
    hookRow->setContentsMargins(0, 0, 0, 0);
    m_hookBadge = new StatusBadge(hookValue);
    m_backend = new QLabel(hookValue);
    m_backend->setObjectName(QStringLiteral("hint"));
    m_backend->setWordWrap(true);
    hookRow->addWidget(m_hookBadge);
    hookRow->addWidget(m_backend, 1);
    grid->addWidget(hookName, 0, 0, Qt::AlignTop);
    grid->addWidget(hookValue, 0, 1);

    m_pointer = addRow(grid, 1, tr("Pointer position"));
    m_lastKey = addRow(grid, 2, tr("Last key event"));
    m_lastButton = addRow(grid, 3, tr("Last mouse button"));
    m_synthetic = addRow(grid, 4, tr("Synthetic (injected)"));
    m_rate = addRow(grid, 5, tr("Events per second"));
    m_watchdog = addRow(grid, 6, tr("Watchdog"));
    bodyLayout->addLayout(grid);

    contentLayout()->addWidget(m_body);

    m_refresh.setInterval(200);
    connect(&m_refresh, &QTimer::timeout, this, &InputDiagnosticsCard::refresh);
    connect(m_toggle, &QToolButton::toggled, this, &InputDiagnosticsCard::setExpanded);
    connect(&m_input, &InputService::statusChanged, this, [this] {
        if (m_body->isVisible())
            refresh();
    });

    setExpanded(false);
}

QLabel *InputDiagnosticsCard::addRow(QGridLayout *grid, int row, const QString &name)
{
    auto *nameLabel = new QLabel(name, m_body);
    nameLabel->setObjectName(QStringLiteral("valueLabel"));
    nameLabel->setMinimumWidth(170);
    auto *value = new QLabel(QStringLiteral("–"), m_body);
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    grid->addWidget(nameLabel, row, 0, Qt::AlignTop);
    grid->addWidget(value, row, 1);
    return value;
}

void InputDiagnosticsCard::setExpanded(bool expanded)
{
    m_toggle->setChecked(expanded);
    m_toggle->setText(expanded ? tr("Hide") : tr("Show"));
    m_toggle->setArrowType(expanded ? Qt::UpArrow : Qt::DownArrow);
    m_body->setVisible(expanded);
    if (expanded) {
        refresh();
        m_refresh.start();
    } else {
        m_refresh.stop();
    }
}

void InputDiagnosticsCard::refresh()
{
    if (!isVisible())
        return;
    const InputService::Stats s = m_input.stats();
    m_hookBadge->setState(s.running ? StatusBadge::State::Active : StatusBadge::State::Error,
                          s.running ? tr("RUNNING") : tr("NOT RUNNING"));
    QString backend = s.backend;
    if (s.running)
        backend += s.canSuppressKeys ? tr(" – can block keys") : tr(" – cannot block keys");
    else if (!s.lastError.isEmpty())
        backend += QStringLiteral(" – ") + s.lastError;
    m_backend->setText(backend);

    if (s.pointerKnown)
        m_pointer->setText(s.screen >= 0 ? tr("%1, %2 (screen %3)")
                                               .arg(qRound(s.pointer.x()))
                                               .arg(qRound(s.pointer.y()))
                                               .arg(s.screen + 1)
                                         : tr("%1, %2").arg(qRound(s.pointer.x())).arg(qRound(s.pointer.y())));
    else
        m_pointer->setText(tr("%1, %2 (relative – the absolute position is not available on Wayland)")
                               .arg(qRound(s.pointer.x()))
                               .arg(qRound(s.pointer.y())));
    m_lastKey->setText(s.lastKey.isEmpty() ? QStringLiteral("–") : s.lastKey);
    m_lastButton->setText(s.lastButton.isEmpty() ? QStringLiteral("–") : s.lastButton);
    m_synthetic->setText(tr("Last event: %1. Last synthetic event: %2")
                             .arg(s.lastEventSynthetic ? tr("yes") : tr("no"),
                                  s.lastSynthetic.isEmpty() ? QStringLiteral("–") : s.lastSynthetic));
    m_rate->setText(tr("%1 (dropped: %2, blocked keys: %3)").arg(s.eventsPerSecond).arg(s.dropped).arg(s.swallowed));
    m_watchdog->setText(tr("Reinstalls: %1 · Self-repairs: %2%3")
                            .arg(s.watchdogReinstalls)
                            .arg(s.selfRepairs)
                            .arg(s.persistentFailure ? tr(" · PERSISTENT FAILURE") : QString()));
}

} // namespace ergo
