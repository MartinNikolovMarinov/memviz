#pragma once

// IMPORTANT: Reduce the number of imports here since this header is imported in platform.

#include "core_assert_fmt.h"
#include "core_types.h"

namespace memviz {

using namespace coretypes;

enum LogTag : i32 {
    ALL_TAG = 0,

    PLATFORM_TAG = 1,
    USER_INPUT_TAG = 2,
    RENDERER_TAG = 3,

    SENTINEL
};

constexpr const char* logTagToCStr(LogTag t) {
    switch (t) {
        case LogTag::ALL_TAG:        return "ALL";
        case LogTag::PLATFORM_TAG:   return "PLATFORM";
        case LogTag::USER_INPUT_TAG: return "USER_INPUT";
        case LogTag::RENDERER_TAG:   return "RENDERER";

        case LogTag::SENTINEL: [[fallthrough]];
        default:
            AssertFmt(false, "Invalid tag {}", i32(t));
    }

    return nullptr;
}

core::LoggerCreateInfo loggerSystemCreateInfo();

void loggerSystemInit();
void loggerSystemShutdown();

void loggerSystemSetLogLevelToTrace();
void loggerSystemSetLogLevelToDebug();
void loggerSystemSetLogLevelToInfo();
void loggerSystemSetLogLevelToWarning();
void loggerSystemSetLogLevelToError();
void loggerSystemSetLogLevelToFatal();

void __debug__testLoggerSetup();

} // memviz
