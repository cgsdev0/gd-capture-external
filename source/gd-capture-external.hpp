#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/texture2drd.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

namespace godot {

class Texture2DRDAutoRelease : public Texture2DRD {
	GDCLASS(Texture2DRDAutoRelease, Texture2DRD)

protected:
	static void _bind_methods();

public:
	inline Texture2DRDAutoRelease() {}
	Texture2DRDAutoRelease(RenderingDevice::DataFormat format, int cx, int cy);
	~Texture2DRDAutoRelease();

	RID rid;
};

class Test : public Node {
	GDCLASS(Test, Node)

	Ref<Texture2DRDAutoRelease> texture;

protected:
	static void _bind_methods();

public:
	Test();

	void _process(double delta) override;

	Ref<Texture2DRD> get_texture() const;
};

} // namespace godot
