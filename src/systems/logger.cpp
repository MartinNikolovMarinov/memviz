#include "basic.h"
#include "systems/logger.h"

namespace memviz {

void loggerSystemInit() {
    for (i32 i = i32(LogTag::ALL_TAG) + 1; i < i32(LogTag::SENTINEL); i++) {
        auto tagsv = core::sv(logTagToCStr(LogTag(i)));
        bool ok = core::loggerSetTag(i, tagsv);
        AssertFmt(ok, "Failed to set logger tag '{}'", i);
    }
}

void loggerSystemShutdown() {
    core::loggerDestroy();
}

void loggerSystemSetLogLevelToTrace() { core::loggerSetLevel(core::LogLevel::L_TRACE); }
void loggerSystemSetLogLevelToDebug() { core::loggerSetLevel(core::LogLevel::L_DEBUG); }
void loggerSystemSetLogLevelToInfo() { core::loggerSetLevel(core::LogLevel::L_INFO); }
void loggerSystemSetLogLevelToWarning() { core::loggerSetLevel(core::LogLevel::L_WARNING); }
void loggerSystemSetLogLevelToError() { core::loggerSetLevel(core::LogLevel::L_ERROR); }
void loggerSystemSetLogLevelToFatal() { core::loggerSetLevel(core::LogLevel::L_FATAL); }

core::LoggerCreateInfo loggerSystemCreateInfo() {
    core::LoggerCreateInfo ret = core::LoggerCreateInfo::createDefault();
    ret.useAnsi = true;
    ret.allocatorId = 0;
    return ret;
}

void __debug__testLoggerSetup() {
#if defined(MEMVIZ_DEBUG) && MEMVIZ_DEBUG == 1

    core::loggerSetLevel(core::LogLevel::L_TRACE);

    auto printTest = [](){
        logTrace("text={}, f32={:f.2}, f64={:f.2}, f32={}, f64={}, int={^1:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logDebug("text={}, f32={:f.3}, f64={:f.3}, f32={}, f64={}, int={^2:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logInfo("text={}, f32={:f.4}, f64={:f.4}, f32={}, f64={}, int={^3:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logWarn("text={}, f32={:f.5}, f64={:f.5}, f32={}, f64={}, int={^4:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logErr("text={}, f32={:f.6}, f64={:f.6}, f32={}, f64={}, int={^5:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logFatal("text={}, f32={:f.7}, f64={:f.7}, f32={}, f64={}, int={^6:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
    };

    auto printTestTagged = [](u8 tag){
        logTraceTagged(tag,
            "text={}, f32={:f.2}, f64={:f.2}, f32={}, f64={}, int={^1:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logDebugTagged(tag,
                "text={}, f32={:f.3}, f64={:f.3}, f32={}, f64={}, int={^2:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logInfoTagged(tag,
                "text={}, f32={:f.4}, f64={:f.4}, f32={}, f64={}, int={^3:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logWarnTagged(tag,
            "text={}, f32={:f.5}, f64={:f.5}, f32={}, f64={}, int={^4:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logErrTagged(tag,
            "text={}, f32={:f.6}, f64={:f.6}, f32={}, f64={}, int={^5:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
        logFatalTagged(tag,
                "text={}, f32={:f.7}, f64={:f.7}, f32={}, f64={}, int={^6:}, hex={:H}",
                "Hey", 1.123456f, 7.9999999, 512.f, 123.451235512, i32(541235), i8(255));
    };

    // Log everyting with and without ansi support.
    {
        logSectionTitleInfoTagged(0, "Test with ANSI");
        printTest();
        core::loggerUseANSI(false);
        defer { core::loggerUseANSI(true); };
        logSectionTitleInfoTagged(0, "Test without ANSI");
        printTest();
    }

    // Test the level setting
    {
        core::loggerSetLevel(core::LogLevel::L_WARNING);
        defer { core::loggerSetLevel(core::LogLevel::L_TRACE); };
        logSectionTitleWarnTagged(0, "Should only log warning and above");
        printTest();
    }

    // Test tag
    {
        logSectionTitleInfoTagged(i8(LogTag::PLATFORM_TAG), "Test printing PLATFORM tag");
        printTestTagged(i8(LogTag::PLATFORM_TAG));

        logSectionTitleInfoTagged(i8(LogTag::PLATFORM_TAG), "Test muting the PLATFORM tag");
        core::loggerMuteTag(i8(LogTag::PLATFORM_TAG), true);
        printTestTagged(i8(LogTag::PLATFORM_TAG)); // nothing should get printed
        printTestTagged(i8(LogTag::ALL_TAG)); // default should get printed

        core::loggerMuteTag(i8(LogTag::PLATFORM_TAG), false);
        logSectionTitleInfoTagged(i8(LogTag::PLATFORM_TAG), "Test un-muting the PLATFORM tag");
        printTestTagged(i8(LogTag::PLATFORM_TAG));
    }

    // Muting all tags
    {
        // First just use the mute api to mute all output from the logger
        logSectionTitleInfoTagged(i8(LogTag::ALL_TAG), "After muting nothing is printed");
        core::loggerMute(true);
        printTestTagged(i8(LogTag::ALL_TAG));
        printTestTagged(i8(LogTag::PLATFORM_TAG));
        core::loggerMute(false);

        // Edge case mute everyhing that is not tagged (this is sometimes useful).
        core::loggerMuteTag(i8(LogTag::ALL_TAG), true);
        logSectionTitleInfoTagged(i8(LogTag::PLATFORM_TAG), "Test with ALL untagged muted");
        printTestTagged(i8(LogTag::ALL_TAG));
        printTestTagged(i8(LogTag::PLATFORM_TAG));

        core::loggerMuteTag(i8(LogTag::ALL_TAG), false);
        logSectionTitleInfoTagged(i8(LogTag::ALL_TAG), "Un-muting everyting back to normal");
        printTestTagged(i8(LogTag::ALL_TAG));
    }
#endif
}

} // memviz
