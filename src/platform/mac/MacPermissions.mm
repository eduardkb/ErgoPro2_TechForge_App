#include "platform/mac/MacPermissions.h"

#include "core/LogCategories.h"

#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/hidsystem/IOHIDLib.h> // IOHIDCheckAccess / IOHIDRequestAccess

#include <QCoreApplication>

namespace ergo {

namespace {

const QString kAccessibility = QStringLiteral("accessibility");
const QString kInputMonitoring = QStringLiteral("input-monitoring");

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::MacPermissions", text);
}

bool accessibilityGranted()
{
    return AXIsProcessTrusted();
}

bool inputMonitoringGranted()
{
    return IOHIDCheckAccess(kIOHIDRequestTypeListenEvent) == kIOHIDAccessTypeGranted;
}

bool openUrl(const char *url)
{
    NSURL *nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
    return nsUrl && [[NSWorkspace sharedWorkspace] openURL:nsUrl];
}

} // namespace

QList<PermissionState> MacPermissions::check() const
{
    PermissionState accessibility;
    accessibility.id = kAccessibility;
    accessibility.displayName = tr("Accessibility");
    accessibility.granted = accessibilityGranted();
    accessibility.explanation = tr("Lets ErgoPro2 click the mouse for you (Auto Click, Key Control) and keep "
                                   "the keys you assign to Key Control for itself.");
    accessibility.howToGrant = tr("Turn on ErgoPro2 in System Settings › Privacy & Security › Accessibility.");
    accessibility.fixKind = FixKind::OpenSettings;
    accessibility.fixLabel = tr("Open System Settings");
    accessibility.requiredFor = {Feature::AutoClick, Feature::KeyControl};

    PermissionState inputMonitoring;
    inputMonitoring.id = kInputMonitoring;
    inputMonitoring.displayName = tr("Input Monitoring");
    inputMonitoring.granted = inputMonitoringGranted();
    inputMonitoring.explanation = tr("Lets ErgoPro2 notice mouse movement and key presses in all applications "
                                     "(Auto Click, Key Control hotkeys and the Break Timer's activity tracking). "
                                     "ErgoPro2 never records or stores what you type.");
    inputMonitoring.howToGrant =
        tr("Turn on ErgoPro2 in System Settings › Privacy & Security › Input Monitoring.");
    inputMonitoring.fixKind = FixKind::OpenSettings;
    inputMonitoring.fixLabel = tr("Open System Settings");
    inputMonitoring.requiredFor = {Feature::AutoClick, Feature::KeyControl, Feature::BreakTimerActivity};

    return {accessibility, inputMonitoring};
}

QString MacPermissions::sessionDescription() const
{
    return QStringLiteral("macOS desktop (Aqua)");
}

PlatformResult MacPermissions::openSettingsFor(const QString &permissionId)
{
    const char *url = nullptr;
    if (permissionId == kAccessibility) {
        // With the prompt option macOS adds ErgoPro2 to the Accessibility list
        // (switched off) and shows its own hint, so the user only flips the switch.
        NSDictionary *options = @{(__bridge NSString *)kAXTrustedCheckOptionPrompt : @YES};
        AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
        url = "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility";
    } else if (permissionId == kInputMonitoring) {
        // Adds ErgoPro2 to the Input Monitoring list (first call shows a system prompt).
        IOHIDRequestAccess(kIOHIDRequestTypeListenEvent);
        url = "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent";
    } else {
        qCCritical(lcPlatform) << "openSettingsFor: unknown permission" << permissionId;
        return PlatformResult::failed(tr("Unknown permission."));
    }

    if (!openUrl(url)) {
        const QString message = tr("System Settings could not be opened. Open it manually: System Settings › "
                                   "Privacy & Security.");
        qCCritical(lcPlatform).noquote() << message << url;
        return PlatformResult::failed(message);
    }
    qCInfo(lcPlatform) << "Opened System Settings for permission" << permissionId;
    return PlatformResult::success();
}

void MacPermissions::runFix(const QString &permissionId, std::function<void(PlatformResult)> done)
{
    // macOS permissions are granted in System Settings only.
    done(openSettingsFor(permissionId));
}

} // namespace ergo
