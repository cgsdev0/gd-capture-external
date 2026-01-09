#include "window_capture_texture.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

godot::WindowCaptureTexture::WindowCaptureTexture() {}

godot::WindowCaptureTexture::~WindowCaptureTexture() {
  stop_capture();
  texture.unref();
}

void godot::WindowCaptureTexture::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("start_capture", "hwnd"),
                              &WindowCaptureTexture::start_capture);
  godot::ClassDB::bind_method(godot::D_METHOD("stop_capture"),
                              &WindowCaptureTexture::stop_capture);
  godot::ClassDB::bind_method(
      godot::D_METHOD("set_mouse_capture", "should_capture"),
      &WindowCaptureTexture::set_mouse_capture);
  godot::ClassDB::bind_method(godot::D_METHOD("get_mouse_capture"),
                              &WindowCaptureTexture::get_mouse_capture);
  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "mouse_capture"),
               "set_mouse_capture", "get_mouse_capture");
}

void godot::WindowCaptureTexture::set_mouse_capture(bool should_capture) {
  this->should_capture_cursor = should_capture;
}

bool godot::WindowCaptureTexture::get_mouse_capture() {
  return this->should_capture_cursor;
}

void godot::WindowCaptureTexture::_process(double delta) {}

void godot::WindowCaptureTexture::stop_capture() {}

static bool is_empty(uint8_t *bytes, unsigned int sample) { return true; }

bool godot::WindowCaptureTexture::start_capture(int64_t hwnd) {}
