#include "basic.h"
#include "platform.h"

#include "systems/logger.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <error.h>

namespace memviz {

namespace {

Display* g_display = nullptr;
Window g_window = 0;
Atom g_wmDeleteWindow;
[[maybe_unused]] bool g_initialized = false; // Probably won't use this in release builds.

WindowCloseCallback windowCloseCallbackX11 = nullptr;
WindowResizeCallback windowResizeCallbackX11 = nullptr;
WindowFocusCallback windowFocusCallbackX11 = nullptr;

KeyCallback keyCallbackX11 = nullptr;

MouseClickCallback mouseClickCallbackX11 = nullptr;
MouseMoveCallback mouseMoveCallbackX11 = nullptr;
MouseScrollCallback mouseScrollCallbackX11 = nullptr;
MouseEnterOrLeaveCallback mouseEnterOrLeaveCallbackX11 = nullptr;

i32 handleXError(Display* display, XErrorEvent* errorEvent);

} // namespace

Error Platform::init(const char *windowTitle, i32 windowWidth, i32 windowHeight) {
    logInfoTagged(
    LogTag::PLATFORM_TAG,
    "Starting X11 platform initialization: title='{}', width={}, height={}",
        windowTitle, windowWidth, windowHeight
    );

    // From the Vulkan Specification:
    // Some implementations may require threads to implement some presentation modes so applications must call
    // XInitThreads() before calling any other Xlib functions.
    if (!XInitThreads()) {
        return Error::FAILED_TO_INITIALIZE_X11_THREADS;
    }

    XSetErrorHandler(handleXError);

    // Open a connection to the X server, which manages the display (i.e., the screen).
    g_display = XOpenDisplay(nullptr);
    if (!g_display) {
        return Error::FAILED_TO_CREATE_X11_DISPLAY;
    }

    // NOTE:
    // If there is a suspected synchronization problem uncomment to make Xlib work synchronously. This degrades
    // performance significantly according to documentation.
    // XSynchronize(g_display, True)

    i32 screen = DefaultScreen(g_display); // TODO2: [MULTI_MONITOR] This won't work great on a multi-monitor setup.

    // The root window is the main top-level window managed by the X server for that screen. New windows are often
    // created as children of the root window. I don't know if I will ever need multi-window support.
    Window root = RootWindow(g_display, screen);

    // Creates a basic top-level window.
    g_window = XCreateSimpleWindow(
        g_display,
        root,                                // Parent window is the root
        10, 10,                              // The x and y coordinates of the window’s position on the screen.
        u32(windowWidth), u32(windowHeight), // Initial width and height
        1,                                   // Border width in pixels.
        BlackPixel(g_display, screen),       // Border color.
        WhitePixel(g_display, screen)        // Background color.
    );
    if (!g_window) {
        return Error::FAILED_TO_CREATE_X11_WINDOW;
    }

    // Requests that the X server report the events associated with the specified event mask.
    constexpr long eventMask =
        ExposureMask |       // Receive expose events (when a portion of the window needs to be redrawn).
        KeyPressMask |       // Receive keyboard press events.
        KeyReleaseMask |     // Key release events.
        ButtonPressMask |    // Mouse button press.
        ButtonReleaseMask |  // Mouse button release.
        PointerMotionMask |  // Mouse movement events.
        EnterWindowMask |    // Mouse enters the window.
        LeaveWindowMask |    // Mouse leaves the window.
        FocusChangeMask |    // Window gains or loses focus.
        StructureNotifyMask; // Window structure changes (resize, close, etc.).
    XSelectInput(g_display, g_window, eventMask);

    // Set the window’s title (visible in the title bar).
    XStoreName(g_display, g_window, windowTitle);

    // Registers the WM_DELETE_WINDOW atom, which is used to handle the window manager’s close button.
    // Atom is basically an ID for a string used by the window manager to handle events.
    g_wmDeleteWindow = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    // Informs the window manager that the application wants to handle the WM_DELETE_WINDOW message.
    XSetWMProtocols(g_display, g_window, &g_wmDeleteWindow, 1);

    // Maps the window on the screen, making it visible.
    XMapWindow(g_display, g_window);
    // Flushes all pending requests to the X server.
    XSync(g_display, True);

    g_initialized = true;

    logInfoTagged(LogTag::PLATFORM_TAG, "X11 platform initialization completed successfully");

    return Error::OK;
}

void Platform::shutdown() {
    g_initialized = false; // Mark the platform as uninitialized

    if (g_window) {
        XDestroyWindow(g_display, g_window);
        g_window = 0;
    }

    // Flush all X events and discard them.
    XSync(g_display, True);

    if (g_display) {
        XCloseDisplay(g_display);
        g_display = nullptr;
    }
}

namespace {

constexpr inline KeyboardModifiers getModifiers (u32 m) {
    KeyboardModifiers ret = KeyboardModifiers::MODNONE;

    if (m & ShiftMask)   ret |= KeyboardModifiers::MODSHIFT;
    if (m & ControlMask) ret |= KeyboardModifiers::MODCONTROL;
    if (m & Mod1Mask)    ret |= KeyboardModifiers::MODALT;
    if (m & Mod4Mask)    ret |= KeyboardModifiers::MODSUPER;

    return ret;
}

inline void handleMouseClickEvent(XEvent& ev, bool isPress) {
    KeyboardModifiers mods = getModifiers(ev.xbutton.state);
    i32 x = i32(ev.xbutton.x);
    i32 y = i32(ev.xbutton.y);

    if (mouseScrollCallbackX11) {
        if (ev.xbutton.button == Button4) {
            mouseScrollCallbackX11(MouseScrollDirection::UP, x, y);
            return;
        }
        else if (ev.xbutton.button == Button5) {
            mouseScrollCallbackX11(MouseScrollDirection::DOWN, x, y);
            return;
        }
    }

    if (mouseClickCallbackX11) {
        if (ev.xbutton.button == Button1) {
            mouseClickCallbackX11(MouseButton::LEFT, isPress, x, y, mods);
            return;
        }
        else if (ev.xbutton.button == Button2) {
            mouseClickCallbackX11(MouseButton::MIDDLE, isPress, x, y, mods);
            return;
        }
        else if (ev.xbutton.button == Button3) {
            mouseClickCallbackX11(MouseButton::RIGHT, isPress, x, y, mods);
            return;
        }
        else {
            mouseClickCallbackX11(MouseButton::NONE, isPress, x, y, mods);
            logDebugTagged(PLATFORM_TAG, "Unknown Mouse Button");
            return;
        }
    }
}

inline void handleKeyEvent(XEvent& ev, bool isPress) {
    if (keyCallbackX11) {
        u32 vkcode = u32(XLookupKeysym(&ev.xkey, 0));
        u32 scancode = ev.xkey.keycode;
        KeyboardModifiers mods = getModifiers(ev.xkey.state);
        keyCallbackX11(vkcode, scancode, isPress, mods);
    }
}

} // namespace

Error Platform::pollEvents(bool block) {
    Assert(g_initialized, "Platform layer not initialized");

    if (!block && !XPending(g_display)) {
        return Error::OK;
    }

    XEvent xevent;
    XNextEvent(g_display, &xevent);

    switch (xevent.type) {
        case DestroyNotify: {
            XDestroyWindowEvent* e = reinterpret_cast<XDestroyWindowEvent*>(&xevent);
            if(e->window == g_window) {
                XSync(g_display, true);
                if (windowCloseCallbackX11) windowCloseCallbackX11();
                return Error::OK;
            }

            break;
        }
        case ClientMessage: {
            if (Atom(xevent.xclient.data.l[0]) == g_wmDeleteWindow) {
                XSync(g_display, true);
                if (windowCloseCallbackX11) windowCloseCallbackX11();
                return Error::OK;
            }
            break;
        }

        case ConfigureNotify:
            if (windowResizeCallbackX11) {
                i32 w = i32(xevent.xconfigure.width);
                i32 h = i32(xevent.xconfigure.height);
                windowResizeCallbackX11(w, h);
            }
            return Error::OK;

        case ButtonPress:
            handleMouseClickEvent(xevent, true);
            return Error::OK;

        case ButtonRelease:
            handleMouseClickEvent(xevent, false);
            return Error::OK;

        case KeyPress:
            handleKeyEvent(xevent, true);
            return Error::OK;

        case KeyRelease:
            handleKeyEvent(xevent, false);
            return Error::OK;

        case MotionNotify: {
            if (mouseMoveCallbackX11) {
                i32 x = i32(xevent.xmotion.x);
                i32 y = i32(xevent.xmotion.y);
                mouseMoveCallbackX11(x, y);
            }
            return Error::OK;
        }

        case EnterNotify: {
            if (mouseEnterOrLeaveCallbackX11) {
                // i32 x = xevent.xcrossing.x;
                // i32 y = xevent.xcrossing.y;
                mouseEnterOrLeaveCallbackX11(true);
            }
            return Error::OK;
        }

        case LeaveNotify: {
            if (mouseEnterOrLeaveCallbackX11) {
                // i32 x = xevent.xcrossing.x;
                // i32 y = xevent.xcrossing.y;
                mouseEnterOrLeaveCallbackX11(false);
            }
            return Error::OK;
        }

        case FocusIn:
            if (windowFocusCallbackX11) windowFocusCallbackX11(true);
            return Error::OK;

        case FocusOut:
            if (windowFocusCallbackX11) windowFocusCallbackX11(false);
            return Error::OK;

        default:
            break;
    }

    return Error::OK;
}

void Platform::registerWindowCloseCallback(WindowCloseCallback cb) { windowCloseCallbackX11 = cb; }
void Platform::registerWindowResizeCallback(WindowResizeCallback cb) { windowResizeCallbackX11 = cb; }
void Platform::registerWindowFocusCallback(WindowFocusCallback cb) { windowFocusCallbackX11 = cb; }

void Platform::registerKeyCallback(KeyCallback cb) { keyCallbackX11 = cb; }

void Platform::registerMouseClickCallback(MouseClickCallback cb) { mouseClickCallbackX11 = cb; }
void Platform::registerMouseMoveCallback(MouseMoveCallback cb) { mouseMoveCallbackX11 = cb; }
void Platform::registerMouseScrollCallback(MouseScrollCallback cb) { mouseScrollCallbackX11 = cb; }
void Platform::registerMouseEnterOrLeaveCallback(MouseEnterOrLeaveCallback cb) { mouseEnterOrLeaveCallbackX11 = cb; }

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char**) {
    // TODO: unconnect once Vulkan SDK is linked.
    // extensions[0] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
}

Error Platform::createVulkanSurface(VkInstance, VkSurfaceKHR&) {
    Assert(g_initialized, "Platform Layer needs to be initialized");

    // TODO: unconnect once Vulkan SDK is linked.

    // VkXlibSurfaceCreateInfoKHR createInfo{};
    // createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    // createInfo.dpy = g_display;
    // createInfo.window = g_window;

    // VkResult vres = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &outSurface);
    // if (vres != VK_SUCCESS) {
    //     // This could technically be a render error as well.
    //     return createPltErr(FAILED_TO_CREATE_X11_KHR_XLIB_SURFACE, "Failed to create Xlib Vulkan surface");
    // }

    return Error::OK;
}

bool Platform::getFrameBufferSize(u32& width, u32& height) {
    Assert(g_initialized, "Platform Layer needs to be initialized");

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(g_display, g_window, &attrs)) {
        // Should not happen ever!
        width = 0;
        height = 0;
        logErrTagged(
            PLATFORM_TAG,
             "Call to XGetWindowAttributes failed"
               "Typically this is due to issues like an invalid Display pointer or Window"
        );
        return false;
    }

    // Calculate the frame buffer size (physical pixel dimensions)
    width = u32(attrs.width);
    height = u32(attrs.height);
    return true;
}

namespace {

i32 handleXError(Display* display, XErrorEvent* errorEvent) {
    constexpr i32 ERROR_TEXT_MAX_SIZE = 512;
    char errorText[ERROR_TEXT_MAX_SIZE] = {};
    XGetErrorText(display, errorEvent->error_code, errorText, sizeof(errorText));

    // NOTE: Returning 0 continues execution; returning non-zero terminates execution.

    if (errorEvent->error_code == Success)
        return 0;

    logErrTagged(
        LogTag::PLATFORM_TAG,
        "Xlib Error: \n\tRequest Code: {}\n\tMinor Code: {}\n\tResource ID: {}, Error Text: {}",
        i32(errorEvent->request_code), i32(errorEvent->minor_code), errorEvent->resourceid, errorText
    );

    switch (errorEvent->error_code) {
        // Benign (often safe to ignore in shutdown, or from event race)
        case BadWindow:    break; // e.g. destroying already-closed window
        case BadDrawable:  break; // can happen if draw is issued to unmapped or stale window
        case BadGC:        break; // same as above, usually in cleanup

        // Recoverable (usually means feature/resource is missing or optional)
        case BadAtom:  break; // invalid WM atom; may just affect window close handling
        case BadColor: break; // invalid color spec; fallback possible
        case BadFont:  break; // font name not found; use default
        case BadName:  break; // named resource not found (e.g. font or color)

        // Severe or unrecoverable (likely logic or config errors)
        case BadRequest:        return BadRequest;        // unknown request; programming error
        case BadPixmap:         return BadPixmap;         // invalid pixmap ID
        case BadValue:          return BadValue;          // parameter out of range
        case BadCursor:         return BadCursor;         // invalid cursor
        case BadMatch:          return BadMatch;          // visual mismatch, or incompatible params
        case BadAccess:         return BadAccess;         // resource already grabbed, permission issue
        case BadAlloc:          return BadAlloc;          // X server out of memory (very rare, but fatal)
        case BadIDChoice:       return BadIDChoice;       // duplicate ID used; internal ID mismanagement
        case BadLength:         return BadLength;         // request length invalid; likely struct mismatch
        case BadImplementation: return BadImplementation; // X server doesn't support a requested feature

        default:
            // What should the reaction be in this case?
            logErrTagged(
                LogTag::PLATFORM_TAG,
                "Unhandled X11 error code: {}, continuing execution.", int(errorEvent->error_code)
            );
            break;
    }

    return 0;
}

} // namespace

} // memviz
