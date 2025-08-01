#pragma once

// IMPORTANT: The platform layer should not use basic.h or anything from core since there are collisions with some of
//            the platforms internal definitions (like defer) on MacOS.

#include <core_types.h>

#include "error.h"
#include "user_input.h"

namespace memviz {

using namespace coretypes;

typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

using WindowCloseCallback = void (*)();
using WindowResizeCallback = void (*)(i32 w, i32 h);
using WindowFocusCallback = void (*)(bool gain);

using KeyCallback = void (*)(u32 vkcode, u32 scancode, bool isPress, KeyboardModifiers mods);

using MouseClickCallback = void (*)(MouseButton button, bool isPress, i32 x, i32 y, KeyboardModifiers mods);
using MouseMoveCallback = void (*)(i32 x, i32 y);
using MouseScrollCallback = void (*)(MouseScrollDirection direction, i32 x, i32 y);
using MouseEnterOrLeaveCallback = void (*)(bool enter);

struct Platform {
    static void registerWindowCloseCallback(WindowCloseCallback cb);
    static void registerWindowResizeCallback(WindowResizeCallback cb);
    static void registerWindowFocusCallback(WindowFocusCallback cb);

    static void registerKeyCallback(KeyCallback cb);

    static void registerMouseClickCallback(MouseClickCallback cb);
    static void registerMouseMoveCallback(MouseMoveCallback cb);
    static void registerMouseScrollCallback(MouseScrollCallback cb);
    static void registerMouseEnterOrLeaveCallback(MouseEnterOrLeaveCallback cb);

    [[nodiscard]] static Error init(const char* windowTitle, i32 windowWidth, i32 windowHeight);
    [[nodiscard]] static Error pollEvents(bool block = false);
    static void shutdown();

    bool getFrameBufferSize(u32& width, u32& height);

    static void requiredVulkanExtsCount(i32& count);
    static void requiredVulkanExts(const char** extensions);
    [[nodiscard]] static Error createVulkanSurface(VkInstance instance, VkSurfaceKHR& surface);
};

} // namespace memviz

