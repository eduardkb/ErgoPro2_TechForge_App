#pragma once

#include <QString>

class QWidget;

namespace ergo {

class FeatureAvailabilityService;

// Performs the fix offered for a permission (open settings / run automatic
// fix) with the needed user confirmation and error messages. Shared by the
// feature banners, the diagnostics card and the macOS permissions window.
namespace PermissionActions {

// Returns true if the action was started successfully.
bool performFix(QWidget *parent, FeatureAvailabilityService &service, const QString &permissionId);

} // namespace PermissionActions
} // namespace ergo
