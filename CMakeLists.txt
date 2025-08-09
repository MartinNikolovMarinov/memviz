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
option(MEMVIZ_USE_VULKAN "Enable Vulkan" OFF)

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
if(MEMVIZ_USE_VULKAN)
log_info("Renderer:                  Vulkan")
endif()

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

        src/x11_platform.cpp # TODO: check this when Wayland support is added.
    )
elseif(OS STREQUAL "darwin")
    log_fatal("OS not supported yet!")
elseif(OS STREQUAL "windows")
    log_fatal("OS not supported yet!")
endif()

if(MEMVIZ_USE_VULKAN)
    set(memviz_src ${memviz_src}
        src/systems/renderer/vulkan_backend.cpp
    )
else()
    # TODO: Temporarily supporting only vulkan as a renderer.
    log_fatal("No supported renderer backend found")
endif()

# ---------------------------------------- End Declare Source Files ----------------------------------------------------

# ---------------------------------------- Begin Create Executable -----------------------------------------------------

add_executable(${target_main} ${memviz_src})
target_link_libraries(${target_main} PRIVATE
    core # link with corelib
)
target_include_directories(${target_main} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(${target_main} PRIVATE
    "MEMVIZ_DEBUG=$<BOOL:${MEMVIZ_DEBUG}>"
    "MEMVIZ_USE_ANSI_LOGGING=$<BOOL:${MEMVIZ_USE_ANSI_LOGGING}>"
    MEMVIZ_ASSETS="${CMAKE_BINARY_DIR}/assets"
)

if(MEMVIZ_USE_VULKAN)
    target_compile_definitions(${target_main} PRIVATE -DMEMVIZ_USE_VULKAN)
endif()

if(OS STREQUAL "linux")
    # TODO: What if the system is Wayland? How can that be detected?

    # X11 dependencies
    find_package(X11 REQUIRED)
    target_link_libraries(${target_main} PRIVATE ${X11_LIBRARIES})
    target_compile_definitions(${target_main} PRIVATE -DUSE_X11)

    if(NOT DEFINED ENV{VULKAN_SDK})
        log_fatal("VULKAN_SDK environment variable is required but not set.")
    endif()

    log_info("Setting up Vulkan config:")

    set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")
    set(VULKAN_BIN_PATH "${VULKAN_SDK_ROOT}/bin")
    set(VULKAN_LIB_PATH "${VULKAN_SDK_ROOT}/lib")

    log_info("VULKAN_SDK_ROOT: ${VULKAN_SDK_ROOT}")
    log_info("VULKAN_BIN_PATH: ${VULKAN_BIN_PATH}")
    log_info("VULKAN_LIB_PATH: ${VULKAN_LIB_PATH}")
    log_info("")
    log_info("VK_LAYER_PATH: $ENV{VK_LAYER_PATH}")
    log_info("VK_ADD_LAYER_PATH: $ENV{VK_ADD_LAYER_PATH}")
    log_info("VK_KHRONOS_PROFILES_PROFILE_NAME: $ENV{VK_KHRONOS_PROFILES_PROFILE_NAME}")
    log_info("VK_KHRONOS_PROFILES_VALIDATE: $ENV{VK_KHRONOS_PROFILES_VALIDATE}")
    log_info("VK_KHRONOS_PROFILES_EMULATE_PORTABILITY: $ENV{VK_KHRONOS_PROFILES_EMULATE_PORTABILITY}")

    # Check LD_LIBRARY_PATH contains $VULKAN_SDK/lib
    if(NOT "$ENV{LD_LIBRARY_PATH}" MATCHES "${VULKAN_LIB_PATH}")
        log_fatal("LD_LIBRARY_PATH does not contain ${VULKAN_LIB_PATH} — Vulkan runtime will fail to load.")
    endif()

    # Check PATH contains $VULKAN_SDK/bin
    if(NOT "$ENV{PATH}" MATCHES "${VULKAN_BIN_PATH}")
        log_fatal("PATH does not contain ${VULKAN_BIN_PATH} — Vulkan tools like 'glslc' wont be found.")
    endif()

    # Optional VK_LAYER_PATH
    if(NOT DEFINED ENV{VK_LAYER_PATH} AND NOT MEMVIZ_DEBUG)
        log_warning("VK_LAYER_PATH is not set in debug. Vulkan validation layers may not work.")
    endif()

    find_package(Vulkan REQUIRED)
    target_link_libraries(${target_main} PRIVATE ${Vulkan_LIBRARIES})
    target_include_directories(${target_main} PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_compile_definitions(${target_main} PRIVATE -DVK_ENABLE_BETA_EXTENSIONS)
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
