#ifndef WINDOW_CAPTURE_TEXTURE_H
#define WINDOW_CAPTURE_TEXTURE_H

#include "pch.h"
#include "capture-base.hpp"
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <atomic>

namespace godot
{

    class WindowCaptureTexture : public CaptureBase
    {
        GDCLASS(WindowCaptureTexture, CaptureBase);

    private:
        HWND target_hwnd = nullptr;
        winrt::com_ptr<ID3D11DeviceContext> d3dContext{nullptr};
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice directDevice{nullptr};
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
        winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{nullptr};
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool{nullptr};
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker arrived{};
        winrt::com_ptr<ID3D11Texture2D> staging_texture{nullptr};
        std::mutex frame_mutex;
        bool frame_ready = false;

        int _cx = 0;
        int _cy = 0;

        std::atomic<bool> closed = true;

        void OnFrameArrived(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const &sender, winrt::Windows::Foundation::IInspectable const &);
        bool should_capture_cursor = false;

    protected:
        static void
        _bind_methods();

    public:
        WindowCaptureTexture();
        ~WindowCaptureTexture();

        // Custom methods
        void _process(double delta) override;
        bool start_capture(int64_t hwnd);
        void set_mouse_capture(bool should_capture);
        bool get_mouse_capture();
        void stop_capture();
    };
}

#endif // WINDOW_CAPTURE_TEXTURE_H