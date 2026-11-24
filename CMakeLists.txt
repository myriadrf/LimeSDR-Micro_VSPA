# if VSPA toolchain is not supplied, automatically download it
if (NOT CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_CURRENT_LIST_DIR}/vspacc_toolchain.cmake)
endif()

cmake_minimum_required (VERSION 3.20)

project("LimeSDR-Micro_VSPA"
    VERSION 1.0.0
    LANGUAGES C)

# Default to Release build
IF(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
ENDIF()

add_compile_options(
    -O2
    -Os
    -nostartup-files
    -ansi off
    -opt=noalias_by_type
    -mvcpu
    -msgstyle gcc
    -enum min
    -gccinc # adopt GCC #include semantics:  add '-I' paths to system list and search directory of referencing file first for #includes (same as '-cwd include')

    $<$<CONFIG:Debug>:-g> # generate debug info
)

add_link_options(
    $<$<CONFIG:Release>:LINKER:-S> # Strips debug information from the executable object file.
    $<$<CONFIG:Release>:LINKER:-s> # Strips all symbols (.symtab, .strtab, .mw_info); the loader only needs section names and contents, symbols remain in the .map.
    $<$<CONFIG:Release>:LINKER:-disable-emit-note> # Omits the .note section (linker version strings).
    LINKER:-enable-stack-effect
    LINKER:-display-section-total
    LINKER:-enable-display-padding-info
)
message(STATUS "BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

add_subdirectory(vspa-sdk)
add_subdirectory(vspa-lib)
add_subdirectory(Sources)
