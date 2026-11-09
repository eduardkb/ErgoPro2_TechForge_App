#include "ui/widgets/FeaturePage.h"

#include "ui/widgets/FeatureBanner.h"

#include <QVBoxLayout>

namespace ergo {

FeaturePage::FeaturePage(Feature feature, FeatureAvailabilityService &service, QWidget *content, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 12, 20, 0);
    layout->setSpacing(0);
    layout->addWidget(new FeatureBanner(feature, service, this));
    content->setParent(this);
    layout->addWidget(content, 1);
}

} // namespace ergo
