#include "core/FeatureAvailability.h"

#include <QCoreApplication>

namespace ergo {

QString featureDisplayName(Feature feature)
{
    switch (feature) {
    case Feature::AutoClick:
        return QCoreApplication::translate("ergo::FeatureAvailability", "Auto Click");
    case Feature::KeyControl:
        return QCoreApplication::translate("ergo::FeatureAvailability", "Key Control");
    case Feature::BreakTimerActivity:
        return QCoreApplication::translate("ergo::FeatureAvailability", "Break Timer activity monitoring");
    case Feature::Statistics:
        return QCoreApplication::translate("ergo::FeatureAvailability", "Statistics recording");
    }
    return {};
}

QList<Feature> allFeatures()
{
    return {Feature::AutoClick, Feature::KeyControl, Feature::BreakTimerActivity, Feature::Statistics};
}

FeatureStatus evaluateFeature(Feature feature, const QList<PermissionState> &permissions)
{
    FeatureStatus status;
    status.feature = feature;

    QStringList missingNames;
    QStringList howTo;
    for (const PermissionState &p : permissions) {
        if (p.granted || !p.requiredFor.contains(feature))
            continue;
        status.missingPermissionIds << p.id;
        missingNames << p.displayName;
        if (!p.howToGrant.isEmpty())
            howTo << p.howToGrant;
        if (status.fixKind == FixKind::None && p.fixKind != FixKind::None) {
            status.fixKind = p.fixKind;
            status.fixPermissionId = p.id;
            status.fixLabel = p.fixLabel;
        }
    }

    if (missingNames.isEmpty())
        return status;

    status.available = false;
    status.reason = QCoreApplication::translate("ergo::FeatureAvailability",
                                                "%1 is unavailable because this is missing: %2.")
                        .arg(featureDisplayName(feature), missingNames.join(QStringLiteral(", ")));
    if (!howTo.isEmpty())
        status.reason += QLatin1Char(' ') + howTo.join(QLatin1Char(' '));
    return status;
}

} // namespace ergo
