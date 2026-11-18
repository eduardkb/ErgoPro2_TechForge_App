// Keep Screen Active on Linux X11 (B.10): EWMH window lookup and XTest input.
#include "platform/linux/LinuxInput.h"

#include "core/Clock.h"
#include "core/LogCategories.h"

#include <QCoreApplication>
#include <QStringList>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>

namespace ergo {

namespace {

QStringList windowClasses(KeepActiveApp app)
{
    switch (app) {
    case KeepActiveApp::Teams:
        return {QStringLiteral("teams-for-linux"), QStringLiteral("microsoft teams"), QStringLiteral("teams")};
    case KeepActiveApp::Zoom:
        return {QStringLiteral("zoom"), QStringLiteral("zoom workplace")};
    }
    return {};
}

// Opens and closes its own connection (used every few minutes only).
class DisplayConnection {
public:
    DisplayConnection() : m_display(XOpenDisplay(nullptr)) {}
    ~DisplayConnection()
    {
        if (m_display)
            XCloseDisplay(m_display);
    }
    Display *get() const { return m_display; }

private:
    Display *m_display;
};

QList<Window> clientList(Display *d)
{
    QList<Window> list;
    Atom type;
    int format;
    unsigned long count = 0, after = 0;
    unsigned char *data = nullptr;
    const Atom prop = XInternAtom(d, "_NET_CLIENT_LIST", False);
    if (XGetWindowProperty(d, DefaultRootWindow(d), prop, 0, 4096, False, XA_WINDOW, &type, &format, &count, &after,
                           &data) == Success
        && data) {
        const auto *windows = reinterpret_cast<const Window *>(data);
        for (unsigned long i = 0; i < count; ++i)
            list << windows[i];
        XFree(data);
    }
    return list;
}

bool matchesClass(Display *d, Window w, const QStringList &classes)
{
    XClassHint hint{};
    if (!XGetClassHint(d, w, &hint))
        return false;
    const QString name = QString::fromLocal8Bit(hint.res_name ? hint.res_name : "").toLower();
    const QString cls = QString::fromLocal8Bit(hint.res_class ? hint.res_class : "").toLower();
    if (hint.res_name)
        XFree(hint.res_name);
    if (hint.res_class)
        XFree(hint.res_class);
    return classes.contains(name) || classes.contains(cls);
}

// Frame in root coordinates incl. the window manager's title bar (_NET_FRAME_EXTENTS).
std::optional<QRectF> windowFrame(Display *d, Window w, bool *viewable)
{
    XWindowAttributes attributes;
    if (!XGetWindowAttributes(d, w, &attributes))
        return std::nullopt;
    *viewable = attributes.map_state == IsViewable;
    int x = 0, y = 0;
    Window child;
    XTranslateCoordinates(d, w, DefaultRootWindow(d), 0, 0, &x, &y, &child);
    long left = 0, right = 0, top = 0, bottom = 0;
    Atom type;
    int format;
    unsigned long count = 0, after = 0;
    unsigned char *data = nullptr;
    if (XGetWindowProperty(d, w, XInternAtom(d, "_NET_FRAME_EXTENTS", False), 0, 4, False, XA_CARDINAL, &type,
                           &format, &count, &after, &data) == Success
        && data) {
        if (count == 4) {
            const auto *e = reinterpret_cast<const long *>(data);
            left = e[0];
            right = e[1];
            top = e[2];
            bottom = e[3];
        }
        XFree(data);
    }
    // Root coordinates = logical pixels without Qt scaling (as the input hook).
    return QRectF(x - left, y - top, attributes.width + left + right, attributes.height + top + bottom);
}

// Largest window of the application (viewable or not).
Window findMainWindow(Display *d, KeepActiveApp app)
{
    const QStringList classes = windowClasses(app);
    Window best = 0;
    double bestArea = 0;
    for (Window w : clientList(d)) {
        if (!matchesClass(d, w, classes))
            continue;
        bool viewable = false;
        const std::optional<QRectF> frame = windowFrame(d, w, &viewable);
        const double area = frame ? frame->width() * frame->height() : 1;
        if (!best || area > bestArea) {
            best = w;
            bestArea = area;
        }
    }
    return best;
}

class X11AppWindows final : public IAppWindows {
    Q_DECLARE_TR_FUNCTIONS(ergo::X11AppWindows)
public:
    explicit X11AppWindows(SyntheticTracker *tracker) : m_tracker(tracker) {}

    QString unsupportedReason() const override { return {}; }

    Raise raise(KeepActiveApp app, QString *detail) override
    {
        DisplayConnection c;
        Display *d = c.get();
        if (!d) {
            if (detail)
                *detail = tr("cannot connect to the X11 display");
            return Raise::Failed;
        }
        // Without a listed window the application counts as not running (a
        // tray-only Teams cannot be found through the window list).
        const Window w = findMainWindow(d, app);
        if (!w)
            return Raise::NotRunning;
        // EWMH activation: the window manager restores (un-minimizes) and raises it.
        XEvent event{};
        event.xclient.type = ClientMessage;
        event.xclient.window = w;
        event.xclient.message_type = XInternAtom(d, "_NET_ACTIVE_WINDOW", False);
        event.xclient.format = 32;
        event.xclient.data.l[0] = 2; // source: pager / tool
        event.xclient.data.l[1] = CurrentTime;
        XSendEvent(d, DefaultRootWindow(d), False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XMapRaised(d, w);
        XFlush(d);
        return Raise::Raised;
    }

    std::optional<QRectF> mainWindowFrame(KeepActiveApp app) const override
    {
        DisplayConnection c;
        Display *d = c.get();
        if (!d)
            return std::nullopt;
        const Window w = findMainWindow(d, app);
        if (!w)
            return std::nullopt;
        bool viewable = false;
        const std::optional<QRectF> frame = windowFrame(d, w, &viewable);
        return viewable ? frame : std::nullopt;
    }

    PlatformResult clickAt(const QPointF &point) override
    {
        DisplayConnection c;
        Display *d = c.get();
        int event = 0, error = 0, major = 0, minor = 0;
        if (!d || !XTestQueryExtension(d, &event, &error, &major, &minor))
            return PlatformResult::notSupported(tr("The X11 XTEST extension is not available."));
        int x = 0, y = 0;
        if (!pointer(d, &x, &y))
            return PlatformResult::failed(tr("Cannot read the pointer position."));
        const qint64 now = monotonicNowMs();
        m_tracker->expectMotion(2, now);
        m_tracker->expect(2, now);
        XTestFakeMotionEvent(d, -1, int(point.x()), int(point.y()), CurrentTime);
        XTestFakeButtonEvent(d, 1, True, CurrentTime);
        XTestFakeButtonEvent(d, 1, False, CurrentTime);
        XTestFakeMotionEvent(d, -1, x, y, CurrentTime); // pointer back
        XFlush(d);
        return PlatformResult::success();
    }

    PlatformResult nudgePointer() override
    {
        DisplayConnection c;
        Display *d = c.get();
        int event = 0, error = 0, major = 0, minor = 0;
        if (!d || !XTestQueryExtension(d, &event, &error, &major, &minor))
            return PlatformResult::notSupported(tr("The X11 XTEST extension is not available."));
        int x = 0, y = 0;
        if (!pointer(d, &x, &y))
            return PlatformResult::failed(tr("Cannot read the pointer position."));
        m_tracker->expectMotion(2, monotonicNowMs());
        XTestFakeMotionEvent(d, -1, x + 1, y, CurrentTime);
        XTestFakeMotionEvent(d, -1, x, y, CurrentTime);
        XFlush(d);
        return PlatformResult::success();
    }

private:
    static bool pointer(Display *d, int *x, int *y)
    {
        Window root, child;
        int wx, wy;
        unsigned int mask;
        return XQueryPointer(d, DefaultRootWindow(d), &root, &child, x, y, &wx, &wy, &mask);
    }

    SyntheticTracker *m_tracker;
};

} // namespace

std::unique_ptr<IAppWindows> createX11AppWindows(SyntheticTracker *tracker)
{
    return std::make_unique<X11AppWindows>(tracker);
}

} // namespace ergo
