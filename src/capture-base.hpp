#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture2drd.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

namespace godot {

class Texture2DRDAutoRelease : public Texture2DRD {
  GDCLASS(Texture2DRDAutoRelease, Texture2DRD)

protected:
  static void _bind_methods();

public:
  inline Texture2DRDAutoRelease() {}
  Texture2DRDAutoRelease(RenderingDevice::DataFormat format, int cx, int cy,
                         const PackedByteArray &bytes, bool invert_rb);
  ~Texture2DRDAutoRelease();

  RID rid;
};

class CaptureBase : public Node {
  GDCLASS(CaptureBase, Node)

protected:
  Ref<Texture2DRDAutoRelease> texture;
  PackedByteArray bytes;
  int cx = 0;
  int cy = 0;

  static void _bind_methods();

  void create_texture(RenderingDevice::DataFormat format, int cx, int cy,
                      bool invert_rb);

public:
  Ref<Texture2DRD> get_texture() const;
};

} // namespace godot
