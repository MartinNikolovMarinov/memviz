#include "basic.h"

#include "platform.h"
#include "systems/logger.h"
#include "systems/renderer/renderer.h"
#include <error.h>

using namespace memviz;


bool g_appIsRunning = true;

void registerEventHandlers() {
    Platform::registerWindowCloseCallback([]() {
        logInfoTagged(USER_INPUT_TAG, "Closing Application!");
        g_appIsRunning = false;
    });
    Platform::registerWindowResizeCallback([](i32 w, i32 h) {
        logInfoTagged(USER_INPUT_TAG, "EVENT: WINDOW_RESIZE (w={}, h={})", w, h);
        // Renderer::resizeTarget(w, h);
    });
    Platform::registerWindowFocusCallback([](bool focus) {
        if (focus) logInfoTagged(USER_INPUT_TAG, "EVENT: WINDOW_FOCUS_GAINED");
        else       logInfoTagged(USER_INPUT_TAG, "EVENT: WINDOW_FOCUS_LOST");
    });
    Platform::registerKeyCallback([](u32 vkcode, u32 scancode, bool isPress, KeyboardModifiers mods) {
        logTraceTagged(USER_INPUT_TAG, "EVENT: KEY_{} (vkcode={}, scancode={}, mods={})",
                       isPress ? "PRESS" : "RELEASE", vkcode, scancode, keyModifiersToCptr(mods));
    });
    Platform::registerMouseClickCallback([](MouseButton button, bool isPress, i32 x, i32 y, KeyboardModifiers mods) {
        logTraceTagged(USER_INPUT_TAG, "EVENT: MOUSE_{} (button={}, x={}, y={}, mods={})",
                       isPress ? "PRESS" : "RELEASE", button, x, y, keyModifiersToCptr(mods));
    });
    Platform::registerMouseMoveCallback([](i32 x, i32 y) {
        // very noisy
        logTraceTagged(USER_INPUT_TAG, "EVENT: MOUSE_MOVE (x={}, y={})", x, y);
    });
    Platform::registerMouseScrollCallback([](MouseScrollDirection direction, i32 x, i32 y) {
        logTraceTagged(USER_INPUT_TAG, "EVENT: MOUSE_SCROLL (direction={}, x={}, y={})", direction, x, y);
    });
    Platform::registerMouseEnterOrLeaveCallback([](bool enter) {
        if (enter) logTraceTagged(USER_INPUT_TAG, "EVENT: MOUSE_ENTER");
        else       logTraceTagged(USER_INPUT_TAG, "EVENT: MOUSE_LEAVE");
    });

    logInfo("Registered event handlers SUCCESSFULLY");
}

int main(int, const char**) {
    basicInit();
    defer { basicShutdown(); };

    loggerSystemSetLogLevelToTrace();

    Error initErr = Platform::init("Example", 1280, 720);
    Assert(initErr == Error::OK);
    defer { Platform::shutdown(); };

    Renderer::CrateInfo rinfo = {};
    rinfo.appName = "Example";
    Error renderInit = Renderer::init(std::move(rinfo));
    Assert(renderInit == Error::OK);
    defer { Renderer::shutdown(); };

    registerEventHandlers();

    while (g_appIsRunning) {
        if (Error err = Platform::pollEvents(); err != Error::OK) {
            logFatal("pollEvents failed with err={}", errToCStr(err));
            break;
        }

        // Renderer::drawFrame();
    }

    return 0;
}
