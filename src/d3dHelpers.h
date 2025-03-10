#ifndef D3D_HELPERS_H
#define D3D_HELPERS_H

struct D3D11DeviceLock
{
public:
    D3D11DeviceLock(std::nullopt_t) {}
    D3D11DeviceLock(ID3D11Multithread *pMultithread)
    {
        m_multithread.copy_from(pMultithread);
        m_multithread->Enter();
    }
    ~D3D11DeviceLock()
    {
        m_multithread->Leave();
        m_multithread = nullptr;
    }

private:
    winrt::com_ptr<ID3D11Multithread> m_multithread;
};

inline auto
CreateWICFactory()
{
    winrt::com_ptr<IWICImagingFactory2> wicFactory;
    winrt::check_hresult(
        ::CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            winrt::guid_of<IWICImagingFactory>(),
            wicFactory.put_void()));

    return wicFactory;
}

inline auto
CreateD2DDevice(
    winrt::com_ptr<ID2D1Factory1> const &factory,
    winrt::com_ptr<ID3D11Device> const &device)
{
    winrt::com_ptr<ID2D1Device> result;
    winrt::check_hresult(factory->CreateDevice(device.as<IDXGIDevice>().get(), result.put()));
    return result;
}

inline auto
CreateD3DDevice(
    D3D_DRIVER_TYPE const type,
    winrt::com_ptr<ID3D11Device> &device)
{
    WINRT_ASSERT(!device);

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    // #ifdef _DEBUG
    //	flags |= D3D11_CREATE_DEVICE_DEBUG;
    // #endif
    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};

    return D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, // IMPORTANT for SpoutDX
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        device.put(),
        &featureLevel,
        nullptr);
}

inline auto
CreateD3DDevice()
{
    winrt::com_ptr<ID3D11Device> device;
    HRESULT hr = CreateD3DDevice(D3D_DRIVER_TYPE_HARDWARE, device);

    if (DXGI_ERROR_UNSUPPORTED == hr)
    {
        hr = CreateD3DDevice(D3D_DRIVER_TYPE_WARP, device);
    }

    winrt::check_hresult(hr);
    return device;
}

inline auto
CreateD2DFactory()
{
    D2D1_FACTORY_OPTIONS options{};

    // #ifdef _DEBUG
    //	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
    // #endif

    winrt::com_ptr<ID2D1Factory1> factory;

    winrt::check_hresult(D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        options,
        factory.put()));

    return factory;
}

inline auto
CreateDXGISwapChain(
    winrt::com_ptr<ID3D11Device> const &device,
    const DXGI_SWAP_CHAIN_DESC1 *desc)
{
    auto dxgiDevice = device.as<IDXGIDevice2>();
    winrt::com_ptr<IDXGIAdapter> adapter;
    winrt::check_hresult(dxgiDevice->GetParent(winrt::guid_of<IDXGIAdapter>(), adapter.put_void()));
    winrt::com_ptr<IDXGIFactory2> factory;
    winrt::check_hresult(adapter->GetParent(winrt::guid_of<IDXGIFactory2>(), factory.put_void()));

    winrt::com_ptr<IDXGISwapChain1> swapchain;
    winrt::check_hresult(factory->CreateSwapChainForComposition(
        device.get(),
        desc,
        nullptr,
        swapchain.put()));

    return swapchain;
}

inline auto
CreateDXGISwapChain(
    winrt::com_ptr<ID3D11Device> const &device,
    uint32_t width,
    uint32_t height,
    DXGI_FORMAT format,
    uint32_t bufferCount)
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Format = format;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferCount = bufferCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    return CreateDXGISwapChain(device, &desc);
}

#include <d3d11.h>
#include <d3d12.h>
#include <d3d11on12.h>
#include <memory>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

struct CommandQueueInfo
{
    winrt::com_ptr<ID3D12CommandQueue> d3d_queue;
};

class D3D11DeviceSingleton
{
public:
    static D3D11DeviceSingleton &get_instance()
    {
        static D3D11DeviceSingleton instance;
        return instance;
    }

    ID3D12CommandQueue *get_command_queue()
    {
        initialize();
        return command_queue;
    }

    ID3D11Device *get_device()
    {
        initialize();
        return d3d11Device.get();
    }

    ID3D12Device *get_device12()
    {
        initialize();
        return d3d12Device;
    }

    ID3D11On12Device *get_device11on12()
    {
        initialize();
        return d3d11on12Device;
    }

    void initialize()
    {
        if (d3d11Device)
        {
            return; // Already initialized
        }

        auto rd = godot::RenderingServer::get_singleton()->get_rendering_device();
        this->d3d12Device = reinterpret_cast<ID3D12Device *>(rd->get_driver_resource(godot::RenderingDevice::DRIVER_RESOURCE_LOGICAL_DEVICE, godot::RID(), 0));

        auto q = reinterpret_cast<CommandQueueInfo *>(rd->get_driver_resource(godot::RenderingDevice::DRIVER_RESOURCE_COMMAND_QUEUE, godot::RID(), 0));
        this->command_queue = q->d3d_queue.get();
        D3D11On12CreateDevice(
            d3d12Device,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr, 0,
            nullptr, 0,
            0,
            d3d11Device.put(),
            d3d11Context.put(),
            nullptr);

        d3d11Device->QueryInterface(__uuidof(ID3D11On12Device), (void **)&this->d3d11on12Device);
    }

private:
    D3D11DeviceSingleton() = default;
    ~D3D11DeviceSingleton()
    {
        // Cleanup device if needed (automatically managed by smart pointers)
    }

    // Prevent copying and assignment
    D3D11DeviceSingleton(const D3D11DeviceSingleton &) = delete;
    D3D11DeviceSingleton &operator=(const D3D11DeviceSingleton &) = delete;
    ID3D11On12Device *d3d11on12Device = nullptr;
    ID3D12Device *d3d12Device = nullptr;
    ID3D12CommandQueue *command_queue = nullptr;
    winrt::com_ptr<ID3D11Device> d3d11Device;
    winrt::com_ptr<ID3D11DeviceContext> d3d11Context;
};

#endif