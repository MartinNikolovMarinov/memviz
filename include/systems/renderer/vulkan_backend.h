#pragma once

#include <core_system_checks.h>
#include <core_macros.h>
#include <core_assert.h>
#include <core_assert_fmt.h>

#if defined(OS_MAC) && OS_MAC == 1
    #define VK_USE_PLATFORM_METAL_EXT
    #define VK_ENABLE_BETA_EXTENSIONS // need this for VK_KHR_portability_subset
    #include <vulkan/vulkan.h>
#elif defined(OS_WIN) && OS_WIN == 1
    #define VK_USE_PLATFORM_WIN32_KHR
    #define WIN32_LEAN_AND_MEAN // vulkan.h might include windows.h
    #include <vulkan/vulkan.h>
#elif defined(OS_LINUX) && OS_LINUX == 1
    #ifdef USE_X11
        #define VK_USE_PLATFORM_XLIB_KHR
        #include <vulkan/vulkan.h>
    #else
        #define VK_USE_PLATFORM_WAYLAND_KHR
        #include <vulkan/vulkan.h>
    #endif
#endif

#define VK_MUST(...) C_VFUNC(VK_MUST, __VA_ARGS__)
#define VK_MUST1(expr) Panic1((expr) == VK_SUCCESS)
#define VK_MUST2(expr, msg) Panic2((expr) == VK_SUCCESS, msg)
#define VK_MUST_FMT(expr, ...) PanicFmt((expr) == VK_SUCCESS, __VA_ARGS__)
