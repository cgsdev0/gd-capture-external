#ifndef CAPTURE_INTEROP_H
#define CAPTURE_INTEROP_H
#include <winrt/Windows.Graphics.Capture.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.capture.h>

inline auto CreateCaptureItemForWindow(HWND hwnd)
{
    // winrt::Windows::UI::WindowId windowId{};
    // windowId.Value = reinterpret_cast<uint64_t>(hwnd);
    // auto item = winrt::Windows::Graphics::Capture::GraphicsCaptureItem::TryCreateFromWindowId(windowId);
    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = {nullptr};
    auto res =
        interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void **>(winrt::put_abi(item)));
    if (res < 0)
    {
        throw res;
    }
    return item;
}

#endif