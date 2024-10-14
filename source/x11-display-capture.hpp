#pragma once

#include "capture-base.hpp"

#include <string>

#include <xcb/xproto.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>

#include "xhelpers.h"

namespace godot {

class X11DisplayCapture : public CaptureBase {
	GDCLASS(X11DisplayCapture, CaptureBase)

	std::string       server;
	xcb_connection_t *xcb          = nullptr;
	xcb_screen_t     *xcb_screen   = nullptr;
	xcb_shm_t        *xshm         = nullptr;
	int               screen_id    = -1;
	int               x_org        = 0;
	int               y_org        = 0;
	int               cx           = 0;
	int               cy           = 0;
	bool              use_xinerama = false;
	bool              use_randr    = false;
	// TODO
	// xcb_xcursor_t    *cursor;

	void capture_stop();
	bool update_geometry();

protected:
	static void _bind_methods();

public:
	~X11DisplayCapture();

	TypedArray<Dictionary> enum_displays();

	void _process(double delta) override;

	void set_index(int index);
	int  get_index() const;
};

} // namespace godot
