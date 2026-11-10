#include "ui/tabs/AutoClickTab.h"

#include "core/autoclick/AutoClickEngine.h"
#include "core/settings/AutoClickSettings.h"
#include "core/settings/SettingsStore.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/autoclick/AutoClickController.h"
#include "ui/widgets/CardWidget.h"
#include "ui/widgets/ParameterRow.h"
#include "ui/widgets/StatusBadge.h"
#include "ui/widgets/ToggleSwitch.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStyle>
#include <QVBoxLayout>

namespace ergo {

namespace AC = AutoClickSettings;

AutoClickTab::AutoClickTab(AutoClickController &controller, SettingsStore &settings,
                           FeatureAvailabilityService &availability, QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_settings(settings)
    , m_availability(availability)
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

    // --- status ------------------------------------------------------------
    auto *statusCard = new CardWidget(tr("Auto Click"),
                                      tr("Clicks the left mouse button for you when the pointer stops moving."),
                                      content);
    auto *row = new QHBoxLayout;
    m_badge = new StatusBadge(statusCard);
    m_badge->setProperty("badgeSize", QStringLiteral("large"));
    m_toggle = new QPushButton(statusCard);
    m_toggle->setCursor(Qt::PointingHandCursor);
    m_toggle->setMinimumWidth(130);
    row->addWidget(m_badge);
    row->addStretch(1);
    row->addWidget(m_toggle);
    statusCard->contentLayout()->addLayout(row);
    m_stateLine = new QLabel(statusCard);
    m_stateLine->setObjectName(QStringLiteral("hint"));
    m_stateLine->setWordWrap(true);
    statusCard->contentLayout()->addWidget(m_stateLine);
    m_lastError = new QLabel(statusCard);
    m_lastError->setObjectName(QStringLiteral("warningText"));
    m_lastError->setWordWrap(true);
    m_lastError->hide();
    statusCard->contentLayout()->addWidget(m_lastError);
    layout->addWidget(statusCard);

    // --- parameters ----------------------------------------------------------
    auto *params = new CardWidget(tr("Settings"), tr("Changes apply immediately."), content);
    m_timer = new ParameterRow(tr("Wait before clicking"), AC::kTriggerTimerMin, AC::kTriggerTimerMax,
                               tr("ms"), params);
    m_timer->setHint(tr("AUTOCLICK_TRIGGER_TIMER – how long the pointer must rest before the click."));
    m_distance = new ParameterRow(tr("Minimum movement"), AC::kDistanceMin, AC::kDistanceMax, tr("px"), params);
    m_distance->setHint(tr("AUTOCLICK_TRIGGER_DISTANCE – how far the pointer must move away from the last click "
                           "before the next auto-click. Resting the mouse never produces repeated clicks."));
    m_jitter = new ParameterRow(tr("Ignore tremor below"), AC::kJitterMin, AC::kJitterMax, tr("px"), params);
    m_jitter->setHint(tr("Small hand tremor up to this distance does not restart the countdown."));
    for (ParameterRow *r : {m_timer, m_distance, m_jitter})
        params->contentLayout()->addWidget(r);

    auto *feedbackRow = new QHBoxLayout;
    auto *feedbackName = new QLabel(tr("Show countdown cross"), params);
    feedbackName->setObjectName(QStringLiteral("valueLabel"));
    feedbackName->setMinimumWidth(170);
    m_feedback = new ToggleSwitch(params);
    feedbackRow->addWidget(feedbackName);
    feedbackRow->addWidget(m_feedback);
    feedbackRow->addStretch(1);
    params->contentLayout()->addLayout(feedbackRow);
    m_feedbackHint = new QLabel(params);
    m_feedbackHint->setObjectName(QStringLiteral("hint"));
    m_feedbackHint->setWordWrap(true);
    params->contentLayout()->addWidget(m_feedbackHint);

    auto *note = new QLabel(tr("Auto Click also works on the ErgoPro2 window. No click happens while a mouse button "
                               "is held or within 0.5 s after typing; a manual click cancels a pending auto-click."),
                            params);
    note->setObjectName(QStringLiteral("hint"));
    note->setWordWrap(true);
    params->contentLayout()->addWidget(note);
    layout->addWidget(params);
    layout->addStretch(1);

    // --- wiring --------------------------------------------------------------
    connect(m_toggle, &QPushButton::clicked, this, [this] { m_controller.toggle(); });
    connect(m_timer, &ParameterRow::valueChanged, this,
            [this](int v) { m_settings.setValue(AC::TriggerTimerMs, v); });
    connect(m_distance, &ParameterRow::valueChanged, this,
            [this](int v) { m_settings.setValue(AC::TriggerDistancePx, v); });
    connect(m_jitter, &ParameterRow::valueChanged, this,
            [this](int v) { m_settings.setValue(AC::JitterTolerancePx, v); });
    connect(m_feedback, &ToggleSwitch::toggled, this,
            [this](bool on) { m_settings.setValue(AC::ShowVisualFeedback, on); });

    connect(&m_settings, &SettingsStore::valueChanged, this, [this](const QString &key) {
        if (key.startsWith(QLatin1String("autoclick.")))
            refresh();
    });
    connect(&m_controller, &AutoClickController::activeChanged, this, &AutoClickTab::refresh);
    connect(&m_controller, &AutoClickController::stateChanged, this, &AutoClickTab::showState);
    connect(&m_controller, &AutoClickController::clickFailed, this, [this](const QString &message) {
        m_lastError->setText(tr("Last click failed: %1").arg(message));
        m_lastError->show();
    });
    connect(&m_availability, &FeatureAvailabilityService::changed, this, &AutoClickTab::refresh);
    refresh();
}

void AutoClickTab::refresh()
{
    const bool available = m_controller.isAvailable();
    const bool enabled = m_controller.isEnabled();
    const bool active = m_controller.isActive();

    if (active)
        m_badge->setState(StatusBadge::State::Active, tr("AutoClick is ACTIVE"));
    else if (enabled && available)
        m_badge->setState(StatusBadge::State::Warning, tr("AutoClick is STARTING…"));
    else
        m_badge->setState(StatusBadge::State::Inactive, tr("AutoClick is INACTIVE"));

    m_toggle->setText(enabled ? tr("Deactivate") : tr("Activate"));
    m_toggle->setProperty("variant", enabled ? QStringLiteral("danger") : QStringLiteral("primary"));
    m_toggle->style()->unpolish(m_toggle);
    m_toggle->style()->polish(m_toggle);

    // Controls of an unavailable feature are disabled (B.1.4); the banner says why.
    for (QWidget *w : std::initializer_list<QWidget *>{m_toggle, m_timer, m_distance, m_jitter})
        w->setEnabled(available);
    m_feedback->setEnabled(available && m_controller.visualFeedbackSupported());

    m_timer->setValue(m_settings.intValue(AC::TriggerTimerMs));
    m_distance->setValue(m_settings.intValue(AC::TriggerDistancePx));
    m_jitter->setValue(m_settings.intValue(AC::JitterTolerancePx));
    {
        const QSignalBlocker block(m_feedback);
        m_feedback->setChecked(m_settings.boolValue(AC::ShowVisualFeedback));
    }
    m_feedbackHint->setText(m_controller.visualFeedbackSupported()
                                ? tr("An orange X grows from the pointer tip until the click happens.")
                                : m_controller.visualFeedbackUnsupportedReason());
    if (!active)
        m_lastError->hide();
    showState(m_controller.lastState(), m_controller.lastSuppressReason());
}

void AutoClickTab::showState(int state, int reason)
{
    if (!m_controller.isActive()) {
        m_stateLine->setText(m_controller.isAvailable() ? tr("Click Activate to start.")
                                                        : tr("Not available – see the message above."));
        return;
    }
    using S = AutoClickEngine::State;
    using R = AutoClickEngine::SuppressReason;
    QString text;
    switch (S(state)) {
    case S::Disabled:
    case S::WaitingForMovement:
        text = tr("Waiting: move the pointer to where you want to click.");
        break;
    case S::Armed:
        text = tr("Armed: stop moving to click.");
        break;
    case S::Countdown:
        text = tr("Clicking when the pointer rests…");
        break;
    case S::Suppressed:
        switch (R(reason)) {
        case R::ButtonHeld: text = tr("Paused: a mouse button is held."); break;
        case R::Typing: text = tr("Paused: you are typing."); break;
        case R::DragLock: text = tr("Paused: Drag Lock is active."); break;
        case R::None: text = tr("Paused."); break;
        }
        break;
    }
    m_stateLine->setText(text);
}

} // namespace ergo
