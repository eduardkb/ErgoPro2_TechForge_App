#include "ui/tabs/DiagnosticsCard.h"

#include "platform/PlatformFactory.h"
#include "ui/AppContext.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/InputService.h"
#include "ui/InstallController.h"
#include "ui/PermissionActions.h"
#include "ui/TextUtils.h"
#include "ui/widgets/StatusBadge.h"

#include <QCoreApplication>
#include <QDir>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSysInfo>
#include <QVBoxLayout>

namespace ergo {

namespace {

QLabel *valueLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

} // namespace

DiagnosticsCard::DiagnosticsCard(AppContext &context, QWidget *parent)
    : CardWidget(tr("Permissions & diagnostics"),
                 tr("What ErgoPro2 needs from the operating system, and whether it has it."), parent)
    , m_context(context)
{
    auto *recheck = new QPushButton(tr("Check again"), this);
    recheck->setCursor(Qt::PointingHandCursor);
    setHeaderWidget(recheck);
    connect(recheck, &QPushButton::clicked, &m_context.availability, &FeatureAvailabilityService::refresh);

    connect(&m_context.availability, &FeatureAvailabilityService::changed, this, &DiagnosticsCard::refresh);
    connect(&m_context.install, &InstallController::changed, this, &DiagnosticsCard::refresh);
    connect(&m_context.input, &InputService::statusChanged, this, &DiagnosticsCard::refresh);
    refresh();
}

void DiagnosticsCard::addRow(int row, const QString &name, QWidget *value, QWidget *action)
{
    auto *nameLabel = new QLabel(name, m_body);
    nameLabel->setObjectName(QStringLiteral("valueLabel"));
    nameLabel->setMinimumWidth(170);
    m_grid->addWidget(nameLabel, row, 0, Qt::AlignTop);
    m_grid->addWidget(value, row, 1);
    if (action)
        m_grid->addWidget(action, row, 2, Qt::AlignTop | Qt::AlignRight);
}

void DiagnosticsCard::refresh()
{
    delete m_body; // rebuilt completely: the number of rows depends on the OS state
    m_body = new QWidget(this);
    m_grid = new QGridLayout(m_body);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setHorizontalSpacing(12);
    m_grid->setVerticalSpacing(10);
    m_grid->setColumnStretch(1, 1);
    contentLayout()->addWidget(m_body);

    int row = 0;
    addRow(row++, tr("Operating system"),
           valueLabel(QStringLiteral("%1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture()),
                      m_body));
    addRow(row++, tr("Desktop session"), valueLabel(m_context.availability.sessionDescription(), m_body));
    addRow(row++, tr("Application"),
           valueLabel(wrappablePath(QCoreApplication::applicationFilePath()), m_body));

    for (const PermissionState &p : m_context.availability.permissions()) {
        auto *value = new QWidget(m_body);
        auto *v = new QVBoxLayout(value);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(4);
        auto *badge = new StatusBadge(value);
        if (p.requiredFor.isEmpty())
            badge->setState(StatusBadge::State::Active, tr("OK"));
        else
            badge->setState(p.granted ? StatusBadge::State::Active : StatusBadge::State::Warning,
                            p.granted ? tr("GRANTED") : tr("MISSING"));
        v->addWidget(badge, 0, Qt::AlignLeft);
        auto *explanation = valueLabel(p.granted || p.howToGrant.isEmpty() ? p.explanation
                                                                           : p.explanation + QLatin1Char(' ') + p.howToGrant,
                                       value);
        explanation->setObjectName(QStringLiteral("hint"));
        v->addWidget(explanation);

        QPushButton *fix = nullptr;
        if (!p.granted && p.fixKind != FixKind::None) {
            fix = new QPushButton(p.fixLabel, m_body);
            fix->setProperty("variant", QStringLiteral("primary"));
            fix->setCursor(Qt::PointingHandCursor);
            const QString id = p.id;
            connect(fix, &QPushButton::clicked, this,
                    [this, id] { PermissionActions::performFix(window(), m_context.availability, id); });
        }
        addRow(row++, p.displayName, value, fix);
    }

    auto *hook = new StatusBadge(m_body);
    const InputService::Stats input = m_context.input.stats();
    hook->setState(input.running ? StatusBadge::State::Active : StatusBadge::State::Error,
                   input.running ? tr("RUNNING") : tr("NOT RUNNING"));
    auto *hookValue = new QWidget(m_body);
    auto *h = new QHBoxLayout(hookValue);
    h->setContentsMargins(0, 0, 0, 0);
    h->addWidget(hook);
    QString detail = input.backend;
    if (!input.running)
        detail = input.lastError.isEmpty() ? tr("Waiting for the permissions above.") : input.lastError;
    else if (!input.canSuppressKeys)
        detail += tr(" – cannot block keys");
    auto *why = new QLabel(detail, hookValue);
    why->setObjectName(QStringLiteral("hint"));
    why->setWordWrap(true);
    h->addWidget(why, 1);
    addRow(row++, tr("Input hook"), hookValue);

    const InstallController::Status install = m_context.install.status();
    addRow(row++, tr("Start at login"),
           valueLabel(install.startsAtLogin ? tr("Registered: %1").arg(wrappablePath(install.autostartLocation))
                                            : tr("Not registered"),
                      m_body));
}

} // namespace ergo
