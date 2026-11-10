#include "platform/mac/MacInput.h"

#include "core/Clock.h"
#include "core/LogCategories.h"
#include "platform/common/Synthetic.h"

#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h> // kVK_* virtual key codes

#include <QCoreApplication>

#include <future>

namespace ergo {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::MacInput", text);
}

Modifiers modifiersFromFlags(CGEventFlags flags)
{
    Modifiers m = Modifier::None;
    if (flags & kCGEventFlagMaskShift)
        m |= Modifier::Shift;
    if (flags & kCGEventFlagMaskControl)
        m |= Modifier::Control;
    if (flags & kCGEventFlagMaskAlternate)
        m |= Modifier::Alt;
    if (flags & kCGEventFlagMaskCommand)
        m |= Modifier::Meta;
    return m;
}

int screenIndexAt(CGPoint point)
{
    CGDirectDisplayID displays[16];
    uint32_t count = 0;
    if (CGGetDisplaysWithPoint(point, 1, displays, &count) != kCGErrorSuccess || count == 0)
        return -1;
    CGDirectDisplayID active[16];
    uint32_t activeCount = 0;
    if (CGGetActiveDisplayList(16, active, &activeCount) != kCGErrorSuccess)
        return -1;
    for (uint32_t i = 0; i < activeCount; ++i) {
        if (active[i] == displays[0])
            return int(i);
    }
    return -1;
}

MouseButtonId buttonFromNumber(int64_t number)
{
    switch (number) {
    case 0: return MouseButtonId::Left;
    case 1: return MouseButtonId::Right;
    case 2: return MouseButtonId::Middle;
    case 3: return MouseButtonId::Back;
    case 4: return MouseButtonId::Forward;
    default: return MouseButtonId::Other;
    }
}

// Modifier flag belonging to a modifier key code (for kCGEventFlagsChanged).
CGEventFlags flagForModifierKey(quint16 keyCode)
{
    switch (keyCode) {
    case kVK_Shift:
    case kVK_RightShift: return kCGEventFlagMaskShift;
    case kVK_Control:
    case kVK_RightControl: return kCGEventFlagMaskControl;
    case kVK_Option:
    case kVK_RightOption: return kCGEventFlagMaskAlternate;
    case kVK_Command:
    case kVK_RightCommand: return kCGEventFlagMaskCommand;
    case kVK_CapsLock: return kCGEventFlagMaskAlphaShift;
    case kVK_Function: return kCGEventFlagMaskSecondaryFn;
    default: return 0;
    }
}

} // namespace

// ---------------------------------------------------------------- key table

int macKeyCodeToQtKey(quint16 keyCode, bool *keypad)
{
    *keypad = false;
    switch (keyCode) {
    case kVK_ANSI_A: return Qt::Key_A;
    case kVK_ANSI_B: return Qt::Key_B;
    case kVK_ANSI_C: return Qt::Key_C;
    case kVK_ANSI_D: return Qt::Key_D;
    case kVK_ANSI_E: return Qt::Key_E;
    case kVK_ANSI_F: return Qt::Key_F;
    case kVK_ANSI_G: return Qt::Key_G;
    case kVK_ANSI_H: return Qt::Key_H;
    case kVK_ANSI_I: return Qt::Key_I;
    case kVK_ANSI_J: return Qt::Key_J;
    case kVK_ANSI_K: return Qt::Key_K;
    case kVK_ANSI_L: return Qt::Key_L;
    case kVK_ANSI_M: return Qt::Key_M;
    case kVK_ANSI_N: return Qt::Key_N;
    case kVK_ANSI_O: return Qt::Key_O;
    case kVK_ANSI_P: return Qt::Key_P;
    case kVK_ANSI_Q: return Qt::Key_Q;
    case kVK_ANSI_R: return Qt::Key_R;
    case kVK_ANSI_S: return Qt::Key_S;
    case kVK_ANSI_T: return Qt::Key_T;
    case kVK_ANSI_U: return Qt::Key_U;
    case kVK_ANSI_V: return Qt::Key_V;
    case kVK_ANSI_W: return Qt::Key_W;
    case kVK_ANSI_X: return Qt::Key_X;
    case kVK_ANSI_Y: return Qt::Key_Y;
    case kVK_ANSI_Z: return Qt::Key_Z;
    case kVK_ANSI_0: return Qt::Key_0;
    case kVK_ANSI_1: return Qt::Key_1;
    case kVK_ANSI_2: return Qt::Key_2;
    case kVK_ANSI_3: return Qt::Key_3;
    case kVK_ANSI_4: return Qt::Key_4;
    case kVK_ANSI_5: return Qt::Key_5;
    case kVK_ANSI_6: return Qt::Key_6;
    case kVK_ANSI_7: return Qt::Key_7;
    case kVK_ANSI_8: return Qt::Key_8;
    case kVK_ANSI_9: return Qt::Key_9;
    case kVK_ANSI_Minus: return Qt::Key_Minus;
    case kVK_ANSI_Equal: return Qt::Key_Equal;
    case kVK_ANSI_LeftBracket: return Qt::Key_BracketLeft;
    case kVK_ANSI_RightBracket: return Qt::Key_BracketRight;
    case kVK_ANSI_Backslash: return Qt::Key_Backslash;
    case kVK_ANSI_Semicolon: return Qt::Key_Semicolon;
    case kVK_ANSI_Quote: return Qt::Key_Apostrophe;
    case kVK_ANSI_Comma: return Qt::Key_Comma;
    case kVK_ANSI_Period: return Qt::Key_Period;
    case kVK_ANSI_Slash: return Qt::Key_Slash;
    case kVK_ANSI_Grave: return Qt::Key_QuoteLeft;
    case kVK_Return: return Qt::Key_Return;
    case kVK_Tab: return Qt::Key_Tab;
    case kVK_Space: return Qt::Key_Space;
    case kVK_Delete: return Qt::Key_Backspace;
    case kVK_ForwardDelete: return Qt::Key_Delete;
    case kVK_Escape: return Qt::Key_Escape;
    case kVK_Command:
    case kVK_RightCommand: return Qt::Key_Meta;
    case kVK_Shift:
    case kVK_RightShift: return Qt::Key_Shift;
    case kVK_Option:
    case kVK_RightOption: return Qt::Key_Alt;
    case kVK_Control:
    case kVK_RightControl: return Qt::Key_Control;
    case kVK_CapsLock: return Qt::Key_CapsLock;
    case kVK_Function: return 0; // Fn / Globe cannot be used (B.5.1)
    case kVK_F1: return Qt::Key_F1;
    case kVK_F2: return Qt::Key_F2;
    case kVK_F3: return Qt::Key_F3;
    case kVK_F4: return Qt::Key_F4;
    case kVK_F5: return Qt::Key_F5;
    case kVK_F6: return Qt::Key_F6;
    case kVK_F7: return Qt::Key_F7;
    case kVK_F8: return Qt::Key_F8;
    case kVK_F9: return Qt::Key_F9;
    case kVK_F10: return Qt::Key_F10;
    case kVK_F11: return Qt::Key_F11;
    case kVK_F12: return Qt::Key_F12;
    case kVK_F13: return Qt::Key_F13;
    case kVK_F14: return Qt::Key_F14;
    case kVK_F15: return Qt::Key_F15;
    case kVK_F16: return Qt::Key_F16;
    case kVK_F17: return Qt::Key_F17;
    case kVK_F18: return Qt::Key_F18;
    case kVK_F19: return Qt::Key_F19;
    case kVK_F20: return Qt::Key_F20;
    case kVK_Help: return Qt::Key_Insert;
    case kVK_Home: return Qt::Key_Home;
    case kVK_End: return Qt::Key_End;
    case kVK_PageUp: return Qt::Key_PageUp;
    case kVK_PageDown: return Qt::Key_PageDown;
    case kVK_LeftArrow: return Qt::Key_Left;
    case kVK_RightArrow: return Qt::Key_Right;
    case kVK_UpArrow: return Qt::Key_Up;
    case kVK_DownArrow: return Qt::Key_Down;
    default: break;
    }
    *keypad = true;
    switch (keyCode) {
    case kVK_ANSI_Keypad0: return Qt::Key_0;
    case kVK_ANSI_Keypad1: return Qt::Key_1;
    case kVK_ANSI_Keypad2: return Qt::Key_2;
    case kVK_ANSI_Keypad3: return Qt::Key_3;
    case kVK_ANSI_Keypad4: return Qt::Key_4;
    case kVK_ANSI_Keypad5: return Qt::Key_5;
    case kVK_ANSI_Keypad6: return Qt::Key_6;
    case kVK_ANSI_Keypad7: return Qt::Key_7;
    case kVK_ANSI_Keypad8: return Qt::Key_8;
    case kVK_ANSI_Keypad9: return Qt::Key_9;
    case kVK_ANSI_KeypadDecimal: return Qt::Key_Period;
    case kVK_ANSI_KeypadMultiply: return Qt::Key_Asterisk;
    case kVK_ANSI_KeypadPlus: return Qt::Key_Plus;
    case kVK_ANSI_KeypadMinus: return Qt::Key_Minus;
    case kVK_ANSI_KeypadDivide: return Qt::Key_Slash;
    case kVK_ANSI_KeypadEquals: return Qt::Key_Equal;
    case kVK_ANSI_KeypadEnter: return Qt::Key_Enter;
    case kVK_ANSI_KeypadClear: return Qt::Key_Clear;
    default: break;
    }
    *keypad = false;
    return 0;
}

// ---------------------------------------------------------------- hook

struct MacInputHook::State {
    CFMachPortRef tap = nullptr;
    CFRunLoopSourceRef source = nullptr;
    CFRunLoopRef runLoop = nullptr;
    InputEventSink sink;
    KeySuppressionRegistry *suppression = nullptr;
    bool active = false;
    std::atomic<int> *selfRepairs = nullptr;
};

namespace {

// Runs on the hook thread for every event. Must stay fast: no logging I/O,
// no allocation beyond the event copy, only the short suppression mutex.
CGEventRef tapCallback(CGEventTapProxy, CGEventType type, CGEventRef cgEvent, void *refcon)
{
    auto *s = static_cast<MacInputHook::State *>(refcon);

    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        // macOS disables taps whose callback was too slow or on secure input:
        // re-enable immediately (B.5.4). The watchdog logs the counter.
        if (s->tap)
            CGEventTapEnable(s->tap, true);
        s->selfRepairs->fetch_add(1);
        return cgEvent;
    }

    InputEvent e;
    e.timestampMs = monotonicNowMs();
    e.synthetic = CGEventGetIntegerValueField(cgEvent, kCGEventSourceUserData) == kSyntheticEventMarker;
    e.modifiers = modifiersFromFlags(CGEventGetFlags(cgEvent));
    const CGPoint location = CGEventGetLocation(cgEvent); // global, points = logical pixels
    e.position = QPointF(location.x, location.y);

    switch (type) {
    case kCGEventMouseMoved:
    case kCGEventLeftMouseDragged:
    case kCGEventRightMouseDragged:
    case kCGEventOtherMouseDragged:
        e.type = InputEventType::MouseMove;
        break;
    case kCGEventLeftMouseDown:
    case kCGEventRightMouseDown:
    case kCGEventOtherMouseDown:
        e.type = InputEventType::MouseDown;
        e.button = buttonFromNumber(CGEventGetIntegerValueField(cgEvent, kCGMouseEventButtonNumber));
        e.screen = screenIndexAt(location);
        break;
    case kCGEventLeftMouseUp:
    case kCGEventRightMouseUp:
    case kCGEventOtherMouseUp:
        e.type = InputEventType::MouseUp;
        e.button = buttonFromNumber(CGEventGetIntegerValueField(cgEvent, kCGMouseEventButtonNumber));
        e.screen = screenIndexAt(location);
        break;
    case kCGEventScrollWheel:
        e.type = InputEventType::Wheel;
        e.wheelDeltaY = int(CGEventGetIntegerValueField(cgEvent, kCGScrollWheelEventDeltaAxis1));
        e.wheelDeltaX = int(CGEventGetIntegerValueField(cgEvent, kCGScrollWheelEventDeltaAxis2));
        break;
    case kCGEventKeyDown:
    case kCGEventKeyUp: {
        e.type = type == kCGEventKeyDown ? InputEventType::KeyDown : InputEventType::KeyUp;
        e.nativeKeyCode = quint32(CGEventGetIntegerValueField(cgEvent, kCGKeyboardEventKeycode));
        e.key = macKeyCodeToQtKey(quint16(e.nativeKeyCode), &e.keypad);
        e.autoRepeat = CGEventGetIntegerValueField(cgEvent, kCGKeyboardEventAutorepeat) != 0;
        break;
    }
    case kCGEventFlagsChanged: {
        // Modifier keys only produce flag changes: derive down/up from the flag.
        e.nativeKeyCode = quint32(CGEventGetIntegerValueField(cgEvent, kCGKeyboardEventKeycode));
        e.key = macKeyCodeToQtKey(quint16(e.nativeKeyCode), &e.keypad);
        const CGEventFlags flag = flagForModifierKey(quint16(e.nativeKeyCode));
        if (flag == 0 || e.key == 0)
            return cgEvent;
        e.type = (CGEventGetFlags(cgEvent) & flag) ? InputEventType::KeyDown : InputEventType::KeyUp;
        break;
    }
    default:
        return cgEvent;
    }

    if (e.isKey() && s->active && s->suppression && s->suppression->shouldSwallow(e))
        e.swallowed = true;
    if (s->active && s->suppression) {
        // Drag Lock (B.5.3, D-03): the click that releases the drag is consumed.
        if (e.isMouseButton() && s->suppression->shouldSwallowMouse(e))
            e.swallowed = true;
        // A left button held by CGEventPost does not turn hardware moves into
        // drags: applications would get plain mouse-moved events and nothing
        // would be dragged or selected. While the drag lock holds the button,
        // real moves are therefore re-typed as left-mouse-dragged events.
        if (type == kCGEventMouseMoved && !e.synthetic && s->suppression->isDragLockHolding()) {
            CGEventSetType(cgEvent, kCGEventLeftMouseDragged);
            CGEventSetIntegerValueField(cgEvent, kCGMouseEventButtonNumber, kCGMouseButtonLeft);
        }
    }
    s->sink(e);
    return e.swallowed ? nullptr : cgEvent; // nullptr = event never reaches other apps
}

} // namespace

MacInputHook::MacInputHook()
    : m_state(std::make_unique<State>())
{
}

MacInputHook::~MacInputHook()
{
    stop();
}

QString MacInputHook::backendName() const
{
    if (!m_running.load())
        return QStringLiteral("CGEventTap");
    return m_active.load() ? QStringLiteral("CGEventTap (active)") : QStringLiteral("CGEventTap (listen-only)");
}

PlatformResult MacInputHook::start(InputEventSink sink, KeySuppressionRegistry *suppression)
{
    if (m_running.load())
        return PlatformResult::success();
    if (m_thread.joinable())
        m_thread.join();

    m_state = std::make_unique<State>();
    m_state->sink = std::move(sink);
    m_state->suppression = suppression;
    m_state->selfRepairs = &m_selfRepairs;

    std::promise<PlatformResult> started;
    std::future<PlatformResult> result = started.get_future();

    m_thread = std::thread([this, &started] {
        State *s = m_state.get();
        const CGEventMask mask =
            CGEventMaskBit(kCGEventMouseMoved) | CGEventMaskBit(kCGEventLeftMouseDragged)
            | CGEventMaskBit(kCGEventRightMouseDragged) | CGEventMaskBit(kCGEventOtherMouseDragged)
            | CGEventMaskBit(kCGEventLeftMouseDown) | CGEventMaskBit(kCGEventLeftMouseUp)
            | CGEventMaskBit(kCGEventRightMouseDown) | CGEventMaskBit(kCGEventRightMouseUp)
            | CGEventMaskBit(kCGEventOtherMouseDown) | CGEventMaskBit(kCGEventOtherMouseUp)
            | CGEventMaskBit(kCGEventScrollWheel) | CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp)
            | CGEventMaskBit(kCGEventFlagsChanged);

        // Active tap first (can block keys; needs Accessibility), then listen-only.
        s->tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, mask,
                                  tapCallback, s);
        s->active = s->tap != nullptr;
        if (!s->tap)
            s->tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
                                      mask, tapCallback, s);
        if (!s->tap) {
            started.set_value(PlatformResult::permissionDenied(
                tr("macOS refused to create the event tap. Grant ErgoPro2 the Input Monitoring and "
                   "Accessibility permissions.")));
            return;
        }

        s->source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, s->tap, 0);
        s->runLoop = CFRunLoopGetCurrent();
        CFRetain(s->runLoop);
        CFRunLoopAddSource(s->runLoop, s->source, kCFRunLoopCommonModes);
        CGEventTapEnable(s->tap, true);
        m_active.store(s->active);
        m_running.store(true);
        started.set_value(PlatformResult::success());

        CFRunLoopRun(); // until stop() calls CFRunLoopStop

        CFRunLoopRemoveSource(s->runLoop, s->source, kCFRunLoopCommonModes);
        CGEventTapEnable(s->tap, false);
        CFMachPortInvalidate(s->tap);
        CFRelease(s->source);
        CFRelease(s->tap);
        s->source = nullptr;
        s->tap = nullptr;
        m_running.store(false);
    });

    const PlatformResult r = result.get();
    if (!r.ok()) {
        m_thread.join();
        qCWarning(lcPlatform).noquote() << "Input hook not started:" << r.message;
        return r;
    }
    qCInfo(lcPlatform).noquote() << "Input hook started:" << backendName();
    if (!m_active.load())
        qCWarning(lcPlatform) << "Event tap is listen-only (Accessibility missing): keys cannot be blocked";
    return r;
}

void MacInputHook::stop()
{
    if (m_state && m_state->runLoop) {
        CFRunLoopStop(m_state->runLoop); // thread-safe
    }
    if (m_thread.joinable())
        m_thread.join();
    if (m_state && m_state->runLoop) {
        CFRelease(m_state->runLoop);
        m_state->runLoop = nullptr;
    }
    m_running.store(false);
}

bool MacInputHook::checkHealth()
{
    if (!m_running.load() || !m_state || !m_state->tap)
        return false;
    if (!CFMachPortIsValid(m_state->tap))
        return false;
    if (!CGEventTapIsEnabled(m_state->tap)) {
        CGEventTapEnable(m_state->tap, true);
        m_selfRepairs.fetch_add(1);
        qCWarning(lcPlatform) << "Event tap was disabled; re-enabled by the watchdog";
    }
    return CGEventTapIsEnabled(m_state->tap);
}

PlatformResult MacInputHook::registerKey(const KeyCombo &combo)
{
    // Keys are blocked inside the active tap (see tapCallback).
    if (!combo.isValid())
        return PlatformResult::failed(tr("Invalid key combination."));
    if (m_running.load() && !m_active.load())
        return PlatformResult::permissionDenied(
            tr("Keys cannot be blocked without the Accessibility permission."));
    return PlatformResult::success();
}

PlatformResult MacInputHook::unregisterKey(const KeyCombo &)
{
    return PlatformResult::success();
}

// ---------------------------------------------------------------- injector

namespace {

PlatformResult postMouse(CGEventType type, CGMouseButton button, int clickState)
{
    if (!AXIsProcessTrusted()) {
        // Without Accessibility macOS silently drops posted events.
        return PlatformResult::permissionDenied(tr("Clicks need the Accessibility permission."));
    }
    CGEventRef current = CGEventCreate(nullptr);
    const CGPoint position = CGEventGetLocation(current);
    CFRelease(current);

    CGEventRef event = CGEventCreateMouseEvent(nullptr, type, position, button);
    if (!event)
        return PlatformResult::failed(tr("Cannot create the mouse event."));
    CGEventSetIntegerValueField(event, kCGEventSourceUserData, kSyntheticEventMarker);
    CGEventSetIntegerValueField(event, kCGMouseEventClickState, clickState);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    return PlatformResult::success();
}

struct ButtonTypes {
    CGEventType down;
    CGEventType up;
    CGMouseButton button;
};

ButtonTypes typesFor(MouseButton button)
{
    switch (button) {
    case MouseButton::Right:
        return {kCGEventRightMouseDown, kCGEventRightMouseUp, kCGMouseButtonRight};
    case MouseButton::Middle:
        return {kCGEventOtherMouseDown, kCGEventOtherMouseUp, kCGMouseButtonCenter};
    case MouseButton::Left:
        break;
    }
    return {kCGEventLeftMouseDown, kCGEventLeftMouseUp, kCGMouseButtonLeft};
}

} // namespace

PlatformResult MacClickInjector::press(MouseButton button)
{
    const ButtonTypes t = typesFor(button);
    return postMouse(t.down, t.button, 1);
}

PlatformResult MacClickInjector::release(MouseButton button)
{
    const ButtonTypes t = typesFor(button);
    return postMouse(t.up, t.button, 1);
}

PlatformResult MacClickInjector::click(MouseButton button)
{
    const ButtonTypes t = typesFor(button);
    if (PlatformResult r = postMouse(t.down, t.button, 1); !r.ok())
        return r;
    return postMouse(t.up, t.button, 1);
}

PlatformResult MacClickInjector::doubleClick(MouseButton button)
{
    // macOS recognizes a double click by the click-state field (1, then 2).
    const ButtonTypes t = typesFor(button);
    for (int state : {1, 2}) {
        if (PlatformResult r = postMouse(t.down, t.button, state); !r.ok())
            return r;
        if (PlatformResult r = postMouse(t.up, t.button, state); !r.ok())
            return r;
    }
    return PlatformResult::success();
}

int MacClickInjector::doubleClickIntervalMs() const
{
    return int(NSEvent.doubleClickInterval * 1000.0);
}

// ---------------------------------------------------------------- power events

MacPowerEvents::~MacPowerEvents()
{
    stop();
}

PlatformResult MacPowerEvents::start(std::function<void(PowerEvent)> callback)
{
    if (m_started)
        return PlatformResult::success();
    NSNotificationCenter *workspace = NSWorkspace.sharedWorkspace.notificationCenter;
    NSDistributedNotificationCenter *distributed = NSDistributedNotificationCenter.defaultCenter;
    NSOperationQueue *main = NSOperationQueue.mainQueue;

    auto add = [&](NSNotificationCenter *center, NSString *name, PowerEvent event) {
        id token = [center addObserverForName:name
                                       object:nil
                                        queue:main
                                   usingBlock:^(NSNotification *) { callback(event); }];
        m_observers.append((__bridge_retained void *)token);
    };
    add(workspace, NSWorkspaceWillSleepNotification, PowerEvent::Sleep);
    add(workspace, NSWorkspaceDidWakeNotification, PowerEvent::Resume);
    add(workspace, NSWorkspaceSessionDidResignActiveNotification, PowerEvent::ScreenLocked);
    add(workspace, NSWorkspaceSessionDidBecomeActiveNotification, PowerEvent::ScreenUnlocked);
    add(distributed, @"com.apple.screenIsLocked", PowerEvent::ScreenLocked);
    add(distributed, @"com.apple.screenIsUnlocked", PowerEvent::ScreenUnlocked);
    m_started = true;
    qCInfo(lcPlatform) << "Listening for sleep/wake and screen lock notifications";
    return PlatformResult::success();
}

void MacPowerEvents::stop()
{
    if (!m_started)
        return;
    NSNotificationCenter *workspace = NSWorkspace.sharedWorkspace.notificationCenter;
    NSDistributedNotificationCenter *distributed = NSDistributedNotificationCenter.defaultCenter;
    for (void *raw : std::as_const(m_observers)) {
        id token = (__bridge_transfer id)raw;
        [workspace removeObserver:token];
        [distributed removeObserver:token];
    }
    m_observers.clear();
    m_started = false;
}

} // namespace ergo
