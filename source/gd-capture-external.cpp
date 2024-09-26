#include "gd-capture-external.hpp"
#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>

using namespace godot;

void Texture2DRDAutoRelease::_bind_methods() {}

Texture2DRDAutoRelease::Texture2DRDAutoRelease(RenderingDevice::DataFormat format, int cx, int cy)
{
	PackedByteArray bytes;
	bytes.resize(cx * cy * 4); // TODO: FIX THE FORMAT DINGUS
	bytes.fill(0);

	Ref<RDTextureFormat> rdf(memnew(RDTextureFormat));
	rdf->set_format(format);
	rdf->set_width(cx);
	rdf->set_height(cy);
	rdf->set_usage_bits(RenderingDevice::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
	                    RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT);

	Ref<RDTextureView> view(memnew(RDTextureView));

	TypedArray<PackedByteArray> levels;
	levels.resize(1);
	levels[0] = std::move(bytes);

	RenderingServer *rs = RenderingServer::get_singleton();
	RenderingDevice *rd = rs->get_rendering_device();
	rid                 = rd->texture_create(rdf, view, levels);

	set_texture_rd_rid(rid);
}

Texture2DRDAutoRelease::~Texture2DRDAutoRelease()
{
	RenderingServer *rs = RenderingServer::get_singleton();
	RenderingDevice *rd = rs->get_rendering_device();
	if (rd->texture_is_valid(rid))
		rd->free_rid(rid);
}

void Test::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_texture"), &Test::get_texture);
}

Test::Test()
{
	//texture = Ref(memnew(Texture2DRDAutoRelease(RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM, 16, 16)));
}

void Test::_process(double delta) {}

Ref<Texture2DRD> Test::get_texture() const
{
	return texture;
}
