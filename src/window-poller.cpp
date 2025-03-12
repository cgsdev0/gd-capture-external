#include "window-poller.h"
#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "pch.h"
#include <xmemory>
#include <dwmapi.h>
#include <WinUser.h>

struct sEnumInfo
{
    int index = 0;
    HMONITOR hMonitor = NULL;
};

// Get monitor by index
static HMONITOR GetMonitorByIndex(int monitorIndex)
{
    sEnumInfo info;
    info.index = monitorIndex;
    int index = 0;
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) -> BOOL
                        {
            auto info = reinterpret_cast<sEnumInfo*>(lParam);
    info->hMonitor = hMonitor;
    if (info->index-- == 0)
    {
        return FALSE;
    }
    return TRUE; }, reinterpret_cast<LPARAM>(&info));

    return info.hMonitor;
}

/// ENUM WINDOWS

static std::wstring _GetClassName(HWND hwnd)
{
    std::array<WCHAR, 1024> className;
    ::GetClassNameW(hwnd, className.data(), (int)className.size());
    std::wstring title(className.data());
    return title;
}

static std::wstring _GetWindowText(HWND hwnd)
{
    std::array<WCHAR, 1024> windowText;
    ::GetWindowTextW(hwnd, windowText.data(), (int)windowText.size());
    std::wstring title(windowText.data());
    return title;
}

static bool _IsAltTabWindow(Window const &window)
{
    HWND hwnd = window.Hwnd();
    HWND shellWindow = GetShellWindow();

    auto title = window.Title();
    auto className = window.ClassName();

    if (hwnd == shellWindow)
    {
        return false;
    }

    if (title.length() == 0)
    {
        return false;
    }

    if (!IsWindowVisible(hwnd))
    {
        return false;
    }

    if (GetAncestor(hwnd, GA_ROOT) != hwnd)
    {
        return false;
    }

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (!((style & WS_DISABLED) != WS_DISABLED))
    {
        return false;
    }

    DWORD cloaked = FALSE;
    HRESULT hrTemp = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (SUCCEEDED(hrTemp) &&
        cloaked == DWM_CLOAKED_SHELL)
    {
        return false;
    }

    return true;
}

struct sEnumInfo2
{
    HMONITOR hMonitor = NULL;
    std::vector<Window> windows;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto class_name = _GetClassName(hwnd);
    auto title = _GetWindowText(hwnd);

    auto window = Window(hwnd, title, class_name);

    if (class_name == L"XamlExplorerHostIslandWindow")
    {
        return TRUE;
    }

    if (class_name == L"XamlWindow")
    {
        return TRUE;
    }

    if (class_name == L"SnipOverlayRootWindow")
    {
        return TRUE;
    }

    if (!_IsAltTabWindow(window))
    {
        return TRUE;
    }

    if (!IsWindowVisible(hwnd))
        return TRUE;

    RECT rect;
    if (!GetWindowRect(hwnd, &rect))
        return TRUE;

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    if (width <= 16 && height <= 16)
        return TRUE;

    auto info = reinterpret_cast<sEnumInfo2 *>(lParam);

    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (hMonitor != info->hMonitor)
        return TRUE;

    info->windows.push_back(window);

    return TRUE;
}

static const std::vector<Window> EnumerateWindows(HMONITOR monitor)
{
    sEnumInfo2 info;
    info.hMonitor = monitor;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&info));

    return info.windows;
}
///////

void godot::WindowPoller::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("set_monitor_index", "should_capture"), &WindowPoller::set_monitor_index);
    godot::ClassDB::bind_method(godot::D_METHOD("get_monitor_index"), &WindowPoller::get_monitor_index);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "monitor_index"), "set_monitor_index", "get_monitor_index");

    ADD_SIGNAL(MethodInfo(
        "window_opened",
        PropertyInfo(Variant::INT, "hwnd"), PropertyInfo(Variant::STRING, "title"), PropertyInfo(Variant::STRING, "class_type")));
    ADD_SIGNAL(MethodInfo(
        "window_closed",
        PropertyInfo(Variant::INT, "hwnd")));
    ADD_SIGNAL(MethodInfo(
        "focus_change",
        PropertyInfo(Variant::INT, "hwnd")));
}

void godot::WindowPoller::set_monitor_index(int monitor_index)
{
    if (monitor_index < 0)
        return;
    this->monitor_index = monitor_index;
    this->monitor = GetMonitorByIndex(monitor_index);
}

int godot::WindowPoller::get_monitor_index()
{
    return this->monitor_index;
}

void godot::WindowPoller::_physics_process(double delta)
{
    HWND activeWindow = GetForegroundWindow();
    if (activeWindow && activeWindow != lastActive)
    {
        lastActive = activeWindow;
        emit_signal("focus_change", reinterpret_cast<uint64_t>(activeWindow));
    }
    std::vector<::Window> w = ::EnumerateWindows(this->monitor);
    for (auto &[hwnd, capture] : activeCaptures)
    {
        capture.ttl--;
    }
    for (const auto &window : w)
    {
        HWND hwnd = window.Hwnd();
        bool active = activeWindow == hwnd;

        auto it = activeCaptures.find(hwnd);
        if (it == activeCaptures.end())
        {
            auto title = godot::String(window.Title().c_str());
            auto class_type = godot::String(window.ClassName().c_str());
            activeCaptures[hwnd] = ActiveCapture{TTL_FRAMES};
            emit_signal("window_opened", reinterpret_cast<uint64_t>(hwnd), title, class_type);
            if (active)
            {
                // synthetic event - edge case when window is moved in from off-screen
                emit_signal("focus_change", reinterpret_cast<uint64_t>(hwnd));
            }
        }
        else
        {
            it->second.ttl = TTL_FRAMES;
        }
    }
    for (auto it = activeCaptures.begin(); it != activeCaptures.end();)
    {
        if (it->second.ttl <= 0)
        {
            emit_signal("window_closed", reinterpret_cast<uint64_t>(it->first));
            it = activeCaptures.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

godot::WindowPoller::WindowPoller()
{
    this->monitor = GetMonitorByIndex(monitor_index);
    using namespace winrt;
    using namespace Windows::Security;
    using namespace Windows::Graphics::Capture;
    auto result = GraphicsCaptureAccess::RequestAccessAsync(GraphicsCaptureAccessKind::Borderless);
    result.Completed([](auto a, auto b)
                     {
        if (a.GetResults() != Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::Allowed)
        {
            std::cerr << "Borderless capture permission denied." << std::endl;
        } });
}

godot::WindowPoller::~WindowPoller()
{
}