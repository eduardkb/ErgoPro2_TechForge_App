#include "ui/tabs/MicroBreaksTab.h"

#include "core/settings/MicroBreakSettings.h"
#include "core/settings/SettingsStore.h"
#include "ui/breaktimer/MicroBreakController.h"
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

#include <algorithm>

namespace ergo {

namespace MB = MicroBreakSettings;

namespace {

QString formatDuration(qint64 ms)
{
    const qint64 s = std::max<qint64>(0, ms) / 1000;
    return QStringLiteral("%1:%2").arg(s / 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0'));
}

QLabel *bigValue(QWidget *parent)
{
    auto *l = new QLabel(QStringLiteral("–"), parent);
    l->setObjectName(QStringLiteral("cardTitle"));
    return l;
}

} // namespace

MicroBreaksTab::MicroBreaksTab(MicroBreakController &controller, SettingsStore &settings, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_settings(settings)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); // small screens (2.5.0)
    outer->addWidget(scroll);
    auto *content = new QWidget(scroll);
    content->setObjectName(QStringLiteral("scrollContent"));
    scroll->setWidget(content);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 12, 0, 20);
    layout->setSpacing(16);

    // --- enable + badge ------------------------------------------------------
    auto *top = new CardWidget(tr("Micro-breaks"),
                               tr("Suggests a very short pause every few minutes of active use: a small window "
                                  "near the tray that never blocks your work. Click it to hide it."),
                               content);
    auto *row = new QHBoxLayout;
    m_enabled = new ToggleSwitch(top);
    auto *enabledLabel = new QLabel(tr("Micro-breaks"), top);
    enabledLabel->setObjectName(QStringLiteral("valueLabel"));
    m_badge = new StatusBadge(top);
    m_badge->setProperty("badgeSize", QStringLiteral("large"));
    row->addWidget(m_enabled);
    row->addWidget(enabledLabel);
    row->addStretch(1);
    row->addWidget(m_badge);
    top->contentLayout()->addLayout(row);
    layout->addWidget(top);

    // --- live status ---------------------------------------------------------
    auto *status = new CardWidget(tr("Status"), {}, content);
    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(24);
    const QStringList names{tr("Active use"), tr("Next micro-break in"), tr("State")};
    m_active = bigValue(status);
    m_until = bigValue(status);
    m_state = bigValue(status);
    const QList<QLabel *> values{m_active, m_until, m_state};
    for (int i = 0; i < values.size(); ++i) {
        auto *name = new QLabel(names[i], status);
        name->setObjectName(QStringLiteral("hint"));
        grid->addWidget(name, 0, i);
        grid->addWidget(values[i], 1, i);
    }
    status->contentLayout()->addLayout(grid);
    layout->addWidget(status);

    // --- parameters ----------------------------------------------------------
    auto *params = new CardWidget(tr("Settings"), tr("Changes apply immediately."), content);
    m_interval = new ParameterRow(tr("Micro-break every"), MB::kIntervalMin, MB::kIntervalMax, tr("min"), params);
    m_interval->setHint(tr("Minutes of active use (mouse or keyboard) until the next micro-break. A pause at least "
                           "as long as a micro-break counts as one and starts the count again."));
    m_show = new ParameterRow(tr("Show it for"), MB::kShowMin, MB::kShowMax, tr("s"), params);
    m_show->setHint(tr("How long the micro-break window stays before it closes by itself."));
    params->contentLayout()->addWidget(m_interval);
    params->contentLayout()->addWidget(m_show);
    auto *note = new QLabel(tr("No micro-break is shown while the Break Timer's reminder or break window is shown; "
                               "after a break the count starts again."),
                            params);
    note->setObjectName(QStringLiteral("hint"));
    note->setWordWrap(true);
    params->contentLayout()->addWidget(note);
    layout->addWidget(params);
    layout->addStretch(1);

    // --- wiring --------------------------------------------------------------
    connect(m_enabled, &ToggleSwitch::toggled, this, [this](bool on) { m_controller.setEnabled(on); });
    connect(m_interval, &ParameterRow::valueChanged, this,
            [this](int v) { m_settings.setValue(MB::IntervalMinutes, v); });
    connect(m_show, &ParameterRow::valueChanged, this, [this](int v) { m_settings.setValue(MB::ShowSeconds, v); });
    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key.startsWith(QLatin1String("microBreaks.")))
            refresh();
    });
    connect(&m_controller, &MicroBreakController::ticked, this, &MicroBreaksTab::refreshStatus);
    connect(&m_controller, &MicroBreakController::availabilityChanged, this, &MicroBreaksTab::refresh);
    refresh();
}

void MicroBreaksTab::refresh()
{
    const bool available = m_controller.isAvailable();
    {
        const QSignalBlocker b(m_enabled);
        m_enabled->setChecked(m_controller.isEnabled());
    }
    m_interval->setValue(m_settings.intValue(MB::IntervalMinutes));
    m_show->setValue(m_settings.intValue(MB::ShowSeconds));
    // Controls of an unavailable feature are disabled (B.1.4); the banner says why.
    for (QWidget *w : std::initializer_list<QWidget *>{m_enabled, m_interval, m_show})
        w->setEnabled(available);
    refreshStatus();
}

void MicroBreaksTab::refreshStatus()
{
    using S = MicroBreakEngine::State;
    const S s = m_controller.state();
    if (s != S::Disabled)
        m_badge->setState(StatusBadge::State::Active, tr("ACTIVE"));
    else if (m_controller.isEnabled() && !m_controller.isAvailable())
        m_badge->setState(StatusBadge::State::Warning, tr("UNAVAILABLE"));
    else
        m_badge->setState(StatusBadge::State::Inactive, tr("INACTIVE"));

    m_active->setText(s == S::Working ? formatDuration(m_controller.activeMs()) : QStringLiteral("–"));
    const qint64 until = m_controller.timeUntilMs();
    QString untilText = until < 0 ? QStringLiteral("–") : formatDuration(until);
    if (s == S::Working && until == 0 && m_controller.isWaitingForBreakTimer())
        untilText = tr("waiting (Break Timer)");
    m_until->setText(untilText);

    QString state;
    switch (s) {
    case S::Disabled: state = tr("Disabled"); break;
    case S::Waiting: state = tr("Idle"); break;
    case S::Working: state = m_controller.isPaused() ? tr("Paused") : tr("Counting"); break;
    case S::Shown: state = tr("Micro-break"); break;
    }
    m_state->setText(state);
}

} // namespace ergo
