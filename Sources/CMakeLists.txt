CMAKE_MINIMUM_REQUIRED (VERSION 3.18)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

# Treat warnings as errors
# set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
include_directories(${CMAKE_CURRENT_LIST_DIR})

set(COMMON_IQPLAYER_SOURCE
    _main.c
    crt0.s
    dfe.c
    l1-trace.c
    signal.c
    iqmod_rx.c
    iqmod_tx.c
)

# CMake does not recognize .sx as assembly, only .s or .asm
set_source_files_properties(
    crt0.s
    PROPERTIES LANGUAGE C
)

set(COMMON_LINK_OPTIONS -mem ${PROJECT_SOURCE_DIR}/LCF/la9310_iqplayer.lcf -nostartup-files)

add_executable(apm-iqplayer-1T1R
    ${COMMON_IQPLAYER_SOURCE}
)
target_compile_definitions(apm-iqplayer-1T1R PRIVATE IQMOD_RX_1T1R)
target_link_options(apm-iqplayer-1T1R PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-1T1R PUBLIC vspa-kernel)

add_executable(apm-iqplayer-2DEC2INT
    ${COMMON_IQPLAYER_SOURCE}
)
target_compile_definitions(apm-iqplayer-2DEC2INT PRIVATE IQMOD_2DEC2INT IQMOD_RX_2DEC2INT)
target_link_options(apm-iqplayer-2DEC2INT PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-2DEC2INT PUBLIC vspa-kernel)

add_executable(apm-iqplayer-4DEC4INT
    ${COMMON_IQPLAYER_SOURCE}
)
target_compile_definitions(apm-iqplayer-4DEC4INT PRIVATE IQMOD_4DEC4INT IQMOD_RX_4DEC4INT)
target_link_options(apm-iqplayer-4DEC4INT PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-4DEC4INT PUBLIC vspa-kernel)
