#include "ui/tabs/BreakTimerTab.h"

#include "core/settings/BreakTimerSettings.h"
#include "core/settings/SettingsStore.h"
#include "ui/breaktimer/BreakTimerController.h"
#include "ui/widgets/CardWidget.h"
#include "ui/widgets/ParameterRow.h"
#include "ui/widgets/StatusBadge.h"
#include "ui/widgets/ToggleSwitch.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace ergo {

namespace BT = BreakTimerSettings;

namespace {

QString formatDuration(qint64 ms)
{
    const qint64 s = std::max<qint64>(0, ms) / 1000;
    if (s >= 3600)
        return QStringLiteral("%1:%2:%3")
            .arg(s / 3600)
            .arg((s / 60) % 60, 2, 10, QLatin1Char('0'))
            .arg(s % 60, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(s / 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0'));
}

QLabel *bigValue(QWidget *parent)
{
    auto *l = new QLabel(QStringLiteral("–"), parent);
    l->setObjectName(QStringLiteral("cardTitle"));
    return l;
}

} // namespace

BreakTimerTab::BreakTimerTab(BreakTimerController &controller, SettingsStore &settings, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_settings(settings)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    outer->addWidget(scroll);
    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("scrollContent"));
    scroll->setWidget(content);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 12, 0, 20);
    layout->setSpacing(16);

    // --- enable + badge ------------------------------------------------------
    auto *top = new CardWidget(tr("Break Timer"),
                               tr("Reminds you to rest after a period of continuous work. Short pauses keep "
                                  "counting; a longer pause resets the timer."),
                               content);
    auto *row = new QHBoxLayout;
    m_enabled = new ToggleSwitch(top);
    auto *enabledLabel = new QLabel(tr("Break reminders"), top);
    enabledLabel->setObjectName(QStringLiteral("valueLabel"));
    m_badge = new StatusBadge(top);
    m_badge->setProperty("badgeSize", QStringLiteral("large"));
    row->addWidget(m_enabled);
    row->addWidget(enabledLabel);
    row->addStretch(1);
    row->addWidget(m_badge);
    top->contentLayout()->addLayout(row);
    layout->addWidget(top);

    // --- parameters ----------------------------------------------------------
    auto *params = new CardWidget(tr("Settings"), tr("Changes apply immediately, also to the running work period."),
                                  content);
    m_work = new ParameterRow(tr("Break after working"), BT::kMaxWorkMin, BT::kMaxWorkMax, tr("min"), params);
    m_work->setHint(tr("BREAKTIMER_MAX_WORK_TIME – active work time until the break reminder appears."));
    m_idle = new ParameterRow(tr("Reset after a pause of"), BT::kIdleResetMin, BT::kIdleResetMax, tr("min"), params);
    m_idle->setHint(tr("BREAKTIMER_IDLE_RESET – no mouse or keyboard use for this long (also sleep or a locked "
                       "screen) counts as a break and resets the work time."));
    m_break = new ParameterRow(tr("Break length"), BT::kBreakMin, BT::kBreakMax, tr("min"), params);
    m_break->setHint(tr("BREAKTIMER_BREAK_TIME – how long the break window stays before it closes by itself."));
    m_postpone = new ParameterRow(tr("Postpone by"), BT::kPostponeMin, BT::kPostponeMax, tr("min"), params);
    m_postpone->setHint(tr("When you click Postpone, the break window returns after this time."));
    for (ParameterRow *r : {m_work, m_idle, m_break, m_postpone})
        params->contentLayout()->addWidget(r);

    auto *fsRow = new QHBoxLayout;
    auto *fsName = new QLabel(tr("No breaks during full-screen apps"), params);
    fsName->setObjectName(QStringLiteral("valueLabel"));
    m_fullscreen = new ToggleSwitch(params);
    fsRow->addWidget(fsName);
    fsRow->addWidget(m_fullscreen);
    fsRow->addStretch(1);
    params->contentLayout()->addLayout(fsRow);
    m_fullscreenHint = new QLabel(params);
    m_fullscreenHint->setObjectName(QStringLiteral("hint"));
    m_fullscreenHint->setWordWrap(true);
    params->contentLayout()->addWidget(m_fullscreenHint);
    layout->addWidget(params);

    // --- live status ---------------------------------------------------------
    auto *status = new CardWidget(tr("Status"), {}, content);
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(24);
    const QStringList names{tr("Active work time"), tr("Next break in"), tr("State")};
    m_workTime = bigValue(status);
    m_untilBreak = bigValue(status);
    m_state = bigValue(status);
    const QList<QLabel *> values{m_workTime, m_untilBreak, m_state};
    for (int i = 0; i < 3; ++i) {
        auto *name = new QLabel(names[i], status);
        name->setObjectName(QStringLiteral("hint"));
        grid->addWidget(name, 0, i);
        grid->addWidget(values[i], 1, i);
    }
    status->contentLayout()->addLayout(grid);
    layout->addWidget(status);
    layout->addStretch(1);

    // --- wiring --------------------------------------------------------------
    connect(m_enabled, &ToggleSwitch::toggled, this, [this](bool on) { m_controller.setEnabled(on); });
    connect(m_work, &ParameterRow::valueChanged, this, [this](int v) { m_settings.setValue(BT::MaxWorkMin, v); });
    connect(m_idle, &ParameterRow::valueChanged, this, [this](int v) { m_settings.setValue(BT::IdleResetMin, v); });
    connect(m_break, &ParameterRow::valueChanged, this, [this](int v) { m_settings.setValue(BT::BreakMin, v); });
    connect(m_postpone, &ParameterRow::valueChanged, this, [this](int v) { m_settings.setValue(BT::PostponeMin, v); });
    connect(m_fullscreen, &ToggleSwitch::toggled, this,
            [this](bool on) { m_settings.setValue(BT::SuppressWhenFullscreen, on); });
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key.startsWith(QLatin1String("breaktimer.")))
            refresh();
    });
    connect(&m_controller, &BreakTimerController::ticked, this, &BreakTimerTab::refreshStatus);
    refresh();
}

void BreakTimerTab::refresh()
{
    const bool available = m_controller.isAvailable();
    {
        const QSignalBlocker b1(m_enabled), b2(m_fullscreen);
        m_enabled->setChecked(m_controller.isEnabled());
        m_fullscreen->setChecked(m_settings.boolValue(BT::SuppressWhenFullscreen));
    }
    m_work->setValue(m_settings.intValue(BT::MaxWorkMin));
    m_idle->setValue(m_settings.intValue(BT::IdleResetMin));
    m_break->setValue(m_settings.intValue(BT::BreakMin));
    m_postpone->setValue(m_settings.intValue(BT::PostponeMin));

    // Controls of an unavailable feature are disabled (B.1.4); the banner says why.
    for (QWidget *w : std::initializer_list<QWidget *>{m_enabled, m_work, m_idle, m_break, m_postpone})
        w->setEnabled(available);
    const bool fsSupported = m_controller.fullscreenDetectionSupported();
    m_fullscreen->setEnabled(available && fsSupported);
    m_fullscreenHint->setText(
        fsSupported ? tr("While a full-screen application (video, presentation, game) is in front, the break waits "
                         "until it is no longer full-screen.")
                    : tr("Not available on this system: ErgoPro2 cannot detect full-screen applications here."));
    refreshStatus();
}

void BreakTimerTab::refreshStatus()
{
    using S = BreakTimerEngine::State;
    const S s = m_controller.state();
    const bool running = s != S::Disabled;
    if (running)
        m_badge->setState(StatusBadge::State::Active, tr("ACTIVE"));
    else if (m_controller.isEnabled() && !m_controller.isAvailable())
        m_badge->setState(StatusBadge::State::Warning, tr("UNAVAILABLE"));
    else
        m_badge->setState(StatusBadge::State::Inactive, tr("INACTIVE"));

    m_workTime->setText(running ? formatDuration(m_controller.interactionMs()) : QStringLiteral("–"));
    const qint64 until = m_controller.timeUntilBreakMs();
    QString untilText = until < 0 ? QStringLiteral("–") : formatDuration(until);
    if (m_controller.isBreakDelayedByFullscreen())
        untilText = tr("waiting (full-screen app)");
    m_untilBreak->setText(untilText);

    QString state;
    switch (s) {
    case S::Disabled: state = tr("Disabled"); break;
    case S::WaitingForActivity: state = tr("Idle"); break;
    case S::Working: state = tr("Working"); break;
    case S::BreakShown: state = tr("On break"); break;
    case S::Postponed: state = tr("Postponed"); break;
    }
    m_state->setText(state);
}

} // namespace ergo
