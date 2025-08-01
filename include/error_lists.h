#pragma once

namespace memviz {

#define MEMVIZ_X11_ERROR_LIST \
    MEMVIZ_PLT_ERROR_ITEM(FAILED_TO_CREATE_X11_DISPLAY, "Failed to open X display") \
    MEMVIZ_PLT_ERROR_ITEM(FAILED_TO_CREATE_X11_WINDOW, "Failed to create X11 window") \
    MEMVIZ_PLT_ERROR_ITEM(FAILED_TO_INITIALIZE_X11_THREADS, "Failed to initialize x11 threads")

} // memviz
