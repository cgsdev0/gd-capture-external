#include "pch.h"
#include "window_capture_texture.h"
#include "d3dHelpers.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <wtypes.h>
#include <mutex>

godot::WindowCaptureTexture::WindowCaptureTexture()
{
}

godot::WindowCaptureTexture::~WindowCaptureTexture()
{
    texture.unref();
}

void godot::WindowCaptureTexture::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("start_capture", "hwnd"), &WindowCaptureTexture::start_capture);
}

void godot::WindowCaptureTexture::_process(double delta)
{
    if (!texture.is_valid() || !frame_ready)
        return;
    RenderingServer *rs = RenderingServer::get_singleton();
    RenderingDevice *rd = rs->get_rendering_device();

    if (rd->texture_is_valid(texture->rid))
    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        rd->texture_update(texture->rid, 0, bytes);
        this->frame_ready = false;
    }
}

void godot::WindowCaptureTexture::OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender, winrt::Windows::Foundation::IInspectable const &)
{
    auto frame = this->framePool.TryGetNextFrame();
    if (!frame)
    {
        return;
    }
    auto frameContentSize = frame.ContentSize();
    auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

    D3D11_TEXTURE2D_DESC desc;
    frameSurface->GetDesc(&desc);

    this->d3dContext->CopyResource(staging_texture.get(), frameSurface.get());

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = this->d3dContext->Map(staging_texture.get(), 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr))
    {
        godot::print_error("Failed to map texture");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        memcpy(bytes.ptrw(), static_cast<uint8_t *>(mappedResource.pData), 4 * cx * cy);
        this->frame_ready = true;
    }
}

bool godot::WindowCaptureTexture::start_capture(int64_t hwnd)
{
    // Start capture using the provided HWND
    // HWND win_hwnd = reinterpret_cast<HWND>(hwnd);

    std::string hwnd_str = "68652";
    HWND win_hwnd = reinterpret_cast<HWND>(std::stoul(hwnd_str));

    // Get the RenderingDevice interface

    using namespace winrt;
    using namespace Windows;
    using namespace Windows::Foundation;
    using namespace Windows::System;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;

    try
    {
        auto d3dDevice = D3D11DeviceSingleton::get_instance().get_device();
        winrt::com_ptr<ID3D11Device> device;
        device.attach(d3dDevice);
        device->GetImmediateContext(this->d3dContext.put());
        auto dxgiDevice = device.as<IDXGIDevice>();
        auto directDevice = CreateDirect3DDevice(dxgiDevice.get());
        this->item = CreateCaptureItemForWindow(win_hwnd);
        auto size = this->item.Size();
        if (size.Width == 0 && size.Height == 0)
        {
            return false;
        }

        // Create staging texture
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = size.Width;
        desc.Height = size.Height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_STAGING; // Staging allows CPU access
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        HRESULT hr = device->CreateTexture2D(&desc, nullptr, staging_texture.put());
        if (FAILED(hr))
        {
            godot::print_error("Failed to create staging texture");
            return false;
        }

        this->framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(
            directDevice,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            size);
        this->session = this->framePool.CreateCaptureSession(this->item);
        this->session.StartCapture();
        create_texture(RenderingDevice::DATA_FORMAT_B8G8R8A8_UNORM, size.Width, size.Height, true);
        this->arrived = this->framePool.FrameArrived(auto_revoke, {this, &godot::WindowCaptureTexture::OnFrameArrived});
        godot::print_line("texture initialized ", size.Width, "x", size.Height);
    }
    catch (HRESULT e)
    {
        godot::print_error("well it didnt go good", e);
    }
    return false;
}