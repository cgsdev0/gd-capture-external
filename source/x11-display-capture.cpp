#include "x11-display-capture.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

void X11DisplayCapture::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_index", "index"), &X11DisplayCapture::set_index);
}

X11DisplayCapture::~X11DisplayCapture()
{
	capture_stop();
}

void X11DisplayCapture::capture_stop()
{
	texture.unref();

	/*if (cursor) {
	        xcb_xcursor_destroy(cursor);
	        cursor = nullptr;
	}*/

	if (xshm) {
		xshm_xcb_detach(xshm);
		xshm = nullptr;
	}

	if (xcb) {
		xcb_disconnect(xcb);
		xcb = nullptr;
	}
}

static bool xshm_check_extensions(xcb_connection_t *xcb)
{
	bool ok = true;
	if (!xcb_get_extension_data(xcb, &xcb_shm_id)->present)
		ok = false;
	xcb_get_extension_data(xcb, &xcb_xinerama_id);
	xcb_get_extension_data(xcb, &xcb_randr_id);
	return ok;
}

bool X11DisplayCapture::update_geometry()
{
	int32_t prev_cx = cx;
	int32_t prev_cy = cy;

	if (use_randr) {
		if (randr_screen_geo(xcb, screen_id, &x_org, &y_org, &cx, &cy, &xcb_screen, NULL) < 0) {
			return false;
		}
	} else if (use_xinerama) {
		if (xinerama_screen_geo(xcb, screen_id, &x_org, &y_org, &cx, &cy) < 0) {
			return false;
		}
		xcb_screen = xcb_get_screen(xcb, 0);
	} else {
		x_org = 0;
		y_org = 0;
		if (x11_screen_geo(xcb, screen_id, &cy, &cx) < 0) {
			return false;
		}
		xcb_screen = xcb_get_screen(xcb, screen_id);
	}

	if (!cx || !cy) {
		godot::UtilityFunctions::print("Failed to get geometry");
		return false;
	}

	return true;
}

void X11DisplayCapture::set_index(int index)
{
	try {
		if (screen_id == index)
			return;

		screen_id = index;
		if (screen_id == -1) {
			texture.unref();
			return;
		}

		xcb = xcb_connect(server.c_str(), nullptr);
		if (!xcb || xcb_connection_has_error(xcb))
			throw "unable to open X display";

		if (!xshm_check_extensions(xcb))
			throw "failed to find any supported screen capture extensions";

		use_randr    = randr_is_active(xcb) ? true : false;
		use_xinerama = xinerama_is_active(xcb) ? true : false;

		if (!update_geometry())
			throw "failed to update geometry";

		xshm = xshm_xcb_attach(xcb, cx, cy);
		if (!xshm)
			throw "failed to attach shm";

		/*cursor = xcb_xcursor_init(xcb);
		xcb_xcursor_offset(cursor, 0, 0);*/

		create_texture(RenderingDevice::DATA_FORMAT_B8G8R8A8_SRGB, cx, cy, true);

	} catch (const char *error) {
		godot::UtilityFunctions::print(__FUNCTION__, ": ", error);
		capture_stop();
	}
}

void X11DisplayCapture::_process(double delta)
{
	if (!texture.is_valid())
		return;

	xcb_shm_get_image_cookie_t img_c;
	xcb_shm_get_image_reply_t *img_r;

	img_c = xcb_shm_get_image_unchecked(
	        xcb, xcb_screen->root, x_org, y_org, cx, cy, ~0, XCB_IMAGE_FORMAT_Z_PIXMAP, xshm->seg, 0);

	img_r = xcb_shm_get_image_reply(xcb, img_c, NULL);

	if (!img_r)
		return;

	update_texture(xshm->data);

	::free(img_r);
}

int X11DisplayCapture::get_index() const
{
	return screen_id;
}

TypedArray<Dictionary> X11DisplayCapture::enum_displays()
{
	return TypedArray<Dictionary>();
}

} // namespace godot
