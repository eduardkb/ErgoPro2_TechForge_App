#include "platform/win/WinInput.h"

#include "core/Clock.h"
#include "core/LogCategories.h"
#include "platform/common/Synthetic.h"

#include <QCoreApplication>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellscalingapi.h> // GetDpiForMonitor
#include <wtsapi32.h>

#include <bitset>
#include <future>

namespace ergo {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::WinInput", text);
}

constexpr UINT kRearmMessage = WM_APP + 1;

// State shared with the hook procedures. Only one hook instance exists; it is
// accessed from the hook thread only (except the pointers set before start).
struct HookState {
    InputEventSink sink;
    KeySuppressionRegistry *suppression = nullptr;
    HHOOK keyboard = nullptr;
    HHOOK mouse = nullptr;
    std::bitset<256> held; // virtual keys currently down (auto-repeat, modifiers)
};
HookState *g_hook = nullptr;

Modifiers heldModifiers(const std::bitset<256> &held)
{
    Modifiers m = Modifier::None;
    if (held.test(VK_LSHIFT) || held.test(VK_RSHIFT) || held.test(VK_SHIFT))
        m |= Modifier::Shift;
    if (held.test(VK_LCONTROL) || held.test(VK_RCONTROL) || held.test(VK_CONTROL))
        m |= Modifier::Control;
    if (held.test(VK_LMENU) || held.test(VK_RMENU) || held.test(VK_MENU))
        m |= Modifier::Alt;
    if (held.test(VK_LWIN) || held.test(VK_RWIN))
        m |= Modifier::Meta;
    return m;
}

QPointF toLogical(POINT pt)
{
    // Hook coordinates are physical pixels; Qt uses logical pixels (per-monitor DPI).
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    UINT dpiX = 96, dpiY = 96;
    if (!monitor || FAILED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) || dpiX == 0)
        dpiX = 96;
    const double scale = double(dpiX) / 96.0;
    return QPointF(pt.x / scale, pt.y / scale);
}

// Hook procedures run on the hook thread and must return quickly.
LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code != HC_ACTION || !g_hook)
        return CallNextHookEx(nullptr, code, wParam, lParam);

    const auto *k = reinterpret_cast<const KBDLLHOOKSTRUCT *>(lParam);
    const bool down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
    const unsigned int vk = k->vkCode & 0xFF;

    InputEvent e;
    e.type = down ? InputEventType::KeyDown : InputEventType::KeyUp;
    e.timestampMs = monotonicNowMs();
    e.nativeKeyCode = vk;
    e.key = winVirtualKeyToQtKey(vk, (k->flags & LLKHF_EXTENDED) != 0, &e.keypad);
    e.synthetic = k->dwExtraInfo == ULONG_PTR(kSyntheticEventMarker);
    if (down) {
        e.autoRepeat = g_hook->held.test(vk); // low-level hooks do not flag repeats
        g_hook->held.set(vk);
    } else {
        g_hook->held.reset(vk);
    }
    e.modifiers = heldModifiers(g_hook->held);

    if (g_hook->suppression && g_hook->suppression->shouldSwallow(e))
        e.swallowed = true;
    g_hook->sink(e);
    if (e.swallowed)
        return 1; // non-zero = the key never reaches other applications
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

LRESULT CALLBACK mouseProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code != HC_ACTION || !g_hook)
        return CallNextHookEx(nullptr, code, wParam, lParam);

    const auto *m = reinterpret_cast<const MSLLHOOKSTRUCT *>(lParam);
    InputEvent e;
    e.timestampMs = monotonicNowMs();
    e.position = toLogical(m->pt);
    e.synthetic = m->dwExtraInfo == ULONG_PTR(kSyntheticEventMarker);
    e.modifiers = heldModifiers(g_hook->held);
    const short wheel = short(HIWORD(m->mouseData));

    switch (wParam) {
    case WM_MOUSEMOVE: e.type = InputEventType::MouseMove; break;
    case WM_LBUTTONDOWN: e.type = InputEventType::MouseDown; e.button = MouseButtonId::Left; break;
    case WM_LBUTTONUP: e.type = InputEventType::MouseUp; e.button = MouseButtonId::Left; break;
    case WM_RBUTTONDOWN: e.type = InputEventType::MouseDown; e.button = MouseButtonId::Right; break;
    case WM_RBUTTONUP: e.type = InputEventType::MouseUp; e.button = MouseButtonId::Right; break;
    case WM_MBUTTONDOWN: e.type = InputEventType::MouseDown; e.button = MouseButtonId::Middle; break;
    case WM_MBUTTONUP: e.type = InputEventType::MouseUp; e.button = MouseButtonId::Middle; break;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        e.type = wParam == WM_XBUTTONDOWN ? InputEventType::MouseDown : InputEventType::MouseUp;
        e.button = HIWORD(m->mouseData) == XBUTTON1 ? MouseButtonId::Back : MouseButtonId::Forward;
        break;
    case WM_MOUSEWHEEL: e.type = InputEventType::Wheel; e.wheelDeltaY = wheel; break;
    case WM_MOUSEHWHEEL: e.type = InputEventType::Wheel; e.wheelDeltaX = wheel; break;
    default: return CallNextHookEx(nullptr, code, wParam, lParam);
    }
    g_hook->sink(e);
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

bool installHooks(HookState *s)
{
    HINSTANCE module = GetModuleHandleW(nullptr);
    s->keyboard = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, module, 0);
    s->mouse = SetWindowsHookExW(WH_MOUSE_LL, mouseProc, module, 0);
    return s->keyboard && s->mouse;
}

void removeHooks(HookState *s)
{
    if (s->keyboard)
        UnhookWindowsHookEx(s->keyboard);
    if (s->mouse)
        UnhookWindowsHookEx(s->mouse);
    s->keyboard = nullptr;
    s->mouse = nullptr;
}

} // namespace

// ---------------------------------------------------------------- key table

int winVirtualKeyToQtKey(unsigned int vk, bool extended, bool *keypad)
{
    *keypad = false;
    if (vk >= 'A' && vk <= 'Z')
        return Qt::Key_A + int(vk - 'A');
    if (vk >= '0' && vk <= '9')
        return Qt::Key_0 + int(vk - '0');
    if (vk >= VK_F1 && vk <= VK_F24)
        return Qt::Key_F1 + int(vk - VK_F1);
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        *keypad = true;
        return Qt::Key_0 + int(vk - VK_NUMPAD0);
    }
    switch (vk) {
    case VK_BACK: return Qt::Key_Backspace;
    case VK_TAB: return Qt::Key_Tab;
    case VK_RETURN:
        *keypad = extended; // the keypad Enter key is "extended"
        return extended ? Qt::Key_Enter : Qt::Key_Return;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT: return Qt::Key_Shift;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL: return Qt::Key_Control;
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU: return Qt::Key_Alt;
    case VK_LWIN:
    case VK_RWIN: return Qt::Key_Meta;
    case VK_PAUSE: return Qt::Key_Pause;
    case VK_CAPITAL: return Qt::Key_CapsLock;
    case VK_ESCAPE: return Qt::Key_Escape;
    case VK_SPACE: return Qt::Key_Space;
    case VK_PRIOR: return Qt::Key_PageUp;
    case VK_NEXT: return Qt::Key_PageDown;
    case VK_END: return Qt::Key_End;
    case VK_HOME: return Qt::Key_Home;
    case VK_LEFT: return Qt::Key_Left;
    case VK_UP: return Qt::Key_Up;
    case VK_RIGHT: return Qt::Key_Right;
    case VK_DOWN: return Qt::Key_Down;
    case VK_SNAPSHOT: return Qt::Key_Print;
    case VK_INSERT: return Qt::Key_Insert;
    case VK_DELETE: return Qt::Key_Delete;
    case VK_APPS: return Qt::Key_Menu;
    case VK_NUMLOCK: return Qt::Key_NumLock;
    case VK_SCROLL: return Qt::Key_ScrollLock;
    case VK_MULTIPLY: *keypad = true; return Qt::Key_Asterisk;
    case VK_ADD: *keypad = true; return Qt::Key_Plus;
    case VK_SUBTRACT: *keypad = true; return Qt::Key_Minus;
    case VK_DECIMAL: *keypad = true; return Qt::Key_Period;
    case VK_DIVIDE: *keypad = true; return Qt::Key_Slash;
    case VK_OEM_1: return Qt::Key_Semicolon;
    case VK_OEM_PLUS: return Qt::Key_Equal;
    case VK_OEM_COMMA: return Qt::Key_Comma;
    case VK_OEM_MINUS: return Qt::Key_Minus;
    case VK_OEM_PERIOD: return Qt::Key_Period;
    case VK_OEM_2: return Qt::Key_Slash;
    case VK_OEM_3: return Qt::Key_QuoteLeft;
    case VK_OEM_4: return Qt::Key_BracketLeft;
    case VK_OEM_5: return Qt::Key_Backslash;
    case VK_OEM_6: return Qt::Key_BracketRight;
    case VK_OEM_7: return Qt::Key_Apostrophe;
    default: return 0;
    }
}

// ---------------------------------------------------------------- hook

WinInputHook::~WinInputHook()
{
    stop();
}

PlatformResult WinInputHook::start(InputEventSink sink, KeySuppressionRegistry *suppression)
{
    if (m_running.load())
        return PlatformResult::success();
    if (m_thread.joinable())
        m_thread.join();

    std::promise<PlatformResult> started;
    std::future<PlatformResult> result = started.get_future();

    m_thread = std::thread([this, &started, sink = std::move(sink), suppression]() mutable {
        HookState state;
        state.sink = std::move(sink);
        state.suppression = suppression;
        g_hook = &state;
        m_threadId.store(GetCurrentThreadId());

        // Make sure the thread has a message queue before anyone posts to it.
        MSG msg;
        PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

        if (!installHooks(&state)) {
            const DWORD error = GetLastError();
            removeHooks(&state);
            g_hook = nullptr;
            started.set_value(PlatformResult::failed(
                tr("Windows refused to install the keyboard/mouse hooks (error %1).").arg(error)));
            return;
        }
        m_running.store(true);
        started.set_value(PlatformResult::success());

        // The low-level hooks are called through this thread's message loop.
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            if (msg.message == kRearmMessage) {
                removeHooks(&state);
                const bool ok = installHooks(&state);
                auto *done = reinterpret_cast<std::promise<bool> *>(msg.lParam);
                done->set_value(ok);
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        removeHooks(&state);
        g_hook = nullptr;
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

void WinInputHook::stop()
{
    if (const unsigned long id = m_threadId.load())
        PostThreadMessageW(id, WM_QUIT, 0, 0);
    if (m_thread.joinable())
        m_thread.join();
    m_threadId.store(0);
    m_running.store(false);
}

bool WinInputHook::checkHealth()
{
    const unsigned long id = m_threadId.load();
    if (!m_running.load() || id == 0)
        return false;
    std::promise<bool> done;
    std::future<bool> ok = done.get_future();
    if (!PostThreadMessageW(id, kRearmMessage, 0, reinterpret_cast<LPARAM>(&done)))
        return false;
    if (ok.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        // The hook thread does not process messages: it must be reinstalled. The
        // promise must outlive a late reply, so wait for it before returning.
        ok.wait();
        return false;
    }
    if (!ok.get())
        return false;
    m_rearms.fetch_add(1);
    return true;
}

PlatformResult WinInputHook::registerKey(const KeyCombo &combo)
{
    // Keys are blocked inside the low-level keyboard hook.
    return combo.isValid() ? PlatformResult::success() : PlatformResult::failed(tr("Invalid key combination."));
}

PlatformResult WinInputHook::unregisterKey(const KeyCombo &)
{
    return PlatformResult::success();
}

// ---------------------------------------------------------------- injector

namespace {

struct ButtonFlags {
    DWORD down;
    DWORD up;
};

ButtonFlags flagsFor(MouseButton button)
{
    switch (button) {
    case MouseButton::Right: return {MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP};
    case MouseButton::Middle: return {MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP};
    case MouseButton::Left: break;
    }
    return {MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP};
}

PlatformResult send(std::initializer_list<DWORD> flags)
{
    INPUT inputs[4] = {};
    UINT count = 0;
    for (DWORD f : flags) {
        inputs[count].type = INPUT_MOUSE;
        inputs[count].mi.dwFlags = f; // no MOVE flag: at the current pointer position
        inputs[count].mi.dwExtraInfo = ULONG_PTR(kSyntheticEventMarker);
        ++count;
    }
    if (SendInput(count, inputs, sizeof(INPUT)) != count) {
        return PlatformResult::failed(
            tr("Windows blocked the click (error %1). Clicks cannot reach programs running as Administrator "
               "unless ErgoPro2 also runs as Administrator.")
                .arg(GetLastError()));
    }
    return PlatformResult::success();
}

} // namespace

PlatformResult WinClickInjector::click(MouseButton button)
{
    const ButtonFlags f = flagsFor(button);
    return send({f.down, f.up});
}

PlatformResult WinClickInjector::doubleClick(MouseButton button)
{
    // Two clicks sent immediately are within the double-click time.
    const ButtonFlags f = flagsFor(button);
    return send({f.down, f.up, f.down, f.up});
}

PlatformResult WinClickInjector::press(MouseButton button)
{
    return send({flagsFor(button).down});
}

PlatformResult WinClickInjector::release(MouseButton button)
{
    return send({flagsFor(button).up});
}

int WinClickInjector::doubleClickIntervalMs() const
{
    return int(GetDoubleClickTime());
}

// ---------------------------------------------------------------- power events

namespace {

std::function<void(PowerEvent)> g_powerCallback;

LRESULT CALLBACK powerWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_POWERBROADCAST:
        if (g_powerCallback) {
            if (wParam == PBT_APMSUSPEND)
                g_powerCallback(PowerEvent::Sleep);
            else if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND)
                g_powerCallback(PowerEvent::Resume);
        }
        return TRUE;
    case WM_WTSSESSION_CHANGE:
        if (g_powerCallback) {
            if (wParam == WTS_SESSION_LOCK)
                g_powerCallback(PowerEvent::ScreenLocked);
            else if (wParam == WTS_SESSION_UNLOCK)
                g_powerCallback(PowerEvent::ScreenUnlocked);
        }
        return 0;
    case WM_CLOSE:
        WTSUnRegisterSessionNotification(hwnd);
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

} // namespace

WinPowerEvents::~WinPowerEvents()
{
    stop();
}

PlatformResult WinPowerEvents::start(std::function<void(PowerEvent)> callback)
{
    if (m_window.load())
        return PlatformResult::success();
    g_powerCallback = std::move(callback);

    std::promise<PlatformResult> started;
    std::future<PlatformResult> result = started.get_future();
    m_thread = std::thread([this, &started] {
        HINSTANCE module = GetModuleHandleW(nullptr);
        WNDCLASSW wc = {};
        wc.lpfnWndProc = powerWindowProc;
        wc.hInstance = module;
        wc.lpszClassName = L"ErgoPro2PowerEvents";
        RegisterClassW(&wc);
        // A hidden top-level window: message-only windows do not get broadcasts.
        HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0, nullptr, nullptr, module, nullptr);
        if (!hwnd) {
            started.set_value(PlatformResult::failed(
                tr("Cannot create the power notification window (error %1).").arg(GetLastError())));
            return;
        }
        WTSRegisterSessionNotification(hwnd, NOTIFY_FOR_THIS_SESSION);
        m_window.store(hwnd);
        started.set_value(PlatformResult::success());
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        m_window.store(nullptr);
    });
    const PlatformResult r = result.get();
    if (!r.ok()) {
        m_thread.join();
        qCWarning(lcPlatform).noquote() << r.message;
        return r;
    }
    qCInfo(lcPlatform) << "Listening for sleep/resume and session lock notifications";
    return r;
}

void WinPowerEvents::stop()
{
    if (void *hwnd = m_window.load())
        PostMessageW(static_cast<HWND>(hwnd), WM_CLOSE, 0, 0);
    if (m_thread.joinable())
        m_thread.join();
}

} // namespace ergo
