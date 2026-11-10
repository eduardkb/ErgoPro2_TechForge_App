#pragma once

#include "core/FeatureAvailability.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QTimer>

#include <functional>

namespace ergo {

class IPermissions;
struct PlatformResult;

// Central service deciding, for each feature, whether it is available and if
// not, why and how to fix it (B.1.4). Re-checks the OS permissions every 3 s
// while something is missing, so the UI updates without a restart.
class FeatureAvailabilityService : public QObject {
    Q_OBJECT
public:
    static constexpr int kRecheckIntervalMs = 3000;

    explicit FeatureAvailabilityService(IPermissions &permissions, QObject *parent = nullptr);

    // OS permissions followed by runtime requirements.
    QList<PermissionState> permissions() const { return m_permissions + m_runtime; }

    // True if an OS permission (not a runtime requirement) is missing.
    bool anyOsPermissionMissing() const;

    // Adds/replaces a runtime requirement (e.g. the input hook) or removes it.
    void setRuntimeRequirement(const PermissionState &requirement);
    void clearRuntimeRequirement(const QString &id);
    FeatureStatus status(Feature feature) const;
    bool anyRequiredPermissionMissing() const;
    QString sessionDescription() const;
    bool promptAtStartup() const;

    PlatformResult openSettings(const QString &permissionId);
    void runFix(const QString &permissionId, std::function<void(PlatformResult)> done);

public slots:
    // Re-reads the permissions; emits changed() when anything differs.
    void refresh();

signals:
    void changed();

private:
    void updateTimer();

    IPermissions &m_permissionsApi;
    QList<PermissionState> m_permissions;
    QList<PermissionState> m_runtime;
    QHash<Feature, FeatureStatus> m_statuses;
    QTimer m_recheckTimer;
    bool m_initialized = false;
};

} // namespace ergo
