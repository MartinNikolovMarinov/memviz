#pragma once

#include <core_types.h>
#include <error.h>

#if defined(MEMVIZ_USE_VULKAN)
    #include "vulkan_backend.h"
#endif

namespace memviz {

using namespace coretypes;

struct Renderer {
    struct CrateInfo {};

    static Error (*init)(CrateInfo&& info);
};

} // memviz

