// Wayland (best effort): reads /dev/input/event* with libevdev and injects
// through uinput. Compiled only when libevdev was found (ERGOPRO2_HAVE_LIBEVDEV).
//
// - Pointer position: Wayland exposes no global position, so positions are the
//   accumulated relative motion (InputEvent::positionKnown = false).
// - Blocking keys: while at least one combo is registered, all keyboards are
//   grabbed (EVIOCGRAB) and every key that is NOT blocked is re-sent through a
//   virtual uinput keyboard.
// - Synthetic events: injected clicks come from ErgoPro2's own virtual mouse
//   and are reported with synthetic = true.

#include "platform/linux/LinuxInput.h"

#include "core/Clock.h"
#include "core/LogCategories.h"

#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QMutexLocker>

#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <future>
#include <vector>

namespace ergo {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ergo::LinuxEvdevInput", text);
}

const char kVirtualKeyboardName[] = "ErgoPro2 virtual keyboard";
const char kVirtualMouseName[] = "ErgoPro2 virtual mouse";

struct Device {
    int fd = -1;
    libevdev *dev = nullptr;
    QString path;
    bool keyboard = false;
    bool pointer = false;
    bool ours = false; // ErgoPro2's own virtual mouse -> synthetic
    bool grabbed = false;
};

} // namespace

int evdevKeyToQtKey(unsigned int code, bool *keypad)
{
    *keypad = false;
    switch (code) {
    case KEY_A: return Qt::Key_A; case KEY_B: return Qt::Key_B; case KEY_C: return Qt::Key_C;
    case KEY_D: return Qt::Key_D; case KEY_E: return Qt::Key_E; case KEY_F: return Qt::Key_F;
    case KEY_G: return Qt::Key_G; case KEY_H: return Qt::Key_H; case KEY_I: return Qt::Key_I;
    case KEY_J: return Qt::Key_J; case KEY_K: return Qt::Key_K; case KEY_L: return Qt::Key_L;
    case KEY_M: return Qt::Key_M; case KEY_N: return Qt::Key_N; case KEY_O: return Qt::Key_O;
    case KEY_P: return Qt::Key_P; case KEY_Q: return Qt::Key_Q; case KEY_R: return Qt::Key_R;
    case KEY_S: return Qt::Key_S; case KEY_T: return Qt::Key_T; case KEY_U: return Qt::Key_U;
    case KEY_V: return Qt::Key_V; case KEY_W: return Qt::Key_W; case KEY_X: return Qt::Key_X;
    case KEY_Y: return Qt::Key_Y; case KEY_Z: return Qt::Key_Z;
    case KEY_1: return Qt::Key_1; case KEY_2: return Qt::Key_2; case KEY_3: return Qt::Key_3;
    case KEY_4: return Qt::Key_4; case KEY_5: return Qt::Key_5; case KEY_6: return Qt::Key_6;
    case KEY_7: return Qt::Key_7; case KEY_8: return Qt::Key_8; case KEY_9: return Qt::Key_9;
    case KEY_0: return Qt::Key_0;
    case KEY_F1: return Qt::Key_F1; case KEY_F2: return Qt::Key_F2; case KEY_F3: return Qt::Key_F3;
    case KEY_F4: return Qt::Key_F4; case KEY_F5: return Qt::Key_F5; case KEY_F6: return Qt::Key_F6;
    case KEY_F7: return Qt::Key_F7; case KEY_F8: return Qt::Key_F8; case KEY_F9: return Qt::Key_F9;
    case KEY_F10: return Qt::Key_F10; case KEY_F11: return Qt::Key_F11; case KEY_F12: return Qt::Key_F12;
    case KEY_F13: return Qt::Key_F13; case KEY_F14: return Qt::Key_F14; case KEY_F15: return Qt::Key_F15;
    case KEY_F16: return Qt::Key_F16; case KEY_F17: return Qt::Key_F17; case KEY_F18: return Qt::Key_F18;
    case KEY_F19: return Qt::Key_F19; case KEY_F20: return Qt::Key_F20; case KEY_F21: return Qt::Key_F21;
    case KEY_F22: return Qt::Key_F22; case KEY_F23: return Qt::Key_F23; case KEY_F24: return Qt::Key_F24;
    case KEY_ESC: return Qt::Key_Escape; case KEY_TAB: return Qt::Key_Tab;
    case KEY_BACKSPACE: return Qt::Key_Backspace; case KEY_ENTER: return Qt::Key_Return;
    case KEY_SPACE: return Qt::Key_Space; case KEY_INSERT: return Qt::Key_Insert;
    case KEY_DELETE: return Qt::Key_Delete; case KEY_HOME: return Qt::Key_Home; case KEY_END: return Qt::Key_End;
    case KEY_PAGEUP: return Qt::Key_PageUp; case KEY_PAGEDOWN: return Qt::Key_PageDown;
    case KEY_LEFT: return Qt::Key_Left; case KEY_RIGHT: return Qt::Key_Right;
    case KEY_UP: return Qt::Key_Up; case KEY_DOWN: return Qt::Key_Down;
    case KEY_LEFTSHIFT: case KEY_RIGHTSHIFT: return Qt::Key_Shift;
    case KEY_LEFTCTRL: case KEY_RIGHTCTRL: return Qt::Key_Control;
    case KEY_LEFTALT: return Qt::Key_Alt; case KEY_RIGHTALT: return Qt::Key_AltGr;
    case KEY_LEFTMETA: case KEY_RIGHTMETA: return Qt::Key_Meta;
    case KEY_CAPSLOCK: return Qt::Key_CapsLock; case KEY_NUMLOCK: return Qt::Key_NumLock;
    case KEY_SCROLLLOCK: return Qt::Key_ScrollLock; case KEY_PAUSE: return Qt::Key_Pause;
    case KEY_SYSRQ: return Qt::Key_Print; case KEY_COMPOSE: return Qt::Key_Menu;
    case KEY_MINUS: return Qt::Key_Minus; case KEY_EQUAL: return Qt::Key_Equal;
    case KEY_LEFTBRACE: return Qt::Key_BracketLeft; case KEY_RIGHTBRACE: return Qt::Key_BracketRight;
    case KEY_BACKSLASH: return Qt::Key_Backslash; case KEY_SEMICOLON: return Qt::Key_Semicolon;
    case KEY_APOSTROPHE: return Qt::Key_Apostrophe; case KEY_COMMA: return Qt::Key_Comma;
    case KEY_DOT: return Qt::Key_Period; case KEY_SLASH: return Qt::Key_Slash; case KEY_GRAVE: return Qt::Key_QuoteLeft;
    default: break;
    }
    *keypad = true;
    switch (code) {
    case KEY_KP0: return Qt::Key_0; case KEY_KP1: return Qt::Key_1; case KEY_KP2: return Qt::Key_2;
    case KEY_KP3: return Qt::Key_3; case KEY_KP4: return Qt::Key_4; case KEY_KP5: return Qt::Key_5;
    case KEY_KP6: return Qt::Key_6; case KEY_KP7: return Qt::Key_7; case KEY_KP8: return Qt::Key_8;
    case KEY_KP9: return Qt::Key_9; case KEY_KPDOT: return Qt::Key_Period; case KEY_KPPLUS: return Qt::Key_Plus;
    case KEY_KPMINUS: return Qt::Key_Minus; case KEY_KPASTERISK: return Qt::Key_Asterisk;
    case KEY_KPSLASH: return Qt::Key_Slash; case KEY_KPENTER: return Qt::Key_Enter;
    default: break;
    }
    *keypad = false;
    return 0;
}

namespace {

class EvdevInputHook final : public IInputHook {
public:
    ~EvdevInputHook() override { stop(); }

    PlatformResult start(InputEventSink sink, KeySuppressionRegistry *suppression) override
    {
        if (m_running.load())
            return PlatformResult::success();
        if (m_thread.joinable())
            m_thread.join();
        m_sink = std::move(sink);
        m_suppression = suppression;
        m_stop.store(false);

        std::promise<PlatformResult> started;
        std::future<PlatformResult> result = started.get_future();
        m_thread = std::thread([this, &started] { run(started); });
        const PlatformResult r = result.get();
        if (!r.ok()) {
            m_thread.join();
            qCWarning(lcPlatform).noquote() << "Input hook not started:" << r.message;
        } else {
            qCInfo(lcPlatform).noquote() << "Input hook started:" << backendName();
        }
        return r;
    }

    void stop() override
    {
        m_stop.store(true);
        if (m_wake[1] >= 0) {
            const char byte = 1;
            (void)::write(m_wake[1], &byte, 1);
        }
        if (m_thread.joinable())
            m_thread.join();
        m_running.store(false);
    }

    bool isRunning() const override { return m_running.load(); }
    bool canSuppressKeys() const override { return m_canForward.load(); }
    QString backendName() const override { return QStringLiteral("evdev (Wayland)"); }
    bool checkHealth() override { return m_running.load() && m_deviceCount.load() > 0; }
    int selfRepairCount() const override { return m_rescans.load(); }

    PlatformResult registerKey(const KeyCombo &combo) override
    {
        if (!combo.isValid())
            return PlatformResult::failed(tr("Invalid key combination."));
        if (m_running.load() && !m_canForward.load())
            return PlatformResult::permissionDenied(
                tr("Keys cannot be blocked: ErgoPro2 has no write access to /dev/uinput."));
        return PlatformResult::success(); // the hook grabs keyboards while combos exist
    }
    PlatformResult unregisterKey(const KeyCombo &) override { return PlatformResult::success(); }

private:
    void run(std::promise<PlatformResult> &started)
    {
        if (::pipe(m_wake) != 0) {
            started.set_value(PlatformResult::failed(tr("Cannot create the hook wake-up pipe.")));
            return;
        }
        fcntl(m_wake[0], F_SETFL, O_NONBLOCK);
        m_inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (m_inotify >= 0)
            inotify_add_watch(m_inotify, "/dev/input", IN_CREATE | IN_DELETE | IN_ATTRIB);

        scanDevices();
        if (m_devices.empty()) {
            closeAll();
            started.set_value(PlatformResult::permissionDenied(
                tr("No readable input device in /dev/input (the user must be in the 'input' group).")));
            return;
        }
        m_canForward.store(createVirtualKeyboard());
        m_running.store(true);
        started.set_value(PlatformResult::success());

        while (!m_stop.load()) {
            updateGrabs();
            std::vector<pollfd> fds;
            fds.push_back({m_wake[0], POLLIN, 0});
            if (m_inotify >= 0)
                fds.push_back({m_inotify, POLLIN, 0});
            for (const Device &d : m_devices)
                fds.push_back({d.fd, POLLIN, 0});
            if (::poll(fds.data(), nfds_t(fds.size()), 500) <= 0)
                continue;
            if (fds[0].revents & POLLIN) {
                char buffer[64];
                while (::read(m_wake[0], buffer, sizeof buffer) > 0) {
                }
            }
            if (m_inotify >= 0 && (fds[1].revents & POLLIN)) {
                char buffer[4096];
                while (::read(m_inotify, buffer, sizeof buffer) > 0) {
                }
                scanDevices(); // device hotplug
                m_rescans.fetch_add(1);
                continue;
            }
            for (Device &d : m_devices)
                readDevice(d);
        }
        closeAll();
        m_running.store(false);
    }

    void scanDevices()
    {
        QHash<QString, bool> present;
        const QDir dir(QStringLiteral("/dev/input"));
        for (const QString &name : dir.entryList({QStringLiteral("event*")}, QDir::System | QDir::Files)) {
            const QString path = dir.filePath(name);
            present.insert(path, true);
            bool known = false;
            for (const Device &d : m_devices)
                known = known || d.path == path;
            if (known)
                continue;
            const int fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
            if (fd < 0)
                continue; // no permission or vanished
            libevdev *dev = nullptr;
            if (libevdev_new_from_fd(fd, &dev) < 0) {
                ::close(fd);
                continue;
            }
            Device d;
            d.fd = fd;
            d.dev = dev;
            d.path = path;
            const QByteArray deviceName = libevdev_get_name(dev);
            if (deviceName == kVirtualKeyboardName) { // our own forwarding device: never read it
                libevdev_free(dev);
                ::close(fd);
                continue;
            }
            d.ours = deviceName == kVirtualMouseName;
            d.keyboard = libevdev_has_event_code(dev, EV_KEY, KEY_A) && libevdev_has_event_code(dev, EV_KEY, KEY_Z);
            d.pointer = libevdev_has_event_code(dev, EV_REL, REL_X) || libevdev_has_event_code(dev, EV_KEY, BTN_LEFT);
            if (!d.keyboard && !d.pointer) {
                libevdev_free(dev);
                ::close(fd);
                continue;
            }
            m_devices.push_back(d);
        }
        // Remove devices that disappeared.
        for (auto it = m_devices.begin(); it != m_devices.end();) {
            if (!present.contains(it->path)) {
                libevdev_free(it->dev);
                ::close(it->fd);
                it = m_devices.erase(it);
            } else {
                ++it;
            }
        }
        m_deviceCount.store(int(m_devices.size()));
    }

    bool createVirtualKeyboard()
    {
        libevdev *proto = libevdev_new();
        libevdev_set_name(proto, kVirtualKeyboardName);
        libevdev_enable_event_type(proto, EV_KEY);
        for (unsigned int code = KEY_ESC; code <= KEY_MICMUTE; ++code)
            libevdev_enable_event_code(proto, EV_KEY, code, nullptr);
        libevdev_enable_event_type(proto, EV_SYN);
        const int rc = libevdev_uinput_create_from_device(proto, LIBEVDEV_UINPUT_OPEN_MANAGED, &m_virtualKeyboard);
        libevdev_free(proto);
        if (rc < 0) {
            m_virtualKeyboard = nullptr;
            return false;
        }
        return true;
    }

    // Keyboards are grabbed only while at least one combo must be blocked.
    void updateGrabs()
    {
        const bool wantGrab = m_virtualKeyboard && m_suppression && !m_suppression->isEmpty();
        for (Device &d : m_devices) {
            if (!d.keyboard || d.ours || d.grabbed == wantGrab)
                continue;
            if (libevdev_grab(d.dev, wantGrab ? LIBEVDEV_GRAB : LIBEVDEV_UNGRAB) == 0)
                d.grabbed = wantGrab;
        }
    }

    void readDevice(Device &d)
    {
        input_event ev;
        int rc = libevdev_next_event(d.dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        while (rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == LIBEVDEV_READ_STATUS_SYNC) {
            handle(d, ev);
            const unsigned int flag = rc == LIBEVDEV_READ_STATUS_SYNC ? LIBEVDEV_READ_FLAG_SYNC : LIBEVDEV_READ_FLAG_NORMAL;
            rc = libevdev_next_event(d.dev, flag, &ev);
        }
    }

    void handle(Device &d, const input_event &ev)
    {
        const qint64 now = monotonicNowMs();
        if (ev.type == EV_REL) {
            if (ev.code == REL_X) { m_position.rx() += ev.value; m_moved = true; }
            else if (ev.code == REL_Y) { m_position.ry() += ev.value; m_moved = true; }
            else if (ev.code == REL_WHEEL || ev.code == REL_HWHEEL) {
                InputEvent e = base(d, now);
                e.type = InputEventType::Wheel;
                (ev.code == REL_WHEEL ? e.wheelDeltaY : e.wheelDeltaX) = ev.value * 120;
                m_sink(e);
            }
            return;
        }
        if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
            if (m_moved) {
                InputEvent e = base(d, now);
                e.type = InputEventType::MouseMove;
                m_sink(e);
                m_moved = false;
            }
            if (d.grabbed)
                forward(EV_SYN, SYN_REPORT, 0);
            return;
        }
        if (ev.type != EV_KEY)
            return;

        if (ev.code >= BTN_MISC && ev.code < KEY_OK) { // mouse buttons
            if (ev.value == 2)
                return;
            InputEvent e = base(d, now);
            e.type = ev.value ? InputEventType::MouseDown : InputEventType::MouseUp;
            switch (ev.code) {
            case BTN_LEFT: e.button = MouseButtonId::Left; break;
            case BTN_RIGHT: e.button = MouseButtonId::Right; break;
            case BTN_MIDDLE: e.button = MouseButtonId::Middle; break;
            case BTN_SIDE: e.button = MouseButtonId::Back; break;
            case BTN_EXTRA: e.button = MouseButtonId::Forward; break;
            default: e.button = MouseButtonId::Other; break;
            }
            m_sink(e);
            return;
        }

        InputEvent e = base(d, now);
        e.type = ev.value ? InputEventType::KeyDown : InputEventType::KeyUp;
        e.autoRepeat = ev.value == 2;
        e.nativeKeyCode = ev.code;
        e.key = evdevKeyToQtKey(ev.code, &e.keypad);
        if (const Modifiers m = modifierForKey(e.key)) {
            if (ev.value)
                m_modifiers |= m;
            else
                m_modifiers &= Modifiers(~m);
        }
        e.modifiers = m_modifiers;
        if (m_suppression && m_suppression->shouldSwallow(e))
            e.swallowed = true;
        m_sink(e);
        if (d.grabbed && !e.swallowed)
            forward(EV_KEY, ev.code, ev.value); // re-send keys that are not blocked
    }

    InputEvent base(const Device &d, qint64 now) const
    {
        InputEvent e;
        e.timestampMs = now;
        e.position = m_position;
        e.positionKnown = false;
        e.modifiers = m_modifiers;
        e.synthetic = d.ours;
        return e;
    }

    void forward(unsigned int type, unsigned int code, int value)
    {
        if (m_virtualKeyboard)
            libevdev_uinput_write_event(m_virtualKeyboard, type, code, value);
    }

    void closeAll()
    {
        for (Device &d : m_devices) {
            if (d.grabbed)
                libevdev_grab(d.dev, LIBEVDEV_UNGRAB);
            libevdev_free(d.dev);
            ::close(d.fd);
        }
        m_devices.clear();
        m_deviceCount.store(0);
        if (m_virtualKeyboard)
            libevdev_uinput_destroy(m_virtualKeyboard);
        m_virtualKeyboard = nullptr;
        m_canForward.store(false);
        if (m_inotify >= 0)
            ::close(m_inotify);
        m_inotify = -1;
        for (int &fd : m_wake) {
            if (fd >= 0)
                ::close(fd);
            fd = -1;
        }
    }

    InputEventSink m_sink;
    KeySuppressionRegistry *m_suppression = nullptr;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};
    std::atomic<bool> m_canForward{false};
    std::atomic<int> m_deviceCount{0};
    std::atomic<int> m_rescans{0};
    int m_wake[2] = {-1, -1};
    int m_inotify = -1;
    std::vector<Device> m_devices;
    libevdev_uinput *m_virtualKeyboard = nullptr;
    QPointF m_position;
    bool m_moved = false;
    Modifiers m_modifiers = Modifier::None;
};

class UinputClickInjector final : public IClickInjector {
public:
    ~UinputClickInjector() override
    {
        if (m_device)
            libevdev_uinput_destroy(m_device);
    }

    PlatformResult click(MouseButton button) override { return send(button, {1, 0}); }
    PlatformResult doubleClick(MouseButton button) override { return send(button, {1, 0, 1, 0}); }
    PlatformResult press(MouseButton button) override { return send(button, {1}); }
    PlatformResult release(MouseButton button) override { return send(button, {0}); }
    int doubleClickIntervalMs() const override { return 400; }

private:
    PlatformResult send(MouseButton button, std::initializer_list<int> values)
    {
        QMutexLocker lock(&m_mutex);
        if (!m_device) {
            libevdev *proto = libevdev_new();
            libevdev_set_name(proto, kVirtualMouseName);
            libevdev_enable_event_type(proto, EV_KEY);
            for (unsigned int code : {BTN_LEFT, BTN_RIGHT, BTN_MIDDLE})
                libevdev_enable_event_code(proto, EV_KEY, code, nullptr);
            libevdev_enable_event_type(proto, EV_REL);
            libevdev_enable_event_code(proto, EV_REL, REL_X, nullptr);
            libevdev_enable_event_code(proto, EV_REL, REL_Y, nullptr);
            libevdev_enable_event_type(proto, EV_SYN);
            const int rc = libevdev_uinput_create_from_device(proto, LIBEVDEV_UINPUT_OPEN_MANAGED, &m_device);
            libevdev_free(proto);
            if (rc < 0) {
                m_device = nullptr;
                return PlatformResult::permissionDenied(
                    tr("Cannot create the virtual mouse: no write access to /dev/uinput (see documentation/BUILD.md "
                       "6.3)."));
            }
        }
        const unsigned int code = button == MouseButton::Right ? BTN_RIGHT : button == MouseButton::Middle ? BTN_MIDDLE : BTN_LEFT;
        for (int value : values) {
            libevdev_uinput_write_event(m_device, EV_KEY, code, value);
            libevdev_uinput_write_event(m_device, EV_SYN, SYN_REPORT, 0);
        }
        return PlatformResult::success();
    }

    libevdev_uinput *m_device = nullptr;
    QMutex m_mutex;
};

} // namespace

std::unique_ptr<IInputHook> createEvdevInputHook()
{
    return std::make_unique<EvdevInputHook>();
}

std::unique_ptr<IClickInjector> createUinputClickInjector()
{
    return std::make_unique<UinputClickInjector>();
}

} // namespace ergo
