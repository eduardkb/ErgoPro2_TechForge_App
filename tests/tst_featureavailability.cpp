#include "core/FeatureAvailability.h"

#include <QTest>

using namespace ergo;

namespace {

PermissionState permission(const QString &id, bool granted, QList<Feature> requiredFor,
                           FixKind fix = FixKind::OpenSettings)
{
    PermissionState p;
    p.id = id;
    p.displayName = id.toUpper();
    p.granted = granted;
    p.howToGrant = QStringLiteral("Grant %1.").arg(id);
    p.fixKind = fix;
    p.fixLabel = QStringLiteral("Fix %1").arg(id);
    p.requiredFor = requiredFor;
    return p;
}

} // namespace

class TestFeatureAvailability : public QObject {
    Q_OBJECT
private slots:
    void availableWhenAllGranted()
    {
        const QList<PermissionState> perms{
            permission(QStringLiteral("a"), true, {Feature::AutoClick, Feature::KeyControl}),
            permission(QStringLiteral("b"), true, allFeatures())};
        for (Feature f : allFeatures()) {
            const FeatureStatus s = evaluateFeature(f, perms);
            QVERIFY(s.available);
            QVERIFY(s.reason.isEmpty());
            QCOMPARE(s.fixKind, FixKind::None);
        }
    }

    void availableWithNoPermissions()
    {
        // e.g. Windows: no requirement at all.
        QVERIFY(evaluateFeature(Feature::AutoClick, {}).available);
        QVERIFY(evaluateFeature(Feature::AutoClick,
                                {permission(QStringLiteral("info"), true, {}, FixKind::None)})
                    .available);
    }

    void missingPermissionOnlyAffectsItsFeatures()
    {
        // macOS: Accessibility is not needed for the Break Timer.
        const QList<PermissionState> perms{
            permission(QStringLiteral("accessibility"), false, {Feature::AutoClick, Feature::KeyControl}),
            permission(QStringLiteral("input-monitoring"), true, allFeatures())};
        QVERIFY(!evaluateFeature(Feature::AutoClick, perms).available);
        QVERIFY(!evaluateFeature(Feature::KeyControl, perms).available);
        QVERIFY(evaluateFeature(Feature::BreakTimerActivity, perms).available);
    }

    void reasonAndFixDescribeTheMissingPermissions()
    {
        const QList<PermissionState> perms{
            permission(QStringLiteral("first"), false, {Feature::AutoClick}, FixKind::None),
            permission(QStringLiteral("second"), false, {Feature::AutoClick}, FixKind::RunFix),
            permission(QStringLiteral("third"), false, {Feature::AutoClick}, FixKind::OpenSettings)};
        const FeatureStatus s = evaluateFeature(Feature::AutoClick, perms);
        QVERIFY(!s.available);
        QCOMPARE(s.missingPermissionIds, QStringList({"first", "second", "third"}));
        QVERIFY(s.reason.contains(QStringLiteral("FIRST, SECOND, THIRD")));
        QVERIFY(s.reason.contains(QStringLiteral("Grant second.")));
        // The first missing permission that HAS a fix provides the button.
        QCOMPARE(s.fixKind, FixKind::RunFix);
        QCOMPARE(s.fixPermissionId, QStringLiteral("second"));
        QCOMPARE(s.fixLabel, QStringLiteral("Fix second"));
    }

    void noFixAvailable()
    {
        const FeatureStatus s = evaluateFeature(
            Feature::BreakTimerActivity,
            {permission(QStringLiteral("session"), false, allFeatures(), FixKind::None)});
        QVERIFY(!s.available);
        QCOMPARE(s.fixKind, FixKind::None);
        QVERIFY(s.fixPermissionId.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestFeatureAvailability)
#include "tst_featureavailability.moc"
