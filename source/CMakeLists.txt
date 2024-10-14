add_library(gd-capture-external MODULE)

find_package(X11 REQUIRED)
find_package(Xcb REQUIRED xcb xcb-xfixes xcb-randr xcb-shm xcb-xinerama xcb-composite)

target_include_directories(gd-capture-external PRIVATE ../godot_cpp/include)
target_link_libraries(gd-capture-external PRIVATE
	godot-cpp
	X11::X11
	xcb::xcb
	xcb::xcb-xfixes
	xcb::xcb-randr
	xcb::xcb-shm
	xcb::xcb-xinerama
	xcb::xcb-composite
)
target_link_options(gd-capture-external PRIVATE -static-libgcc -static-libstdc++)

target_sources(gd-capture-external PRIVATE
	capture-base.cpp
	capture-base.hpp
	x11-display-capture.cpp
	x11-display-capture.hpp
	xhelpers.c
	xhelpers.h
	plugin.cpp
	plugin.hpp
)
