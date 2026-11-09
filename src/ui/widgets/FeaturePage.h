#pragma once

#include "core/FeatureAvailability.h"

#include <QWidget>

namespace ergo {

class FeatureAvailabilityService;

// Function tab: availability banner on top, the feature's content below.
// Later stages pass their real content widget instead of a placeholder.
class FeaturePage : public QWidget {
    Q_OBJECT
public:
    FeaturePage(Feature feature, FeatureAvailabilityService &service, QWidget *content, QWidget *parent = nullptr);
};

} // namespace ergo
