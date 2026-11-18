#include "platform/mac/MacAppWindows.h"

#include "core/LogCategories.h"
#include "platform/common/Synthetic.h"

#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>

namespace ergo {

namespace {

// Bundle identifiers (new Teams first).
NSArray<NSString *> *bundleIds(KeepActiveApp app)
{
    switch (app) {
    case KeepActiveApp::Teams:
        return @[ @"com.microsoft.teams2", @"com.microsoft.teams" ];
    case KeepActiveApp::Zoom:
        return @[ @"us.zoom.xos" ];
    }
    return @[];
}

NSRunningApplication *runningApp(KeepActiveApp app)
{
    for (NSString *bundleId in bundleIds(app)) {
        NSArray<NSRunningApplication *> *apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:bundleId];
        for (NSRunningApplication *a in apps) {
            if (!a.terminated)
                return a;
        }
    }
    return nil;
}

bool postMouse(CGEventType type, CGPoint position)
{
    CGEventRef event = CGEventCreateMouseEvent(nullptr, type, position, kCGMouseButtonLeft);
    if (!event)
        return false;
    CGEventSetIntegerValueField(event, kCGEventSourceUserData, kSyntheticEventMarker);
    if (type == kCGEventLeftMouseDown || type == kCGEventLeftMouseUp)
        CGEventSetIntegerValueField(event, kCGMouseEventClickState, 1);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    return true;
}

bool pointerPosition(CGPoint *position)
{
    CGEventRef current = CGEventCreate(nullptr);
    if (!current)
        return false;
    *position = CGEventGetLocation(current);
    CFRelease(current);
    return true;
}

} // namespace

IAppWindows::Raise MacAppWindows::raise(KeepActiveApp app, QString *detail)
{
    @autoreleasepool {
        NSRunningApplication *running = runningApp(app);
        if (!running)
            return Raise::NotRunning;
        if (running.hidden)
            [running unhide];

        // Restore minimized windows and raise the largest one (Accessibility).
        bool hasWindow = false;
        AXUIElementRef axApp = AXUIElementCreateApplication(running.processIdentifier);
        CFArrayRef windows = nullptr;
        if (axApp && AXUIElementCopyAttributeValue(axApp, kAXWindowsAttribute, (CFTypeRef *)&windows) == kAXErrorSuccess
            && windows) {
            AXUIElementRef best = nullptr;
            double bestArea = 0;
            for (CFIndex i = 0; i < CFArrayGetCount(windows); ++i) {
                auto window = (AXUIElementRef)CFArrayGetValueAtIndex(windows, i);
                CFTypeRef minimized = nullptr;
                if (AXUIElementCopyAttributeValue(window, kAXMinimizedAttribute, &minimized) == kAXErrorSuccess
                    && minimized) {
                    if (CFBooleanGetValue((CFBooleanRef)minimized))
                        AXUIElementSetAttributeValue(window, kAXMinimizedAttribute, kCFBooleanFalse);
                    CFRelease(minimized);
                }
                CFTypeRef sizeValue = nullptr;
                CGSize size = CGSizeZero;
                if (AXUIElementCopyAttributeValue(window, kAXSizeAttribute, &sizeValue) == kAXErrorSuccess && sizeValue) {
                    AXValueGetValue((AXValueRef)sizeValue, (AXValueType)kAXValueCGSizeType, &size);
                    CFRelease(sizeValue);
                }
                if (size.width * size.height > bestArea) {
                    bestArea = size.width * size.height;
                    best = window;
                }
            }
            if (best) {
                hasWindow = true;
                AXUIElementPerformAction(best, kAXRaiseAction);
            }
            CFRelease(windows);
        } else if (detail) {
            *detail = tr("the window list of the application could not be read (Accessibility)");
        }
        if (axApp)
            CFRelease(axApp);

        if (!hasWindow && running.bundleURL) {
            // Only in the menu bar / no window open: "reopen" like a click on its
            // Dock icon, which shows the main window again.
            NSWorkspaceOpenConfiguration *config = [NSWorkspaceOpenConfiguration configuration];
            config.activates = YES;
            [[NSWorkspace sharedWorkspace] openApplicationAtURL:running.bundleURL
                                                  configuration:config
                                              completionHandler:nil];
        }
        [running activateWithOptions:NSApplicationActivateAllWindows];
        return Raise::Raised;
    }
}

std::optional<QRectF> MacAppWindows::mainWindowFrame(KeepActiveApp app) const
{
    @autoreleasepool {
        NSRunningApplication *running = runningApp(app);
        if (!running)
            return std::nullopt;
        const pid_t pid = running.processIdentifier;
        CFArrayRef list = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
                                                     kCGNullWindowID);
        if (!list)
            return std::nullopt;
        std::optional<QRectF> best;
        double bestArea = 0;
        for (NSDictionary *info in (__bridge NSArray *)list) {
            if ([info[(id)kCGWindowOwnerPID] intValue] != pid || [info[(id)kCGWindowLayer] intValue] != 0)
                continue;
            CGRect bounds;
            if (!CGRectMakeWithDictionaryRepresentation((__bridge CFDictionaryRef)info[(id)kCGWindowBounds], &bounds))
                continue;
            const double area = bounds.size.width * bounds.size.height;
            if (area > bestArea) {
                bestArea = area;
                // Global, top-left origin, points = Qt logical pixels.
                best = QRectF(bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
            }
        }
        CFRelease(list);
        return best;
    }
}

PlatformResult MacAppWindows::clickAt(const QPointF &point)
{
    if (!AXIsProcessTrusted())
        return PlatformResult::permissionDenied(tr("Clicks need the Accessibility permission."));
    CGPoint original;
    if (!pointerPosition(&original))
        return PlatformResult::failed(tr("Cannot read the pointer position."));
    const CGPoint target = CGPointMake(point.x(), point.y());
    const bool ok = postMouse(kCGEventMouseMoved, target) && postMouse(kCGEventLeftMouseDown, target)
                    && postMouse(kCGEventLeftMouseUp, target);
    postMouse(kCGEventMouseMoved, original); // always put the pointer back
    return ok ? PlatformResult::success() : PlatformResult::failed(tr("Cannot create the mouse event."));
}

PlatformResult MacAppWindows::nudgePointer()
{
    if (!AXIsProcessTrusted())
        return PlatformResult::permissionDenied(tr("Moving the pointer needs the Accessibility permission."));
    CGPoint original;
    if (!pointerPosition(&original))
        return PlatformResult::failed(tr("Cannot read the pointer position."));
    const bool ok = postMouse(kCGEventMouseMoved, CGPointMake(original.x + 1, original.y));
    postMouse(kCGEventMouseMoved, original);
    return ok ? PlatformResult::success() : PlatformResult::failed(tr("Cannot create the mouse event."));
}

} // namespace ergo
