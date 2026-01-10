#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "capture-base.hpp"
#include "window_capture_texture.h"
#include "window-poller.h"

void initialize_godot_window_capture_module(godot::ModuleInitializationLevel p_level)
{
    if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    godot::ClassDB::register_class<godot::Texture2DRDAutoRelease>();
    godot::ClassDB::register_class<godot::CaptureBase>();
    godot::ClassDB::register_class<godot::WindowCaptureTexture>();
    godot::ClassDB::register_class<godot::WindowPoller>();
}

void uninitialize_godot_window_capture_module(godot::ModuleInitializationLevel p_level)
{
    if (p_level != godot::MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
}

extern "C"
{
    GDExtensionBool GDE_EXPORT godot_window_capture_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
    {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_godot_window_capture_module);
        init_obj.register_terminator(uninitialize_godot_window_capture_module);

        init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}