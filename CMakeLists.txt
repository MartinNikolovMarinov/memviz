cmake_minimum_required(VERSION 3.2)

project(STL_VIEWER VERSION 0.0.0 LANGUAGES CXX)

set(target_main memviz)

# Standard Requirements:

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generates compile_commands.json

# Add directories from lib cmake projects:

set(CORE_ASSERT_ENABLED ON CACHE BOOL "Enable Asserts in corelib" FORCE)
set(CORE_LIBRARY_SHARED OFF CACHE BOOL "Enable Asserts in corelib" FORCE)
add_subdirectory(lib/core)

# Include cmake modules from:

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_local")

include(DetectOS)
include(CompilerOptions)
include(Logger)
include(MEMVIZDefaultFlags)

init_logger("[MEMVIZ]")

# ---------------------------------------- Begin Options ---------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Release")
    set(MEMVIZ_DEBUG OFF)
else()
    set(MEMVIZ_DEBUG ON)
endif()

option(MEMVIZ_USE_ANSI_LOGGING "Use ANSI in logs" OFF)
option(MEMVIZ_ENABLE_ASAN "Enable ASAN" OFF)
option(MEMVIZ_ENABLE_UBSAN "Enable UBSAN" OFF)
option(MEMVIZ_ENABLE_TSAN "Enable TSAN" OFF)

# Print Selected Options:

log_info("---------------------------------------------")
log_info("Version:                   ${PROJECT_VERSION}")
log_info("Platform:                  ${OS}")
log_info("Arch:                      ${CMAKE_SYSTEM_PROCESSOR}")
log_info("C++ Version:               ${CMAKE_CXX_STANDARD}")
log_info("Compiler:                  ${CMAKE_CXX_COMPILER_ID}")
log_info("Debug:                     ${MEMVIZ_DEBUG}")
log_info("ANSI logging:              ${MEMVIZ_USE_ANSI_LOGGING}")
log_info("ASAN Enabled:              ${MEMVIZ_ENABLE_ASAN}")
log_info("UBSAN Enabled:             ${MEMVIZ_ENABLE_UBSAN}")
log_info("TSAN Enabled:              ${MEMVIZ_ENABLE_TSAN}")
log_info("---------------------------------------------")

# ---------------------------------------- End Options -----------------------------------------------------------------

# ---------------------------------------- Begin Declare Source Files --------------------------------------------------

set(memviz_src
    src/basic.cpp

    src/systems/logger.cpp
)

if(OS STREQUAL "linux")
    set(memviz_src ${memviz_src}
        main_linux.cpp

        src/x11_platform.cpp # temporary here
    )
elseif(OS STREQUAL "darwin")
    log_fatal("OS not supported yet!")
elseif(OS STREQUAL "windows")
    log_fatal("OS not supported yet!")
endif()


# ---------------------------------------- End Declare Source Files ----------------------------------------------------

# ---------------------------------------- Begin Create Executable -----------------------------------------------------

add_executable(${target_main} ${memviz_src})
target_link_libraries(${target_main} PUBLIC
    core # link with corelib
)
target_include_directories(${target_main} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(${target_main} PUBLIC
    "MEMVIZ_DEBUG=$<BOOL:${MEMVIZ_DEBUG}>"
    "MEMVIZ_USE_ANSI_LOGGING=$<BOOL:${MEMVIZ_USE_ANSI_LOGGING}>"
    MEMVIZ_ASSETS="${CMAKE_BINARY_DIR}/assets"
)

if(OS STREQUAL "linux")
    find_package(X11 REQUIRED)
    find_package(Vulkan REQUIRED)  # Link with Vulkan from system
    target_link_libraries(${target_main} PRIVATE ${Vulkan_LIBRARIES} ${X11_LIBRARIES})
    target_compile_definitions(${target_main} PRIVATE -DUSE_X11)

    # TODO: What if the system is Wayland? How can that be detected?
endif()

memviz_target_set_default_flags(${target_main} ${MEMVIZ_DEBUG} false)
memviz_target_enable_sanitizers(${target_main} ${MEMVIZ_ENABLE_ASAN} ${MEMVIZ_ENABLE_UBSAN} ${MEMVIZ_ENABLE_TSAN})

# ---------------------------------------- End Create Executable -------------------------------------------------------

# ---------------------------------------- Begin Custom Targets --------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Usage: cmake --build . --target run_valgrind
    add_custom_target(run_valgrind
        COMMAND valgrind --leak-check=full --suppressions=${CMAKE_SOURCE_DIR}/valgrind.supp $<TARGET_FILE:${target_main}>
        DEPENDS ${target_main}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running ${target_main} with Valgrind (GCC only)"
    )
endif()

# ---------------------------------------- End Custom Targets ----------------------------------------------------------
