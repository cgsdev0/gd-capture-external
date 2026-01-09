#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags


# Get the source files
env.Append(CPPPATH=["src/"])
env.Append(LIBS=["wayland-client"])
sources = Glob("src/*.cpp")
c_sources   = Glob("src/*.c")

# Determine the platform and target
library = env.SharedLibrary(
    "bin/libgodot-window-capture{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources  + c_sources,
)

Default(library)
