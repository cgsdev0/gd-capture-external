#ifndef GODOT_WINDOW_CAPTURE_REGISTER_TYPES_H
#define GODOT_WINDOW_CAPTURE_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

void initialize_godot_window_capture_module(godot::ModuleInitializationLevel p_level);
void uninitialize_godot_window_capture_module(godot::ModuleInitializationLevel p_level);

extern "C"
{
    GDExtensionBool GDE_EXPORT godot_window_capture_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization);
}

#endif // GODOT_WINDOW_CAPTURE_REGISTER_TYPES_H