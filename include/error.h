#pragma once

#include "core_types.h"
#include "error_lists.h"

namespace memviz {

using namespace coretypes;

enum struct Error {
    OK,

#define MEMVIZ_PLT_ERROR_ITEM(name, msg) name,
    MEMVIZ_X11_ERROR_LIST
#undef MEMVIZ_PLT_ERROR_ITEM

#define MEMVIZ_PLT_ERROR_ITEM(name, msg) name,
    MEMVIZ_VULKAN_ERROR_LIST
#undef MEMVIZ_PLT_ERROR_ITEM

    SENTINEL
};

constexpr inline bool operator==(Error a, Error b) {
    return static_cast<i32>(a) == static_cast<i32>(b);
}

constexpr inline bool operator!=(Error a, Error b) {
    return !(a == b);
}

constexpr inline const char* errToCStr(Error err) {
    switch (err) {
        case Error::OK: return "ok";

#define MEMVIZ_PLT_ERROR_ITEM(name, msg) case Error::name: return msg;
        MEMVIZ_X11_ERROR_LIST
#undef MEMVIZ_PLT_ERROR_ITEM

#define MEMVIZ_PLT_ERROR_ITEM(name, msg) case Error::name: return msg;
        MEMVIZ_VULKAN_ERROR_LIST
#undef MEMVIZ_PLT_ERROR_ITEM

        case Error::SENTINEL: [[fallthrough]];
        default:
            return "unknown";
    }
}

} // namespace memviz
