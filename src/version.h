#pragma once

// Single source of truth for the plugin version.
// Update here; version.rc and Main.cpp both include this file.
#define VER_MAJOR 1
#define VER_MINOR 0
#define VER_PATCH 1
#define VER_BUILD 0

// String forms required by the VERSIONINFO resource block, derived from the
// numeric macros above (two-step expansion is needed to stringize macro values).
#define VER_STRINGIZE2(x) #x
#define VER_STRINGIZE(x)  VER_STRINGIZE2(x)

#define VER_FILE_STR \
    VER_STRINGIZE(VER_MAJOR) "." VER_STRINGIZE(VER_MINOR) "." VER_STRINGIZE(VER_PATCH) "." VER_STRINGIZE(VER_BUILD)
#define VER_PRODUCT_STR \
    VER_STRINGIZE(VER_MAJOR) "." VER_STRINGIZE(VER_MINOR) "." VER_STRINGIZE(VER_PATCH)
