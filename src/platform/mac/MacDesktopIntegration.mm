#include "platform/mac/MacDesktopIntegration.h"

#include "core/LogCategories.h"

#import <AppKit/AppKit.h>
#include <CoreServices/CoreServices.h>

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

} // namespace ergo
