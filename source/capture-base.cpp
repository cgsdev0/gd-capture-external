#include "capture-base.hpp"
#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

/* ========================================================================= */

void Texture2DRDAutoRelease::_bind_methods() {}

Texture2DRDAutoRelease::Texture2DRDAutoRelease(RenderingDevice::DataFormat format,
                                               int                         cx,
                                               int                         cy,
                                               const PackedByteArray      &bytes,
                                               bool                        invert_rb)
{
	Ref<RDTextureFormat> rdf(memnew(RDTextureFormat));
	rdf->set_format(format);
	rdf->set_width(cx);
	rdf->set_height(cy);
	rdf->set_usage_bits(RenderingDevice::TEXTURE_USAGE_CAN_UPDATE_BIT |
	                    RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT);

	Ref<RDTextureView> view(memnew(RDTextureView));

	if (invert_rb) {
		view->set_swizzle_r(RenderingDevice::TEXTURE_SWIZZLE_ZERO);
		view->set_swizzle_g(RenderingDevice::TEXTURE_SWIZZLE_ZERO);
		view->set_swizzle_b(RenderingDevice::TEXTURE_SWIZZLE_ZERO);
		view->set_swizzle_a(RenderingDevice::TEXTURE_SWIZZLE_ZERO);
	}

	TypedArray<PackedByteArray> levels;

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

/* ========================================================================= */

void CaptureBase::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_texture"), &CaptureBase::get_texture);

	ADD_SIGNAL(MethodInfo(
	        "texture_changed",
	        PropertyInfo(
	                Variant::OBJECT, "texture", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT, "Texture2D")));
}

void CaptureBase::create_texture(RenderingDevice::DataFormat format, int cx, int cy, bool invert_rb)
{
	if (this->cx == cx && this->cy == cy)
		return;

	this->cx = cx;
	this->cy = cy;

	bytes.resize(cx * cy * 4); // TODO: FIX THE FORMAT DINGUS
	bytes.fill(0);

	texture = Ref(memnew(Texture2DRDAutoRelease(format, cx, cy, bytes, invert_rb)));
	emit_signal("texture_changed", texture);
}

void CaptureBase::update_texture(const void *data)
{
	RenderingServer *rs = RenderingServer::get_singleton();
	RenderingDevice *rd = rs->get_rendering_device();

	if (rd->texture_is_valid(texture->rid)) {
		memcpy(bytes.ptrw(), data, 4 * cx * cy); // TODO: FIX THE FORMAT DINGUS
		rd->texture_update(texture->rid, 0, bytes);
	}
}

Ref<Texture2DRD> CaptureBase::get_texture() const
{
	return texture;
}
