#pragma once

#pragma once

#include "plugin.hpp"
#include "capture-base.hpp"

#include <string>

#include <xcb/xproto.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xfixes.h>
#include <xcb/xinerama.h>

#include "xhelpers.h"

namespace godot {

class X11WindowCapture : public CaptureBase {
	GDCLASS(X11WindowCapture, CaptureBase)

protected:
	static void _bind_methods();

public:
	~X11WindowCapture();

	void _process(double delta) override;
};

} // namespace godot
