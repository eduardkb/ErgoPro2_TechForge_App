#include "ui/widgets/FeatureBanner.h"

#include "ui/FeatureAvailabilityService.h"
#include "ui/PermissionActions.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace ergo {

FeatureBanner::FeatureBanner(Feature feature, FeatureAvailabilityService &service, QWidget *parent)
    : QFrame(parent)
    , m_feature(feature)
    , m_service(service)
{
    setObjectName(QStringLiteral("banner"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(14);

    auto *icon = new QLabel(QStringLiteral("⚠"), this);
    icon->setObjectName(QStringLiteral("bannerIcon"));
    layout->addWidget(icon, 0, Qt::AlignTop);

    auto *texts = new QVBoxLayout;
    texts->setSpacing(4);
    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("bannerTitle"));
    m_reason = new QLabel(this);
    m_reason->setObjectName(QStringLiteral("bannerText"));
    m_reason->setWordWrap(true);
    texts->addWidget(m_title);
    texts->addWidget(m_reason);
    layout->addLayout(texts, 1);

    auto *buttons = new QVBoxLayout;
    buttons->setSpacing(6);
    m_fix = new QPushButton(this);
    m_fix->setProperty("variant", QStringLiteral("primary"));
    m_fix->setCursor(Qt::PointingHandCursor);
    auto *recheck = new QPushButton(tr("Check again"), this);
    recheck->setCursor(Qt::PointingHandCursor);
    buttons->addWidget(m_fix);
    buttons->addWidget(recheck);
    buttons->addStretch(1);
    layout->addLayout(buttons);

    connect(m_fix, &QPushButton::clicked, this,
            [this] { PermissionActions::performFix(window(), m_service, m_fixPermissionId); });
    connect(recheck, &QPushButton::clicked, &m_service, &FeatureAvailabilityService::refresh);
    connect(&m_service, &FeatureAvailabilityService::changed, this, &FeatureBanner::update);
    update();
}

void FeatureBanner::update()
{
    const FeatureStatus s = m_service.status(m_feature);
    setVisible(!s.available);
    if (s.available)
        return;
    m_title->setText(tr("%1 is unavailable").arg(featureDisplayName(m_feature)));
    m_reason->setText(s.reason);
    m_fixPermissionId = s.fixPermissionId;
    m_fix->setText(s.fixLabel);
    m_fix->setVisible(s.fixKind != FixKind::None);
}

} // namespace ergo
