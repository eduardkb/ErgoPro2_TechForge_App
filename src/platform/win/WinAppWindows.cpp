#include "platform/win/WinAppWindows.h"

#include "core/LogCategories.h"
#include "platform/common/Synthetic.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#include <shellscalingapi.h>

#include <QFileInfo>

#include <algorithm>
#include <cmath>

namespace ergo {

namespace {

QStringList processNames(KeepActiveApp app)
{
    switch (app) {
    case KeepActiveApp::Teams:
        return {QStringLiteral("ms-teams.exe"), QStringLiteral("teams.exe")}; // new, classic
    case KeepActiveApp::Zoom:
        return {QStringLiteral("zoom.exe")};
    }
    return {};
}

QString processName(DWORD pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return {};
    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    QString name;
    if (QueryFullProcessImageNameW(process, 0, path, &size))
        name = QFileInfo(QString::fromWCharArray(path, int(size))).fileName().toLower();
    CloseHandle(process);
    return name;
}

struct Candidate {
    HWND hwnd = nullptr;
    bool visible = false;
    double area = 0;
};

struct Search {
    QStringList names;
    Candidate best;
    bool running = false;
};

BOOL CALLBACK enumWindow(HWND hwnd, LPARAM lParam)
{
    auto *search = reinterpret_cast<Search *>(lParam);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!search->names.contains(processName(pid)))
        return TRUE;
    search->running = true;
    // Main windows: top-level, not owned, with a caption and a title.
    const LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if (GetWindow(hwnd, GW_OWNER) || !(style & WS_CAPTION) || GetWindowTextLengthW(hwnd) == 0)
        return TRUE;
    WINDOWPLACEMENT placement = {};
    placement.length = sizeof(placement);
    GetWindowPlacement(hwnd, &placement);
    const RECT r = placement.rcNormalPosition;
    Candidate c{hwnd, IsWindowVisible(hwnd) != 0, double(r.right - r.left) * double(r.bottom - r.top)};
    // Prefer visible windows, then the largest.
    if (!search->best.hwnd || (c.visible && !search->best.visible)
        || (c.visible == search->best.visible && c.area > search->best.area))
        search->best = c;
    return TRUE;
}

Search findMainWindow(KeepActiveApp app)
{
    Search search;
    search.names = processNames(app);
    EnumWindows(enumWindow, reinterpret_cast<LPARAM>(&search));
    return search;
}

double monitorScale(HMONITOR monitor)
{
    UINT dpiX = 96, dpiY = 96;
    if (!monitor || FAILED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) || dpiX == 0)
        dpiX = 96;
    return double(dpiX) / 96.0;
}

// Same mapping as the input hook (WinInput.cpp): Qt keeps each monitor's
// top-left corner and scales relative to it.
QPointF toLogical(POINT pt)
{
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    const double scale = monitorScale(monitor);
    MONITORINFO info = {};
    info.cbSize = sizeof(info);
    if (!monitor || !GetMonitorInfoW(monitor, &info))
        return QPointF(pt.x / scale, pt.y / scale);
    const double ox = info.rcMonitor.left, oy = info.rcMonitor.top;
    return QPointF(ox + (pt.x - ox) / scale, oy + (pt.y - oy) / scale);
}

struct MonitorSearch {
    QPointF logical;
    POINT physical{0, 0};
    bool found = false;
};

BOOL CALLBACK enumMonitor(HMONITOR monitor, HDC, LPRECT rect, LPARAM lParam)
{
    auto *s = reinterpret_cast<MonitorSearch *>(lParam);
    const double scale = monitorScale(monitor);
    const double ox = rect->left, oy = rect->top;
    const QRectF logicalRect(ox, oy, (rect->right - rect->left) / scale, (rect->bottom - rect->top) / scale);
    if (!logicalRect.contains(s->logical))
        return TRUE;
    s->physical.x = LONG(std::lround(ox + (s->logical.x() - ox) * scale));
    s->physical.y = LONG(std::lround(oy + (s->logical.y() - oy) * scale));
    s->found = true;
    return FALSE;
}

std::optional<POINT> toPhysical(const QPointF &logical)
{
    MonitorSearch s;
    s.logical = logical;
    EnumDisplayMonitors(nullptr, nullptr, enumMonitor, reinterpret_cast<LPARAM>(&s));
    if (!s.found)
        return std::nullopt;
    return s.physical;
}

INPUT mouseInput(DWORD flags, LONG dx = 0, LONG dy = 0)
{
    INPUT in = {};
    in.type = INPUT_MOUSE;
    in.mi.dx = dx;
    in.mi.dy = dy;
    in.mi.dwFlags = flags;
    in.mi.dwExtraInfo = ULONG_PTR(kSyntheticEventMarker);
    return in;
}

// Absolute move to a physical position on the virtual desktop.
INPUT absoluteMove(POINT p)
{
    const int left = GetSystemMetrics(SM_XVIRTUALSCREEN), top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = std::max(2, GetSystemMetrics(SM_CXVIRTUALSCREEN));
    const int height = std::max(2, GetSystemMetrics(SM_CYVIRTUALSCREEN));
    const LONG nx = LONG(std::lround(double(p.x - left) * 65535.0 / double(width - 1)));
    const LONG ny = LONG(std::lround(double(p.y - top) * 65535.0 / double(height - 1)));
    return mouseInput(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK, nx, ny);
}

} // namespace

IAppWindows::Raise WinAppWindows::raise(KeepActiveApp app, QString *detail)
{
    const Search search = findMainWindow(app);
    if (!search.running)
        return Raise::NotRunning;
    if (!search.best.hwnd) {
        if (detail)
            *detail = tr("the application has no main window");
        return Raise::Failed;
    }
    HWND hwnd = search.best.hwnd;
    if (IsIconic(hwnd))
        ShowWindow(hwnd, SW_RESTORE);
    else if (!IsWindowVisible(hwnd))
        ShowWindow(hwnd, SW_SHOW); // hidden in the tray
    BringWindowToTop(hwnd);
    if (!SetForegroundWindow(hwnd))
        qCDebug(lcPlatform) << "SetForegroundWindow refused (foreground lock); the click activates the window";
    return Raise::Raised;
}

std::optional<QRectF> WinAppWindows::mainWindowFrame(KeepActiveApp app) const
{
    const Search search = findMainWindow(app);
    HWND hwnd = search.best.hwnd;
    if (!hwnd || !IsWindowVisible(hwnd) || IsIconic(hwnd))
        return std::nullopt;
    RECT r;
    // The visible frame without the invisible resize borders of Windows 10/11.
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r))) && !GetWindowRect(hwnd, &r))
        return std::nullopt;
    const QPointF topLeft = toLogical(POINT{r.left, r.top});
    const QPointF bottomRight = toLogical(POINT{r.right, r.bottom});
    return QRectF(topLeft, bottomRight);
}

PlatformResult WinAppWindows::clickAt(const QPointF &point)
{
    const std::optional<POINT> target = toPhysical(point);
    if (!target)
        return PlatformResult::failed(tr("The click position is outside every screen."));
    POINT original;
    if (!GetCursorPos(&original))
        return PlatformResult::failed(tr("Cannot read the pointer position (error %1).").arg(GetLastError()));
    INPUT input[4] = {absoluteMove(*target), mouseInput(MOUSEEVENTF_LEFTDOWN), mouseInput(MOUSEEVENTF_LEFTUP),
                      absoluteMove(original)};
    const UINT sent = SendInput(4, input, sizeof(INPUT));
    if (sent != 4)
        return PlatformResult::failed(tr("Windows did not accept the click (error %1).").arg(GetLastError()));
    return PlatformResult::success();
}

PlatformResult WinAppWindows::nudgePointer()
{
    INPUT input[2] = {mouseInput(MOUSEEVENTF_MOVE, 1, 0), mouseInput(MOUSEEVENTF_MOVE, -1, 0)};
    if (SendInput(2, input, sizeof(INPUT)) != 2)
        return PlatformResult::failed(tr("Windows did not accept the pointer movement (error %1).").arg(GetLastError()));
    return PlatformResult::success();
}

} // namespace ergo
