add_library(gd-capture-external MODULE)
target_include_directories(gd-capture-external PRIVATE ../godot_cpp/include)
target_link_libraries(gd-capture-external PRIVATE godot-cpp)
target_link_options(gd-capture-external PRIVATE -static-libgcc -static-libstdc++)
target_sources(gd-capture-external PRIVATE
	gd-capture-external.cpp
	gd-capture-external.hpp
	register_types.cpp
	register_types.hpp
)
