#ifndef WINDOW_CAPTURE_TEXTURE_H
#define WINDOW_CAPTURE_TEXTURE_H

#include "pch.h"
#include "capture-base.hpp"
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot
{

    class WindowCaptureTexture : public CaptureBase
    {
        GDCLASS(WindowCaptureTexture, CaptureBase);

    private:
        HWND target_hwnd = nullptr;
        winrt::com_ptr<ID3D11DeviceContext> d3dContext{nullptr};
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
        winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{nullptr};
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool{nullptr};
        winrt::com_ptr<IDXGISwapChain1> swapChain{nullptr};
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker arrived{};
        winrt::com_ptr<ID3D11Texture2D> staging_texture{nullptr};
        std::mutex frame_mutex;
        bool frame_ready = false;

        void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender, winrt::Windows::Foundation::IInspectable const &);

    protected:
        static void
        _bind_methods();

    public:
        WindowCaptureTexture();
        ~WindowCaptureTexture();

        // Custom methods
        void _process(double delta) override;
        bool start_capture(int64_t hwnd);
    };
}

#endif // WINDOW_CAPTURE_TEXTURE_H