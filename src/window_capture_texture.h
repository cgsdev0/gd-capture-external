#ifndef WINDOW_CAPTURE_TEXTURE_H
#define WINDOW_CAPTURE_TEXTURE_H

#include "capture-base.hpp"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {

class WindowCaptureTexture : public CaptureBase {
  GDCLASS(WindowCaptureTexture, CaptureBase);

private:
  int _cx = 0;
  int _cy = 0;

  bool should_capture_cursor = false;

protected:
  static void _bind_methods();

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
} // namespace godot

#endif // WINDOW_CAPTURE_TEXTURE_H
