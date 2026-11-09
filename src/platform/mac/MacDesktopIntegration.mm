#include "platform/mac/MacDesktopIntegration.h"

#include "core/LogCategories.h"

#import <AppKit/AppKit.h>
#include <CoreServices/CoreServices.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace ergo {

void MacDesktopIntegration::setDockIconVisible(bool visible)
{
    // Regular = Dock icon + app menu; Accessory = menu bar (status item) only.
    const NSApplicationActivationPolicy policy =
        visible ? NSApplicationActivationPolicyRegular : NSApplicationActivationPolicyAccessory;
    if (NSApp.activationPolicy == policy)
        return;
    if (![NSApp setActivationPolicy:policy]) {
        qCWarning(lcPlatform) << "NSApp setActivationPolicy failed, requested Dock icon visible =" << visible;
        return;
    }
    qCDebug(lcPlatform) << "Dock icon visible:" << visible;
}

void MacDesktopIntegration::activateApplication()
{
    // After switching back to the Regular policy the app must be activated,
    // otherwise the window appears behind other applications.
    if (@available(macOS 14.0, *))
        [NSApp activate];
    else
        [NSApp activateIgnoringOtherApps:YES];
}

bool MacDesktopIntegration::isSessionEnding() const
{
    // When macOS logs out / restarts / shuts down, it sends a "quit" Apple
    // Event carrying a quit reason. Cmd+Q from the app menu has no such event.
    NSAppleEventDescriptor *event = [[NSAppleEventManager sharedAppleEventManager] currentAppleEvent];
    if (!event || event.eventClass != kCoreEventClass || event.eventID != kAEQuitApplication)
        return false;
    NSAppleEventDescriptor *reason = [event attributeDescriptorForKeyword:kAEQuitReason];
    if (!reason)
        return false;
    switch (reason.enumCodeValue) {
    case kAELogOut:
    case kAEReallyLogOut:
    case kAEShowRestartDialog:
    case kAEShowShutdownDialog:
    case kAERestart:
    case kAEShutDown:
        qCInfo(lcPlatform) << "macOS session is ending (logout/restart/shutdown)";
        return true;
    default:
        return false;
    }
}

PlatformResult MacDesktopIntegration::launchApplication(const QString &executablePath, const QStringList &arguments)
{
    // Start the enclosing .app through LaunchServices so macOS treats it as a
    // normal app launch (permissions are attributed to ErgoPro2, not a terminal).
    QDir bundle(QFileInfo(executablePath).absolutePath()); // .../Contents/MacOS
    bundle.cdUp();
    bundle.cdUp();
    bool started = false;
    if (bundle.absolutePath().endsWith(QLatin1String(".app"))) {
        QStringList openArguments{QStringLiteral("-n"), bundle.absolutePath()};
        if (!arguments.isEmpty())
            openArguments << QStringLiteral("--args") << arguments;
        started = QProcess::startDetached(QStringLiteral("/usr/bin/open"), openArguments);
    } else {
        started = QProcess::startDetached(executablePath, arguments);
    }
    if (!started) {
        const QString message =
            QCoreApplication::translate("ergo::MacDesktopIntegration", "Cannot start '%1'.").arg(executablePath);
        qCCritical(lcPlatform).noquote() << message;
        return PlatformResult::failed(message);
    }
    return PlatformResult::success();
}

} // namespace ergo
