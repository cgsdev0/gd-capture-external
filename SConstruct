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

# Add Windows Graphics Capture API dependencies
env.Append(CPPDEFINES=["NOMINMAX", "WINRT_LEAN_AND_MEAN"])
env.Append(LIBS=["d3d11", "runtimeobject", "windowsapp"])

# Enable C++17 for WinRT support
if env["platform"] == "windows":
    env.Append(CXXFLAGS=["/std:c++17", "/Zc:twoPhase-", "/await", "/EHsc"])
    # Add options to ignore specific warnings
    env.Append(CXXFLAGS=["/wd4100", "/wd4505", "/wd4834"])

# Get the source files
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

# Determine the platform and target
if env["platform"] == "windows":
    library = env.SharedLibrary(
        "bin/libgodot-window-capture{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )
else:
    print("Platform not supported for WindowCaptureTexture: ", env["platform"])
    library = None

Default(library)