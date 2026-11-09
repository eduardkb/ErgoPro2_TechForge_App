#include "ui/PermissionsDialog.h"

#include "core/LogCategories.h"
#include "ui/FeatureAvailabilityService.h"
#include "ui/PermissionActions.h"
#include "ui/widgets/CardWidget.h"
#include "ui/widgets/StatusBadge.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace ergo {

PermissionsDialog::PermissionsDialog(FeatureAvailabilityService &service, QWidget *parent)
    : QDialog(parent)
    , m_service(service)
{
    setWindowTitle(tr("ErgoPro2 needs your permission"));
    setModal(true);
    // Fixed width: word-wrapped labels then get their correct height.
    setFixedWidth(600);

    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto *title = new QLabel(tr("Allow ErgoPro2 to help with your mouse and keyboard"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    title->setWordWrap(true);
    layout->addWidget(title);

    auto *intro = new QLabel(tr("macOS protects the mouse and keyboard. ErgoPro2 needs the permissions below for "
                                "Auto Click, Key Control and the Break Timer. Click “Open System Settings”, "
                                "switch on ErgoPro2 in the list, and come back – this window updates by itself."),
                             this);
    intro->setObjectName(QStringLiteral("hint"));
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_rows = new QVBoxLayout;
    m_rows->setSpacing(10);
    layout->addLayout(m_rows);

    m_summary = new QLabel(this);
    m_summary->setWordWrap(true);
    layout->addWidget(m_summary);

    auto *buttons = new QHBoxLayout;
    auto *later = new QPushButton(tr("Continue without"), this);
    auto *recheck = new QPushButton(tr("Check again"), this);
    m_restart = new QPushButton(tr("Restart ErgoPro2"), this);
    m_restart->setProperty("variant", QStringLiteral("primary"));
    m_restart->setVisible(false);
    buttons->addWidget(later);
    buttons->addStretch(1);
    buttons->addWidget(recheck);
    buttons->addWidget(m_restart);
    layout->addLayout(buttons);

    connect(later, &QPushButton::clicked, this, [this] {
        qCInfo(lcUi) << "Permissions window closed without granting all permissions";
        reject();
    });
    connect(recheck, &QPushButton::clicked, &m_service, &FeatureAvailabilityService::refresh);
    connect(m_restart, &QPushButton::clicked, this, &PermissionsDialog::restartRequested);
    connect(&m_service, &FeatureAvailabilityService::changed, this, &PermissionsDialog::rebuild);
    rebuild();
}

void PermissionsDialog::rebuild()
{
    while (QLayoutItem *item = m_rows->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    bool allGranted = true;
    for (const PermissionState &p : m_service.permissions()) {
        if (p.requiredFor.isEmpty())
            continue;
        allGranted = allGranted && p.granted;

        auto *card = new CardWidget(p.displayName, p.explanation, this);
        auto *badge = new StatusBadge(card);
        badge->setState(p.granted ? StatusBadge::State::Active : StatusBadge::State::Warning,
                        p.granted ? tr("GRANTED") : tr("MISSING"));
        card->setHeaderWidget(badge);
        if (!p.granted) {
            auto *row = new QHBoxLayout;
            auto *how = new QLabel(p.howToGrant, card);
            how->setObjectName(QStringLiteral("hint"));
            how->setWordWrap(true);
            row->addWidget(how, 1);
            if (p.fixKind != FixKind::None) {
                auto *open = new QPushButton(p.fixLabel, card);
                open->setProperty("variant", QStringLiteral("primary"));
                open->setCursor(Qt::PointingHandCursor);
                const QString id = p.id;
                connect(open, &QPushButton::clicked, this, [this, id] {
                    if (PermissionActions::performFix(this, m_service, id)) {
                        m_settingsOpened = true;
                        m_restart->setVisible(true);
                    }
                });
                row->addWidget(open);
            }
            card->contentLayout()->addLayout(row);
        }
        m_rows->addWidget(card);
    }

    if (allGranted) {
        m_summary->setText(tr("All permissions are granted. Thank you!"));
        m_restart->setVisible(false);
        if (!m_closing) {
            m_closing = true;
            qCInfo(lcUi) << "All permissions granted; closing the permissions window";
            QTimer::singleShot(1500, this, &QDialog::accept);
        }
    } else {
        m_summary->setText(m_settingsOpened
                               ? tr("Already switched on but still shown as missing? Some macOS versions apply the "
                                    "permission only after a restart – click “Restart ErgoPro2”.")
                               : tr("Without these permissions the affected features stay unavailable. You can "
                                    "grant them later from the General tab or the banner on each tab."));
    }
    layout()->activate();
    resize(width(), layout()->totalHeightForWidth(width()));
}

} // namespace ergo
