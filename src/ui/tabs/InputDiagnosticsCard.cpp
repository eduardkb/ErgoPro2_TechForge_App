#include "ui/tabs/InputDiagnosticsCard.h"

#include "core/LogCategories.h"
#include "ui/InputService.h"
#include "ui/widgets/StatusBadge.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace ergo {

namespace {
const KeyCombo kTestCombo{Qt::Key_F9, Modifier::None, false};
constexpr int kClickTestDelaySeconds = 3;
constexpr int kSwallowTestSeconds = 30;
}

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

    // --- manual tests -------------------------------------------------------
    auto *tests = new QGridLayout;
    tests->setHorizontalSpacing(12);
    tests->setVerticalSpacing(8);
    tests->setColumnStretch(1, 1);
    m_clickButton = new QPushButton(tr("Test left click in %1 s").arg(kClickTestDelaySeconds), m_body);
    m_clickResult = new QLabel(tr("Move the pointer over something harmless, e.g. an empty desktop area."), m_body);
    m_swallowButton = new QPushButton(tr("Test: block F9 for %1 s").arg(kSwallowTestSeconds), m_body);
    m_swallowResult = new QLabel(tr("While blocked, F9 must not reach any other application."), m_body);
    for (QPushButton *b : {m_clickButton, m_swallowButton})
        b->setCursor(Qt::PointingHandCursor);
    for (QLabel *l : {m_clickResult, m_swallowResult}) {
        l->setObjectName(QStringLiteral("hint"));
        l->setWordWrap(true);
    }
    tests->addWidget(m_clickButton, 0, 0);
    tests->addWidget(m_clickResult, 0, 1);
    tests->addWidget(m_swallowButton, 1, 0);
    tests->addWidget(m_swallowResult, 1, 1);
    bodyLayout->addLayout(tests);

    contentLayout()->addWidget(m_body);

    m_refresh.setInterval(200);
    connect(&m_refresh, &QTimer::timeout, this, &InputDiagnosticsCard::refresh);
    connect(m_toggle, &QToolButton::toggled, this, &InputDiagnosticsCard::setExpanded);
    connect(&m_input, &InputService::statusChanged, this, [this] {
        if (m_body->isVisible())
            refresh();
    });

    m_clickCountdown.setInterval(1000);
    connect(&m_clickCountdown, &QTimer::timeout, this, [this] {
        if (--m_clickSecondsLeft > 0) {
            m_clickResult->setText(tr("Clicking in %1 s…").arg(m_clickSecondsLeft));
            return;
        }
        m_clickCountdown.stop();
        m_clickButton->setEnabled(true);
        m_waitingForSyntheticClick = true;
        m_clickSent.start();
        const PlatformResult r = m_input.injectLeftClick();
        m_clickResult->setText(r.ok() ? tr("Click sent – waiting for the hook to report it as synthetic…")
                                      : tr("Click failed: %1").arg(r.message));
        if (!r.ok())
            m_waitingForSyntheticClick = false;
        QTimer::singleShot(1500, this, [this] {
            if (m_waitingForSyntheticClick) {
                m_waitingForSyntheticClick = false;
                m_clickResult->setText(tr("The click was sent, but the hook did not report it within 1.5 s."));
                qCWarning(lcUi) << "Diagnostics click test: synthetic click not seen by the hook";
            }
        });
    });
    connect(&m_input, &InputService::syntheticClickSeen, this, [this](const InputEvent &e) {
        if (!m_waitingForSyntheticClick)
            return;
        m_waitingForSyntheticClick = false;
        m_clickResult->setText(tr("✅ Click at %1, %2 received back from the hook as synthetic after %3 ms.")
                                   .arg(qRound(e.position.x()))
                                   .arg(qRound(e.position.y()))
                                   .arg(m_clickSent.elapsed()));
        qCInfo(lcUi) << "Diagnostics click test passed";
    });
    connect(m_clickButton, &QPushButton::clicked, this, &InputDiagnosticsCard::startClickTest);

    m_swallowCountdown.setInterval(1000);
    connect(&m_swallowCountdown, &QTimer::timeout, this, [this] {
        const quint64 blocked = m_input.stats().swallowed - m_swallowedAtStart;
        if (--m_swallowSecondsLeft > 0) {
            m_swallowResult->setText(tr("F9 is blocked for %1 s – press F9 in another application. Blocked key "
                                        "events so far: %2.")
                                         .arg(m_swallowSecondsLeft)
                                         .arg(blocked));
            return;
        }
        m_swallowCountdown.stop();
        m_input.unregisterKey(kTestCombo);
        m_swallowButton->setEnabled(true);
        m_swallowResult->setText(tr("Test finished: %1 F9 key event(s) were blocked. F9 works normally again.")
                                     .arg(blocked));
    });
    connect(m_swallowButton, &QPushButton::clicked, this, &InputDiagnosticsCard::startSwallowTest);

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

void InputDiagnosticsCard::startClickTest()
{
    m_clickButton->setEnabled(false);
    m_clickSecondsLeft = kClickTestDelaySeconds;
    m_clickResult->setText(tr("Clicking in %1 s…").arg(m_clickSecondsLeft));
    m_clickCountdown.start();
}

void InputDiagnosticsCard::startSwallowTest()
{
    const PlatformResult r = m_input.registerKey(kTestCombo);
    if (!r.ok()) {
        m_swallowResult->setText(tr("Cannot block F9: %1").arg(r.message));
        return;
    }
    m_swallowButton->setEnabled(false);
    m_swallowedAtStart = m_input.stats().swallowed;
    m_swallowSecondsLeft = kSwallowTestSeconds;
    m_swallowResult->setText(tr("F9 is blocked for %1 s – press F9 in another application.").arg(m_swallowSecondsLeft));
    m_swallowCountdown.start();
}

} // namespace ergo
