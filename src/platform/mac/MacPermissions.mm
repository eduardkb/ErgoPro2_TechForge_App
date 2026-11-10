#include "platform/mac/MacPermissions.h"

#include "core/LogCategories.h"
#include "ergopro2/Version.h"

#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/hidsystem/IOHIDLib.h> // IOHIDCheckAccess / IOHIDRequestAccess
#include <Security/Security.h>

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

// macOS stores permission grants against the app's code signature. If the
// running bundle's signature is invalid (e.g. only the linker's partial
// signature) or uses another identifier than the bundle id, the Accessibility
// switch in System Settings never matches this process and AXIsProcessTrusted()
// stays false. Returns a description of such a problem, or an empty string.
QString signatureProblem()
{
    SecCodeRef code = nullptr;
    if (SecCodeCopySelf(kSecCSDefaultFlags, &code) != errSecSuccess || !code)
        return tr("its code signature cannot be read");
    SecStaticCodeRef staticCode = nullptr;
    OSStatus status = SecCodeCopyStaticCode(code, kSecCSDefaultFlags, &staticCode);
    CFRelease(code);
    if (status != errSecSuccess || !staticCode)
        return tr("its code signature cannot be read (error %1)").arg(status);

    QString problem;
    status = SecStaticCodeCheckValidity(staticCode, kSecCSCheckAllArchitectures, nullptr);
    if (status != errSecSuccess) {
        problem = tr("its code signature is invalid (error %1)").arg(status);
    } else {
        CFDictionaryRef info = nullptr;
        if (SecCodeCopySigningInformation(staticCode, kSecCSSigningInformation, &info) == errSecSuccess && info) {
            const auto identifier = static_cast<CFStringRef>(CFDictionaryGetValue(info, kSecCodeInfoIdentifier));
            const QString id = identifier ? QString::fromCFString(identifier) : QString();
            if (id != QLatin1String(ERGOPRO2_BUNDLE_ID))
                problem = tr("it is signed with the identifier '%1' instead of '%2'")
                              .arg(id, QLatin1String(ERGOPRO2_BUNDLE_ID));
            CFRelease(info);
        }
    }
    CFRelease(staticCode);
    return problem;
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
    if (!accessibility.granted) {
        // Explain the most common reason why a switched-on permission is not
        // recognized; logged once per distinct problem (check() runs every 3 s).
        static bool logged = false;
        static QString lastLogged;
        const QString problem = signatureProblem();
        if (!logged || problem != lastLogged) {
            logged = true;
            lastLogged = problem;
            if (problem.isEmpty())
                qCInfo(lcPlatform) << "Accessibility not granted; the code signature is valid";
            else
                qCWarning(lcPlatform).noquote() << "Accessibility cannot be matched to this copy of ErgoPro2:"
                                                << problem;
        }
        if (!problem.isEmpty()) {
            accessibility.howToGrant +=
                QLatin1Char(' ')
                + tr("If it is already on: macOS cannot match it to this copy of ErgoPro2 because %1. Sign the "
                     "app (documentation/BUILD.md 1.6), remove ErgoPro2 from the Accessibility list with “−”, "
                     "then add it again.")
                      .arg(problem);
        }
    }
    accessibility.fixKind = FixKind::OpenSettings;
    accessibility.fixLabel = tr("Open System Settings");
    accessibility.requiredFor = {Feature::AutoClick, Feature::KeyControl};

    PermissionState inputMonitoring;
    inputMonitoring.id = kInputMonitoring;
    inputMonitoring.displayName = tr("Input Monitoring");
    inputMonitoring.granted = inputMonitoringGranted();
    inputMonitoring.explanation = tr("Lets ErgoPro2 notice mouse movement and key presses in all applications "
                                     "(Auto Click, Key Control hotkeys, the Break Timer's activity tracking and the "
                                     "Statistics). ErgoPro2 only counts key presses and clicks; it never records "
                                     "or stores what you type.");
    inputMonitoring.howToGrant =
        tr("Turn on ErgoPro2 in System Settings › Privacy & Security › Input Monitoring.");
    inputMonitoring.fixKind = FixKind::OpenSettings;
    inputMonitoring.fixLabel = tr("Open System Settings");
    inputMonitoring.requiredFor = {Feature::AutoClick, Feature::KeyControl, Feature::BreakTimerActivity,
                                   Feature::Statistics};

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
