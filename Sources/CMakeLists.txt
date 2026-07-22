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
    tone_generator.c
    receiver.c
    transmitter.c
    dma_common.c
    vspa_state.c
    vspa_features.c
    timer_control.c
)

# CMake does not recognize .sx as assembly, only .s or .asm
set_source_files_properties(
    crt0.s
    PROPERTIES LANGUAGE C
)

set(LA9310_LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/LCF/la9310_iqplayer.lcf)
set(COMMON_LINK_OPTIONS -mem ${LA9310_LINKER_SCRIPT} -nostartup-files)

add_executable(limesdr-micro-iqplayer
    ${COMMON_IQPLAYER_SOURCE}
)
target_compile_definitions(limesdr-micro-iqplayer PRIVATE IQMOD_RX_1T1R)
target_link_options(limesdr-micro-iqplayer PUBLIC ${COMMON_LINK_OPTIONS}
    LINKER:-Map,${EXECUTABLE_OUTPUT_PATH}/limesdr-micro-iqplayer.map
)
target_link_libraries(limesdr-micro-iqplayer PUBLIC vspa-kernel vspa-sdk)
set_target_properties(limesdr-micro-iqplayer PROPERTIES LINK_DEPENDS ${LA9310_LINKER_SCRIPT})
