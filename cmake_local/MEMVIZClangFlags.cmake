include(CompilerOptions)

macro(memviz_target_set_default_flags
    target
    is_debug
    save_temporary_files)

    set(common_flags -std=c++20 -pthread)
    set(debug_flags "")
    set(release_flags "")

    if(${save_temporary_files})
        set(common_flags "${common_flags}" "-g" "-save-temps")
    endif()

    generate_common_flags(
        common_flags "${common_flags}"
        debug_flags "${debug_flags}"
        release_flags "${release_flags}"
    )

    # This apperantly needs to be set after all other flags. Probably because of some ordering problem.
    set(common_flags ${common_flags}
        -Wno-gnu-zero-variadic-macro-arguments # Supress warning for " , ##__VA_ARGS__ " in variadic macros
    )

    if(${is_debug})
        target_compile_options(${target} PRIVATE ${common_flags} ${debug_flags})
    else()
        target_compile_options(${target} PRIVATE ${common_flags} ${release_flags})
    endif()

endmacro()

macro(memviz_target_enable_sanitizers
      target
      enable_asan
      enable_ubsan
      enable_tsan)

    if(${enable_tsan} AND ${enable_asan})
        message(FATAL_ERROR "ASAN and TSAN cannot be enabled at the same time.")
    endif()

    set(sanitize_flags "")

    if(${enable_asan})

        if(APPLE)
            list(APPEND sanitize_flags -fsanitize=address)
            message(STATUS "LeakSanitizer is not supported on macOS — skipping")
        else()
            # FIXME: For some reason sanitizers are broken on my machine.

            # Enable leak detection only on non-Apple platforms
            # set(ENV{LSAN_OPTIONS} "verbosity=1:log_threads=1")
            # list(APPEND sanitize_flags -fsanitize=address)
            # list(APPEND sanitize_flags -fsanitize=leak)
        endif()
    endif()

    if(${enable_ubsan})
        list(APPEND sanitize_flags -fsanitize=undefined)
    endif()

    if(${enable_tsan})
        list(APPEND sanitize_flags -fsanitize=thread)
    endif()

    if(sanitize_flags)
        target_compile_options(${target} PRIVATE ${sanitize_flags})
        target_link_options(${target} PRIVATE ${sanitize_flags})
    endif()
endmacro()
