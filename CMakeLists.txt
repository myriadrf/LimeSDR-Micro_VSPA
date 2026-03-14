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
    -O3
    -Os
    -nostartup-files
    -ansi off
    -opt=noalias_by_type
    -mvcpu
    -msgstyle gcc
    $<$<CONFIG:Debug>:-g> # generate debug info
)

add_link_options(
    $<$<CONFIG:Release>:LINKER:-S> # Strips debug information from the executable object file.
    $<$<CONFIG:Release>:LINKER:-xd> # Instructs the linker to remove symbols that were emitted by the compiler and assembler for debugging sections.
    $<$<CONFIG:Release>:LINKER:-x> # Removes all local symbols from the symbol table.
)

message(STATUS "BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

include_directories(include)
add_subdirectory(vspa-sdk)
add_subdirectory(vspa-lib)
add_subdirectory(Sources)
