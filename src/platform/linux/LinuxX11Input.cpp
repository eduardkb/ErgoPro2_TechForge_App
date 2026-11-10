#include "platform/linux/LinuxInput.h"

#include "core/Clock.h"
#include "core/LogCategories.h"

#include <QCoreApplication>
#include <QMutexLocker>

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/record.h>
#include <X11/keysym.h>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <deque>
#include <functional>
#include <future>

namespace ergo {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::LinuxX11Input", text);
}

struct KeyPair {
    unsigned long keysym;
    int key;
    bool keypad;
};

// Keysym <-> Qt::Key. Letters use lower-case keysyms (level 0 of the key).
const KeyPair kKeyTable[] = {
    {XK_Escape, Qt::Key_Escape, false}, {XK_Tab, Qt::Key_Tab, false}, {XK_ISO_Left_Tab, Qt::Key_Tab, false},
    {XK_BackSpace, Qt::Key_Backspace, false}, {XK_Return, Qt::Key_Return, false},
    {XK_Insert, Qt::Key_Insert, false}, {XK_Delete, Qt::Key_Delete, false}, {XK_Pause, Qt::Key_Pause, false},
    {XK_Print, Qt::Key_Print, false}, {XK_Home, Qt::Key_Home, false}, {XK_End, Qt::Key_End, false},
    {XK_Left, Qt::Key_Left, false}, {XK_Up, Qt::Key_Up, false}, {XK_Right, Qt::Key_Right, false},
    {XK_Down, Qt::Key_Down, false}, {XK_Prior, Qt::Key_PageUp, false}, {XK_Next, Qt::Key_PageDown, false},
    {XK_Shift_L, Qt::Key_Shift, false}, {XK_Shift_R, Qt::Key_Shift, false},
    {XK_Control_L, Qt::Key_Control, false}, {XK_Control_R, Qt::Key_Control, false},
    {XK_Alt_L, Qt::Key_Alt, false}, {XK_Alt_R, Qt::Key_Alt, false},
    {XK_Super_L, Qt::Key_Meta, false}, {XK_Super_R, Qt::Key_Meta, false},
    {XK_Meta_L, Qt::Key_Meta, false}, {XK_Meta_R, Qt::Key_Meta, false},
    {XK_ISO_Level3_Shift, Qt::Key_AltGr, false}, {XK_Caps_Lock, Qt::Key_CapsLock, false},
    {XK_Num_Lock, Qt::Key_NumLock, false}, {XK_Scroll_Lock, Qt::Key_ScrollLock, false},
    {XK_Menu, Qt::Key_Menu, false}, {XK_space, Qt::Key_Space, false},
    {XK_minus, Qt::Key_Minus, false}, {XK_equal, Qt::Key_Equal, false},
    {XK_bracketleft, Qt::Key_BracketLeft, false}, {XK_bracketright, Qt::Key_BracketRight, false},
    {XK_backslash, Qt::Key_Backslash, false}, {XK_semicolon, Qt::Key_Semicolon, false},
    {XK_apostrophe, Qt::Key_Apostrophe, false}, {XK_comma, Qt::Key_Comma, false},
    {XK_period, Qt::Key_Period, false}, {XK_slash, Qt::Key_Slash, false}, {XK_grave, Qt::Key_QuoteLeft, false},
    {XK_KP_0, Qt::Key_0, true}, {XK_KP_1, Qt::Key_1, true}, {XK_KP_2, Qt::Key_2, true},
    {XK_KP_3, Qt::Key_3, true}, {XK_KP_4, Qt::Key_4, true}, {XK_KP_5, Qt::Key_5, true},
    {XK_KP_6, Qt::Key_6, true}, {XK_KP_7, Qt::Key_7, true}, {XK_KP_8, Qt::Key_8, true},
    {XK_KP_9, Qt::Key_9, true}, {XK_KP_Insert, Qt::Key_0, true}, {XK_KP_End, Qt::Key_1, true},
    {XK_KP_Down, Qt::Key_2, true}, {XK_KP_Next, Qt::Key_3, true}, {XK_KP_Left, Qt::Key_4, true},
    {XK_KP_Begin, Qt::Key_5, true}, {XK_KP_Right, Qt::Key_6, true}, {XK_KP_Home, Qt::Key_7, true},
    {XK_KP_Up, Qt::Key_8, true}, {XK_KP_Prior, Qt::Key_9, true}, {XK_KP_Decimal, Qt::Key_Period, true},
    {XK_KP_Delete, Qt::Key_Period, true}, {XK_KP_Add, Qt::Key_Plus, true}, {XK_KP_Subtract, Qt::Key_Minus, true},
    {XK_KP_Multiply, Qt::Key_Asterisk, true}, {XK_KP_Divide, Qt::Key_Slash, true},
    {XK_KP_Enter, Qt::Key_Enter, true},
};

Modifiers modifiersFromState(unsigned int state)
{
    Modifiers m = 0; // not Modifier::None: X11 headers #define None
    if (state & ShiftMask)
        m |= Modifier::Shift;
    if (state & ControlMask)
        m |= Modifier::Control;
    if (state & Mod1Mask)
        m |= Modifier::Alt;
    if (state & Mod4Mask)
        m |= Modifier::Meta;
    return m;
}

unsigned int stateFromModifiers(Modifiers m)
{
    unsigned int state = 0;
    if (m & Modifier::Shift)
        state |= ShiftMask;
    if (m & Modifier::Control)
        state |= ControlMask;
    if (m & Modifier::Alt)
        state |= Mod1Mask;
    if (m & Modifier::Meta)
        state |= Mod4Mask;
    return state;
}

// NumLock (Mod2) and CapsLock (Lock) must not prevent a grab from matching.
const unsigned int kLockVariants[] = {0, LockMask, Mod2Mask, LockMask | Mod2Mask};

std::atomic<bool> g_grabFailed{false};

int grabErrorHandler(Display *, XErrorEvent *error)
{
    if (error->error_code == BadAccess)
        g_grabFailed.store(true);
    return 0;
}

} // namespace

int x11KeysymToQtKey(unsigned long keysym, bool *keypad)
{
    *keypad = false;
    if (keysym >= XK_a && keysym <= XK_z)
        return Qt::Key_A + int(keysym - XK_a);
    if (keysym >= XK_A && keysym <= XK_Z)
        return Qt::Key_A + int(keysym - XK_A);
    if (keysym >= XK_0 && keysym <= XK_9)
        return Qt::Key_0 + int(keysym - XK_0);
    if (keysym >= XK_F1 && keysym <= XK_F35)
        return Qt::Key_F1 + int(keysym - XK_F1);
    for (const KeyPair &p : kKeyTable) {
        if (p.keysym == keysym) {
            *keypad = p.keypad;
            return p.key;
        }
    }
    return 0;
}

unsigned long qtKeyToX11Keysym(int key, bool keypad)
{
    if (!keypad) {
        if (key >= Qt::Key_A && key <= Qt::Key_Z)
            return XK_a + (key - Qt::Key_A);
        if (key >= Qt::Key_0 && key <= Qt::Key_9)
            return XK_0 + (key - Qt::Key_0);
        if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
            return XK_F1 + (key - Qt::Key_F1);
    }
    for (const KeyPair &p : kKeyTable) {
        if (p.key == key && p.keypad == keypad)
            return p.keysym;
    }
    return NoSymbol;
}

// ---------------------------------------------------------------- hook

struct X11InputHook::Impl {
    SyntheticTracker *tracker = nullptr;
    InputEventSink sink;
    KeySuppressionRegistry *suppression = nullptr;
    Display *control = nullptr; // XRecord control connection
    Display *data = nullptr;    // XRecord data connection
    Display *grab = nullptr;    // key grabs + keymap lookups
    XRecordContext context = 0;
    int wakePipe[2] = {-1, -1};
    QSet<quint32> heldKeys;     // keycodes currently down (auto-repeat)

    // Commands executed on the hook thread (Xlib connections are single-threaded).
    QMutex mutex;
    std::deque<std::function<void()>> commands;
    bool stopRequested = false;

    QSet<KeyCombo> grabbed;     // combos that must be grabbed (survive restarts)
    std::atomic<int> *regrabs = nullptr;

    void post(std::function<void()> command)
    {
        {
            QMutexLocker lock(&mutex);
            commands.push_back(std::move(command));
        }
        const char byte = 1;
        if (wakePipe[1] >= 0)
            (void)::write(wakePipe[1], &byte, 1);
    }

    PlatformResult grabCombo(const KeyCombo &combo, bool grabIt)
    {
        const unsigned long keysym = qtKeyToX11Keysym(combo.key, combo.keypad);
        const KeyCode keycode = keysym == NoSymbol ? 0 : XKeysymToKeycode(grab, keysym);
        if (keycode == 0)
            return PlatformResult::failed(tr("The key %1 does not exist on this keyboard.").arg(combo.toString()));
        const Window root = DefaultRootWindow(grab);
        const unsigned int mods = stateFromModifiers(combo.modifiers);

        XSync(grab, False);
        g_grabFailed.store(false);
        XErrorHandler previous = XSetErrorHandler(grabErrorHandler);
        for (unsigned int variant : kLockVariants) {
            if (grabIt)
                XGrabKey(grab, keycode, mods | variant, root, False, GrabModeAsync, GrabModeAsync);
            else
                XUngrabKey(grab, keycode, mods | variant, root);
        }
        XSync(grab, False);
        XSetErrorHandler(previous);

        if (grabIt && g_grabFailed.load()) {
            for (unsigned int variant : kLockVariants)
                XUngrabKey(grab, keycode, mods | variant, root);
            XSync(grab, False);
            return PlatformResult::failed(
                tr("%1 is already used by another application (X11 grab refused).").arg(combo.toString()));
        }
        return PlatformResult::success();
    }

    void regrabAll()
    {
        for (const KeyCombo &combo : std::as_const(grabbed)) {
            grabCombo(combo, false);
            grabCombo(combo, true);
        }
    }

    static void recordCallback(XPointer closure, XRecordInterceptData *recorded);
    void handleRecorded(const xEvent *event);
};

void X11InputHook::Impl::recordCallback(XPointer closure, XRecordInterceptData *recorded)
{
    auto *self = reinterpret_cast<Impl *>(closure);
    if (recorded->category == XRecordFromServer && recorded->data)
        self->handleRecorded(reinterpret_cast<const xEvent *>(recorded->data));
    XRecordFreeData(recorded);
}

void X11InputHook::Impl::handleRecorded(const xEvent *event)
{
    const int type = event->u.u.type & 0x7F;
    const unsigned int detail = event->u.u.detail;
    InputEvent e;
    e.timestampMs = monotonicNowMs();
    // Root coordinates are device pixels; without Qt display scaling they equal
    // Qt's logical pixels.
    e.position = QPointF(event->u.keyButtonPointer.rootX, event->u.keyButtonPointer.rootY);
    e.modifiers = modifiersFromState(event->u.keyButtonPointer.state);

    switch (type) {
    case MotionNotify:
        e.type = InputEventType::MouseMove;
        break;
    case ButtonPress:
    case ButtonRelease: {
        const bool press = type == ButtonPress;
        if (detail >= 4 && detail <= 7) {
            if (!press)
                return; // wheel "buttons" send press+release; report the press only
            e.type = InputEventType::Wheel;
            if (detail == 4) e.wheelDeltaY = 120;
            if (detail == 5) e.wheelDeltaY = -120;
            if (detail == 6) e.wheelDeltaX = -120;
            if (detail == 7) e.wheelDeltaX = 120;
            break;
        }
        e.type = press ? InputEventType::MouseDown : InputEventType::MouseUp;
        switch (detail) {
        case 1: e.button = MouseButtonId::Left; break;
        case 2: e.button = MouseButtonId::Middle; break;
        case 3: e.button = MouseButtonId::Right; break;
        case 8: e.button = MouseButtonId::Back; break;
        case 9: e.button = MouseButtonId::Forward; break;
        default: e.button = MouseButtonId::Other; break;
        }
        e.synthetic = tracker && tracker->consume(e.timestampMs);
        break;
    }
    case KeyPress:
    case KeyRelease: {
        const bool press = type == KeyPress;
        e.type = press ? InputEventType::KeyDown : InputEventType::KeyUp;
        e.nativeKeyCode = detail;
        e.key = x11KeysymToQtKey(XkbKeycodeToKeysym(grab, KeyCode(detail), 0, 0), &e.keypad);
        if (press) {
            e.autoRepeat = heldKeys.contains(detail);
            heldKeys.insert(detail);
        } else {
            heldKeys.remove(detail);
        }
        // Grabbed keys never reach other applications; mark them for subscribers.
        if (suppression && suppression->shouldSwallow(e))
            e.swallowed = true;
        break;
    }
    default:
        return;
    }
    sink(e);
}

X11InputHook::X11InputHook(SyntheticTracker *tracker)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->tracker = tracker;
    m_impl->regrabs = &m_regrabs;
}

X11InputHook::~X11InputHook()
{
    stop();
}

PlatformResult X11InputHook::start(InputEventSink sink, KeySuppressionRegistry *suppression)
{
    if (m_running.load())
        return PlatformResult::success();
    if (m_thread.joinable())
        m_thread.join();

    Impl *s = m_impl.get();
    s->sink = std::move(sink);
    s->suppression = suppression;
    s->stopRequested = false;
    s->heldKeys.clear();

    std::promise<PlatformResult> started;
    std::future<PlatformResult> result = started.get_future();

    m_thread = std::thread([this, s, &started] {
        s->control = XOpenDisplay(nullptr);
        s->data = XOpenDisplay(nullptr);
        s->grab = XOpenDisplay(nullptr);
        int major = 0, minor = 0;
        if (!s->control || !s->data || !s->grab || !XRecordQueryVersion(s->control, &major, &minor)) {
            started.set_value(PlatformResult::notSupported(
                tr("Cannot connect to the X11 display or the RECORD extension is missing.")));
            for (Display *d : {s->control, s->data, s->grab})
                if (d)
                    XCloseDisplay(d);
            s->control = s->data = s->grab = nullptr;
            return;
        }

        XRecordRange *range = XRecordAllocRange();
        range->device_events.first = KeyPress;
        range->device_events.last = MotionNotify;
        XRecordClientSpec clients = XRecordAllClients;
        s->context = XRecordCreateContext(s->control, 0, &clients, 1, &range, 1);
        XFree(range);
        XSync(s->control, False);
        if (!s->context || !XRecordEnableContextAsync(s->data, s->context, &Impl::recordCallback,
                                                      reinterpret_cast<XPointer>(s))) {
            started.set_value(PlatformResult::failed(tr("Cannot start X11 event recording.")));
            return;
        }
        // Receive MappingNotify on the grab connection (re-grab after keymap changes).
        XSelectInput(s->grab, DefaultRootWindow(s->grab), 0);
        if (::pipe(s->wakePipe) != 0) {
            started.set_value(PlatformResult::failed(tr("Cannot create the hook wake-up pipe.")));
            return;
        }
        fcntl(s->wakePipe[0], F_SETFL, O_NONBLOCK);
        s->regrabAll(); // keys registered before (re)start
        m_running.store(true);
        started.set_value(PlatformResult::success());

        pollfd fds[3] = {{ConnectionNumber(s->data), POLLIN, 0},
                         {ConnectionNumber(s->grab), POLLIN, 0},
                         {s->wakePipe[0], POLLIN, 0}};
        while (true) {
            XRecordProcessReplies(s->data);
            while (XPending(s->grab) > 0) {
                XEvent ev;
                XNextEvent(s->grab, &ev);
                if (ev.type == MappingNotify) {
                    XRefreshKeyboardMapping(&ev.xmapping);
                    s->regrabAll();
                    s->regrabs->fetch_add(1);
                }
            }
            std::deque<std::function<void()>> pending;
            bool stop = false;
            {
                QMutexLocker lock(&s->mutex);
                pending.swap(s->commands);
                stop = s->stopRequested;
            }
            for (auto &command : pending)
                command();
            if (stop || s->stopRequested)
                break;
            if (::poll(fds, 3, 500) > 0 && (fds[2].revents & POLLIN)) {
                char buffer[64];
                while (::read(s->wakePipe[0], buffer, sizeof buffer) > 0) {
                }
            }
        }

        XRecordDisableContext(s->control, s->context);
        XRecordFreeContext(s->control, s->context);
        XSync(s->control, False);
        for (const KeyCombo &combo : std::as_const(s->grabbed))
            s->grabCombo(combo, false);
        for (Display *d : {s->data, s->grab, s->control})
            XCloseDisplay(d);
        s->control = s->data = s->grab = nullptr;
        s->context = 0;
        ::close(s->wakePipe[0]);
        ::close(s->wakePipe[1]);
        s->wakePipe[0] = s->wakePipe[1] = -1;
        m_running.store(false);
    });

    const PlatformResult r = result.get();
    if (!r.ok()) {
        m_thread.join();
        qCWarning(lcPlatform).noquote() << "Input hook not started:" << r.message;
        return r;
    }
    qCInfo(lcPlatform).noquote() << "Input hook started:" << backendName();
    return r;
}

void X11InputHook::stop()
{
    if (m_running.load())
        m_impl->post([s = m_impl.get()] { s->stopRequested = true; }); // handled on the hook thread
    if (m_thread.joinable())
        m_thread.join();
    m_running.store(false);
}

bool X11InputHook::checkHealth()
{
    if (!m_running.load())
        return false;
    // A round trip on the hook thread proves the loop and the connections work.
    auto done = std::make_shared<std::promise<bool>>();
    std::future<bool> alive = done->get_future();
    m_impl->post([s = m_impl.get(), done] {
        XSync(s->grab, False);
        done->set_value(true);
    });
    return alive.wait_for(std::chrono::seconds(2)) == std::future_status::ready && alive.get();
}

PlatformResult X11InputHook::registerKey(const KeyCombo &combo)
{
    if (!combo.isValid())
        return PlatformResult::failed(tr("Invalid key combination."));
    if (!m_running.load()) {
        m_impl->grabbed.insert(combo); // applied when the hook starts
        return PlatformResult::success();
    }
    auto done = std::make_shared<std::promise<PlatformResult>>();
    std::future<PlatformResult> result = done->get_future();
    m_impl->post([s = m_impl.get(), combo, done] {
        PlatformResult r = s->grabCombo(combo, true);
        if (r.ok())
            s->grabbed.insert(combo);
        done->set_value(r);
    });
    if (result.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
        return PlatformResult::failed(tr("The input hook did not respond."));
    return result.get();
}

PlatformResult X11InputHook::unregisterKey(const KeyCombo &combo)
{
    if (!m_running.load()) {
        m_impl->grabbed.remove(combo);
        return PlatformResult::success();
    }
    auto done = std::make_shared<std::promise<PlatformResult>>();
    std::future<PlatformResult> result = done->get_future();
    m_impl->post([s = m_impl.get(), combo, done] {
        s->grabbed.remove(combo);
        done->set_value(s->grabCombo(combo, false));
    });
    if (result.wait_for(std::chrono::seconds(2)) != std::future_status::ready)
        return PlatformResult::failed(tr("The input hook did not respond."));
    return result.get();
}

// ---------------------------------------------------------------- injector

X11ClickInjector::~X11ClickInjector()
{
    if (m_display)
        XCloseDisplay(static_cast<Display *>(m_display));
}

PlatformResult X11ClickInjector::send(MouseButton button, std::initializer_list<bool> presses)
{
    QMutexLocker lock(&m_mutex);
    if (!m_display)
        m_display = XOpenDisplay(nullptr);
    auto *display = static_cast<Display *>(m_display);
    int event = 0, error = 0, major = 0, minor = 0;
    if (!display || !XTestQueryExtension(display, &event, &error, &major, &minor))
        return PlatformResult::notSupported(tr("The X11 XTEST extension is not available."));
    const unsigned int number = button == MouseButton::Right ? 3 : button == MouseButton::Middle ? 2 : 1;
    m_tracker->expect(int(presses.size()), monotonicNowMs());
    for (bool press : presses)
        XTestFakeButtonEvent(display, number, press ? True : False, CurrentTime);
    XFlush(display);
    return PlatformResult::success();
}

PlatformResult X11ClickInjector::click(MouseButton button)
{
    return send(button, {true, false});
}

PlatformResult X11ClickInjector::doubleClick(MouseButton button)
{
    return send(button, {true, false, true, false});
}

PlatformResult X11ClickInjector::press(MouseButton button)
{
    return send(button, {true});
}

PlatformResult X11ClickInjector::release(MouseButton button)
{
    return send(button, {false});
}

} // namespace ergo

// ---------------------------------------------------------------- no libevdev

#ifndef ERGOPRO2_HAVE_LIBEVDEV
namespace ergo {

// Built without libevdev: Wayland input is reported as unavailable.
std::unique_ptr<IInputHook> createEvdevInputHook()
{
    return nullptr;
}

std::unique_ptr<IClickInjector> createUinputClickInjector()
{
    return nullptr;
}

int evdevKeyToQtKey(unsigned int, bool *keypad)
{
    *keypad = false;
    return 0;
}

} // namespace ergo
#endif
