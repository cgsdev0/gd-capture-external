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
    stop_capture();
    texture.unref();
}

void godot::WindowCaptureTexture::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("start_capture", "hwnd"), &WindowCaptureTexture::start_capture);
    godot::ClassDB::bind_method(godot::D_METHOD("stop_capture"), &WindowCaptureTexture::stop_capture);
    godot::ClassDB::bind_method(godot::D_METHOD("set_mouse_capture", "should_capture"), &WindowCaptureTexture::set_mouse_capture);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mouse_capture"), &WindowCaptureTexture::get_mouse_capture);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "mouse_capture"), "set_mouse_capture", "get_mouse_capture");
}

void godot::WindowCaptureTexture::set_mouse_capture(bool should_capture)
{
    this->should_capture_cursor = should_capture;
    if (this->session)
    {
        this->session.IsCursorCaptureEnabled(should_capture);
    }
}

bool godot::WindowCaptureTexture::get_mouse_capture()
{
    return this->should_capture_cursor;
}

void godot::WindowCaptureTexture::_process(double delta)
{
    if (!texture.is_valid() || closed.load() == true)
        return;

    // update our texture if we have resized
    create_texture(RenderingDevice::DATA_FORMAT_B8G8R8A8_UNORM, _cx, _cy, true);

    if (!frame_ready)
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
void godot::WindowCaptureTexture::stop_capture()
{
    auto expected = false;
    if (closed.compare_exchange_strong(expected, true))
    {
        std::lock_guard<std::mutex> lock(frame_mutex);
        arrived.revoke();
        framePool.Close();
        session.Close();
        framePool = nullptr;
        session = nullptr;
        item = nullptr;
    }
}

void godot::WindowCaptureTexture::OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender, winrt::Windows::Foundation::IInspectable const &)
{
    if (closed.load() == true)
        return;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame = sender.TryGetNextFrame();

    // drain
    while (auto next = sender.TryGetNextFrame()) {
        frame = next;
    }

    auto size = frame.ContentSize();
    auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    auto newSize = false;
    if (_cx != size.Width || _cy != size.Height)
    {
        newSize = true;
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
        auto d3dDevice = D3D11DeviceSingleton::get_instance().get_device();
        HRESULT hr = d3dDevice->CreateTexture2D(&desc, nullptr, staging_texture.put());
    }
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
        this->_cx = size.Width;
        this->_cy = size.Height;
        // we may drop frames when resizing bigger; oh well
        if (bytes.size() >= 4 * _cx * _cy)
        {
            // memcpy(bytes.ptrw(), static_cast<uint8_t *>(mappedResource.pData), 4 * cx * cy);
            auto rowPitch = mappedResource.RowPitch;
            auto dst = bytes.ptrw();
            auto src = static_cast<uint8_t *>(mappedResource.pData);
            for (int y = 0; y < _cy; ++y)
            {
                memcpy(dst + (y * _cx * 4), src + (y * rowPitch), _cx * 4);
            }
            this->frame_ready = true;
        }
        d3dContext->Unmap(staging_texture.get(), 0);
    }
    if (newSize)
    {
        framePool.Recreate(
            this->directDevice,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            1,
            size);
    }
}

bool godot::WindowCaptureTexture::start_capture(int64_t hwnd)
{
    if (closed.load() == false)
        return false;

    // Start capture using the provided HWND
    HWND win_hwnd = reinterpret_cast<HWND>(hwnd);

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
        this->directDevice = CreateDirect3DDevice(dxgiDevice.get());
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
            1,
            size);
        this->session = this->framePool.CreateCaptureSession(this->item);
        this->session.IsBorderRequired(false);
        this->session.IsCursorCaptureEnabled(should_capture_cursor);
        this->session.StartCapture();
        _cx = size.Width;
        _cy = size.Height;
        create_texture(RenderingDevice::DATA_FORMAT_B8G8R8A8_UNORM, size.Width, size.Height, true);
        this->arrived = this->framePool.FrameArrived(auto_revoke, {this, &godot::WindowCaptureTexture::OnFrameArrived});
        godot::print_line("texture initialized ", size.Width, "x", size.Height);
        closed.exchange(false);
    }
    catch (HRESULT e)
    {
        godot::print_error("well it didnt go good", e);
        return false;
    }
    return true;
}