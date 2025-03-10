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
    auto res = static_cast<ID3D11Resource *>(frameSurface.get());
    auto interop = D3D11DeviceSingleton::get_instance().get_device11on12();
    auto d3d12Device = D3D11DeviceSingleton::get_instance().get_device12();
    auto command_queue = D3D11DeviceSingleton::get_instance().get_command_queue();

    ID3D12Resource *capture_d3d12_resource;
    interop->CreateWrappedResource(
        frameSurface.get(),
        nullptr,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        IID_PPV_ARGS(&capture_d3d12_resource));

    this->commandAllocator->Reset();
    this->commandList->Reset(this->commandAllocator.get(), nullptr);

    ID3D12Resource *godot_internal_tex = this->tex_handle;

    D3D12_RESOURCE_BARRIER barrier_to_copy_dest = {};
    barrier_to_copy_dest.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_to_copy_dest.Transition.pResource = godot_internal_tex;
    barrier_to_copy_dest.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier_to_copy_dest.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier_to_copy_dest.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier_to_copy_dest);

    // Perform the copy from D3D11 texture to D3D12 texture
    commandList->CopyResource(godot_internal_tex, capture_d3d12_resource);

    D3D12_RESOURCE_BARRIER barrier_back_to_shader = {};
    barrier_back_to_shader.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_back_to_shader.Transition.pResource = godot_internal_tex;
    barrier_back_to_shader.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier_back_to_shader.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier_back_to_shader.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier_back_to_shader);

    HRESULT hr = commandList->Close();
    if (FAILED(hr))
    {
        godot::print_error("failed to close ", hr);
    }

    // 4. Execute the command list
    ID3D12CommandList *ppCommandLists[] = {commandList.get()};
    command_queue->ExecuteCommandLists(1, ppCommandLists);
}

bool godot::WindowCaptureTexture::start_capture(int64_t hwnd)
{
    // hard-coded for now
    std::string hwnd_str = "68652";
    HWND win_hwnd = reinterpret_cast<HWND>(std::stoul(hwnd_str));

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
        auto d3d12Device = D3D11DeviceSingleton::get_instance().get_device12();
        auto interop = D3D11DeviceSingleton::get_instance().get_device11on12();
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

        HRESULT hr = d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
        if (FAILED(hr))
        {
            godot::print_error("failed to create command allocator");
        }

        hr = d3d12Device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocator.get(),
            nullptr,
            IID_PPV_ARGS(&commandList));
        if (FAILED(hr))
        {
            godot::print_error("failed to create command list");
        }

        this->swapChain = CreateDXGISwapChain(
            device,
            static_cast<uint32_t>(size.Width),
            static_cast<uint32_t>(size.Height),
            static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized),
            2);
        this->framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(
            directDevice,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            size);
        this->session = this->framePool.CreateCaptureSession(this->item);
        this->session.StartCapture();

        create_texture(RenderingDevice::DATA_FORMAT_B8G8R8A8_UNORM, size.Width, size.Height, true);
        auto rd = godot::RenderingServer::get_singleton()->get_rendering_device();
        this->tex_handle = reinterpret_cast<ID3D12Resource *>(rd->get_driver_resource(godot::RenderingDevice::DRIVER_RESOURCE_TEXTURE, texture->rid, 0));

        this->arrived = this->framePool.FrameArrived(auto_revoke, {this, &godot::WindowCaptureTexture::OnFrameArrived});

        godot::print_line("texture initialized ", size.Width, "x", size.Height);
    }
    catch (HRESULT e)
    {
        godot::print_error("well it didnt go good", e);
    }
    return false;
}