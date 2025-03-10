#ifndef PCH_H
#define PCH_H

#include <Unknwn.h>
#include <inspectable.h>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>

// STL
#include <locale>
#include <atomic>
#include <memory>

// D3D
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

// Helpers
#include "d3dHelpers.h"
#include "direct3d11.interop.h"
#include "capture.interop.h"

struct Window
{
public:
    Window(nullptr_t) {}
    Window(HWND hwnd, ::std::wstring const &title, ::std::wstring &className)
    {
        m_hwnd = hwnd;
        m_title = title;
        m_className = className;
    }

    HWND Hwnd() const noexcept { return m_hwnd; }
    ::std::wstring Title() const noexcept { return m_title; }
    ::std::wstring ClassName() const noexcept { return m_className; }

private:
    HWND m_hwnd;
    ::std::wstring m_title;
    ::std::wstring m_className;
};
#endif