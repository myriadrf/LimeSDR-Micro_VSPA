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

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Os -nostartup-files -ansi off -g -opt=noalias_by_type -mvcpu -msgstyle gcc")

message(STATUS "BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

include_directories(include)
add_subdirectory(vspa-sdk)
add_subdirectory(vspa-lib)
add_subdirectory(Sources)
