#include "platform/mac/MacDesktopIntegration.h"

#include "core/LogCategories.h"

#import <AppKit/AppKit.h>
#include <CoreServices/CoreServices.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>

#include <cmath>
#include <unistd.h>

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

void MacDesktopIntegration::raiseOverlayWindow(quintptr nativeWindow)
{
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(nativeWindow);
    NSWindow *window = view.window;
    if (!window) {
        qCWarning(lcPlatform) << "raiseOverlayWindow: no native window";
        return;
    }
    // Screen-saver level: above normal, floating and full-screen app windows.
    window.level = NSScreenSaverWindowLevel;
    // Visible on every Space, also next to full-screen apps; not in Cmd+` cycling.
    window.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces
                                | NSWindowCollectionBehaviorFullScreenAuxiliary
                                | NSWindowCollectionBehaviorStationary | NSWindowCollectionBehaviorIgnoresCycle;
    window.hidesOnDeactivate = NO;
    [window orderFrontRegardless];
}

std::optional<bool> MacDesktopIntegration::isFullscreenAppActive() const
{
    // An on-screen window of another app that covers a whole display. Window
    // bounds and owners are readable without the Screen Recording permission.
    CGDirectDisplayID displays[16];
    uint32_t displayCount = 0;
    if (CGGetActiveDisplayList(16, displays, &displayCount) != kCGErrorSuccess)
        return std::nullopt;
    CFArrayRef windows =
        CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (!windows)
        return std::nullopt;
    const pid_t self = getpid();
    bool found = false;
    for (CFIndex i = 0; i < CFArrayGetCount(windows) && !found; ++i) {
        auto info = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windows, i));
        int layer = -1;
        int pid = 0;
        CFNumberGetValue(static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowLayer)), kCFNumberIntType, &layer);
        CFNumberGetValue(static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowOwnerPID)), kCFNumberIntType, &pid);
        if (layer != 0 || pid == self)
            continue;
        CGRect bounds;
        if (!CGRectMakeWithDictionaryRepresentation(static_cast<CFDictionaryRef>(CFDictionaryGetValue(info, kCGWindowBounds)),
                                                   &bounds))
            continue;
        for (uint32_t d = 0; d < displayCount; ++d) {
            const CGRect screen = CGDisplayBounds(displays[d]);
            if (std::abs(bounds.origin.x - screen.origin.x) < 1 && std::abs(bounds.origin.y - screen.origin.y) < 1
                && std::abs(bounds.size.width - screen.size.width) < 1
                && std::abs(bounds.size.height - screen.size.height) < 1) {
                found = true;
                break;
            }
        }
    }
    CFRelease(windows);
    return found;
}

} // namespace ergo
