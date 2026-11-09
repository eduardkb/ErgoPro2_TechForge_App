#pragma once

#include "core/FeatureAvailability.h"

#include <QFrame>

class QLabel;
class QPushButton;

namespace ergo {

class FeatureAvailabilityService;

// Warning banner shown at the top of a function tab while its feature is
// unavailable (B.1.4): what is missing, why, and a button to fix it.
// Hidden automatically when the feature becomes available.
class FeatureBanner : public QFrame {
    Q_OBJECT
public:
    FeatureBanner(Feature feature, FeatureAvailabilityService &service, QWidget *parent = nullptr);

private:
    void update();

    Feature m_feature;
    FeatureAvailabilityService &m_service;
    QLabel *m_title = nullptr;
    QLabel *m_reason = nullptr;
    QPushButton *m_fix = nullptr;
    QString m_fixPermissionId;
};

} // namespace ergo
