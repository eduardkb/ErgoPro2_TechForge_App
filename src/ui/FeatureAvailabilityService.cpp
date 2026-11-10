#include "ui/FeatureAvailabilityService.h"

#include "core/LogCategories.h"
#include "platform/PlatformInterfaces.h"

namespace ergo {

namespace {

// Compact fingerprint of the permission states to detect changes.
QString fingerprint(const QList<PermissionState> &permissions)
{
    QString result;
    for (const PermissionState &p : permissions)
        result += p.id + (p.granted ? QLatin1String("=1;") : QLatin1String("=0;"));
    return result;
}

} // namespace

FeatureAvailabilityService::FeatureAvailabilityService(IPermissions &permissions, QObject *parent)
    : QObject(parent)
    , m_permissionsApi(permissions)
{
    m_recheckTimer.setInterval(kRecheckIntervalMs);
    connect(&m_recheckTimer, &QTimer::timeout, this, &FeatureAvailabilityService::refresh);
    refresh();
}

void FeatureAvailabilityService::refresh()
{
    const QList<PermissionState> current = m_permissionsApi.check();
    const bool different = !m_initialized || fingerprint(current) != fingerprint(m_permissions);
    m_permissions = current;
    m_statuses.clear();
    const QList<PermissionState> all = permissions();
    for (Feature feature : allFeatures())
        m_statuses.insert(feature, evaluateFeature(feature, all));
    updateTimer();

    if (!different)
        return;
    m_initialized = true;
    for (const PermissionState &p : m_permissions) {
        if (p.granted)
            qCInfo(lcPlatform).noquote() << "Permission" << p.displayName << ": granted";
        else
            qCWarning(lcPlatform).noquote() << "Permission" << p.displayName << ": MISSING";
    }
    for (Feature feature : allFeatures()) {
        const FeatureStatus &s = m_statuses[feature];
        if (!s.available)
            qCWarning(lcPlatform).noquote() << s.reason;
    }
    emit changed();
}

void FeatureAvailabilityService::updateTimer()
{
    if (anyRequiredPermissionMissing()) {
        if (!m_recheckTimer.isActive())
            m_recheckTimer.start();
    } else {
        m_recheckTimer.stop();
    }
}

FeatureStatus FeatureAvailabilityService::status(Feature feature) const
{
    return m_statuses.value(feature);
}

bool FeatureAvailabilityService::anyRequiredPermissionMissing() const
{
    for (const PermissionState &p : permissions()) {
        if (!p.granted && !p.requiredFor.isEmpty())
            return true;
    }
    return false;
}

bool FeatureAvailabilityService::anyOsPermissionMissing() const
{
    for (const PermissionState &p : m_permissions) {
        if (!p.granted && !p.requiredFor.isEmpty())
            return true;
    }
    return false;
}

void FeatureAvailabilityService::setRuntimeRequirement(const PermissionState &requirement)
{
    PermissionState r = requirement;
    r.runtime = true;
    for (PermissionState &existing : m_runtime) {
        if (existing.id == r.id) {
            if (existing.granted == r.granted && existing.explanation == r.explanation)
                return;
            existing = r;
            m_initialized = false; // force changed()
            refresh();
            return;
        }
    }
    m_runtime.append(r);
    m_initialized = false;
    refresh();
}

void FeatureAvailabilityService::clearRuntimeRequirement(const QString &id)
{
    for (int i = 0; i < m_runtime.size(); ++i) {
        if (m_runtime.at(i).id == id) {
            m_runtime.removeAt(i);
            m_initialized = false;
            refresh();
            return;
        }
    }
}

QString FeatureAvailabilityService::sessionDescription() const
{
    return m_permissionsApi.sessionDescription();
}

bool FeatureAvailabilityService::promptAtStartup() const
{
    return m_permissionsApi.promptAtStartup();
}

PlatformResult FeatureAvailabilityService::openSettings(const QString &permissionId)
{
    const PlatformResult result = m_permissionsApi.openSettingsFor(permissionId);
    refresh();
    return result;
}

void FeatureAvailabilityService::runFix(const QString &permissionId, std::function<void(PlatformResult)> done)
{
    m_permissionsApi.runFix(permissionId, [this, done](PlatformResult result) {
        refresh();
        done(result);
    });
}

} // namespace ergo
