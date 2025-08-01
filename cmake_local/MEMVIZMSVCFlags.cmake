include(CompilerOptions)

macro(memviz_target_set_default_flags
    target
    is_debug
    save_temporary_files)

    set(common_flags "/std:c++20")
    set(debug_flags "")
    set(release_flags "")

    if(${save_temporary_files})
        set(common_flags "${common_flags}" "/Fa${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    generate_common_flags(
        common_flags "${common_flags}"
        debug_flags "${debug_flags}"
        release_flags "${release_flags}"
    )

    if(${is_debug})
        target_compile_options(${target} PRIVATE ${common_flags} ${debug_flags})
    else()
        target_compile_options(${target} PRIVATE ${common_flags} ${release_flags})
    endif()

    if (${is_debug} AND OS STREQUAL "windows")
        # DbgHelp.lib is required for stacktrace on Windows.
        target_link_options(${target} PRIVATE /INCREMENTAL:NO /OPT:REF)
        target_link_libraries(${target} PUBLIC DbgHelp.lib)
    endif()

endmacro()

