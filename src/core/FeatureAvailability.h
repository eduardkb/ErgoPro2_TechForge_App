#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace ergo {

// Features whose availability depends on OS permissions (B.1.4).
enum class Feature { AutoClick, KeyControl, BreakTimerActivity };

QString featureDisplayName(Feature feature);

// What the user (or the app) can do to obtain a missing permission.
enum class FixKind {
    None,         // nothing the app can do (explained in the text)
    OpenSettings, // open the OS settings page where the user grants it
    RunFix        // run an automatic fix (e.g. Linux pkexec command)
};

// State of one OS permission / requirement, reported by the platform layer.
struct PermissionState {
    QString id;          // stable id, e.g. "accessibility", "input-monitoring", "input-devices"
    QString displayName; // e.g. "Accessibility"
    bool granted = false;
    QString explanation; // why ErgoPro2 needs it
    QString howToGrant;  // where/how the user grants it (shown when missing)
    FixKind fixKind = FixKind::None;
    QString fixLabel;    // button text, e.g. "Open System Settings"
    QList<Feature> requiredFor;
    // Runtime requirement reported by the app itself (e.g. "input hook running"),
    // not an OS permission the user grants in a settings page.
    bool runtime = false;
};

struct FeatureStatus {
    Feature feature = Feature::AutoClick;
    bool available = true;
    QString reason;               // empty when available
    QStringList missingPermissionIds;
    FixKind fixKind = FixKind::None;
    QString fixPermissionId;      // permission the fix button acts on
    QString fixLabel;
};

// Pure rule: a feature is available when every permission that lists it in
// requiredFor is granted. The fix offered is the one of the first missing
// permission that has a fix.
FeatureStatus evaluateFeature(Feature feature, const QList<PermissionState> &permissions);

QList<Feature> allFeatures();

} // namespace ergo
