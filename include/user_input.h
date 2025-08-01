#pragma once

// IMPORTANT: Reduce the number of imports here since this header is imported in platform.

#include "systems/logger.h"
#include <core_types.h>

namespace memviz {

using namespace coretypes;

enum struct KeyboardModifiers : u8 {
    MODNONE = 0,
    MODSHIFT = 1 << 0,
    MODCONTROL = 1 << 1,
    MODALT = 1 << 2,
    MODSUPER = 1 << 3,
};

inline constexpr KeyboardModifiers operator|(KeyboardModifiers lhs, KeyboardModifiers rhs) {
    return KeyboardModifiers(u8(lhs) | u8(rhs));
}

inline constexpr KeyboardModifiers operator&(KeyboardModifiers lhs, KeyboardModifiers rhs) {
    return KeyboardModifiers(u8(lhs) & u8(rhs));
}

inline constexpr KeyboardModifiers operator^(KeyboardModifiers lhs, KeyboardModifiers rhs) {
    return KeyboardModifiers(u8(lhs) ^ u8(rhs));
}

// Define compound assignment operators
inline constexpr KeyboardModifiers& operator|=(KeyboardModifiers& lhs, KeyboardModifiers rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline constexpr KeyboardModifiers& operator&=(KeyboardModifiers& lhs, KeyboardModifiers rhs) {
    lhs = lhs & rhs;
    return lhs;
}

inline constexpr KeyboardModifiers& operator^=(KeyboardModifiers& lhs, KeyboardModifiers rhs) {
    lhs = lhs ^ rhs;
    return lhs;
}

constexpr const char* keyModifiersToCptr(KeyboardModifiers m) {
    using KeyboardModifiers::MODNONE;
    using KeyboardModifiers::MODSHIFT;
    using KeyboardModifiers::MODCONTROL;
    using KeyboardModifiers::MODALT;
    using KeyboardModifiers::MODSUPER;

    if (m == MODNONE) return "None";

    if (m == (MODSHIFT | MODCONTROL | MODALT | MODSUPER))  return "Shift + Control + Alt + Super";
    if (m == (MODSHIFT | MODCONTROL | MODALT))             return "Shift + Control + Alt";
    if (m == (MODSHIFT | MODCONTROL | MODSUPER))           return "Shift + Control + Super";
    if (m == (MODSHIFT | MODALT | MODSUPER))               return "Shift + Alt + Super";
    if (m == (MODCONTROL | MODALT | MODSUPER))             return "Control + Alt + Super";

    if (m == (MODSHIFT | MODCONTROL)) return "Shift + Control";
    if (m == (MODSHIFT | MODALT))     return "Shift + Alt";
    if (m == (MODSHIFT | MODSUPER))   return "Shift + Super";
    if (m == (MODCONTROL | MODALT))   return "Control + Alt";
    if (m == (MODCONTROL | MODSUPER)) return "Control + Super";
    if (m == (MODALT | MODSUPER))     return "Alt + Super";

    if (m == MODSHIFT)   return "Shift";
    if (m == MODCONTROL) return "Control";
    if (m == MODALT)     return "Alt";
    if (m == MODSUPER)   return "Super";

    return "Unknown";
}

enum struct MouseButton : u8 {
    NONE,
    LEFT,
    MIDDLE,
    RIGHT,
    SENTINEL
};

enum struct MouseScrollDirection : u8 {
    NONE,
    UP,
    DOWN,
    SENTINEL
};

} // namesapce memviz
