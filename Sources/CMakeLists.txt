CMAKE_MINIMUM_REQUIRED (VERSION 3.18)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

# Treat warnings as errors
# set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
include_directories(${CMAKE_CURRENT_LIST_DIR})

set(COMMON_IQPLAYER_SOURCE
    _main.c
    crt0.s
    dc_cal.c
    dfe.c
    l1-trace.c
    ovly_init.c
    signal.c
)

# CMake does not recognize .sx as assembly, only .s or .asm
set_source_files_properties(
    crt0.s
    PROPERTIES LANGUAGE C
)

set(COMMON_LINK_OPTIONS -mem ${PROJECT_SOURCE_DIR}/LCF/la9310_iqplayer.lcf -nostartup-files -O3 -Os)


add_executable(apm-iqplayer-0T1R
    ${COMMON_IQPLAYER_SOURCE}
    iqmod_rx.c
)
target_compile_definitions(apm-iqplayer-0T1R PRIVATE IQMOD_RX_0T1R)
target_link_options(apm-iqplayer-0T1R PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-0T1R PUBLIC vspa-kernel)


add_executable(apm-iqplayer-1T0R
    ${COMMON_IQPLAYER_SOURCE}
    iqmod_rx.c
)
target_compile_definitions(apm-iqplayer-1T0R PRIVATE IQMOD_RX_0T1R)
target_link_options(apm-iqplayer-1T0R PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-1T0R PUBLIC vspa-kernel)


add_executable(apm-iqplayer-1T1R
    ${COMMON_IQPLAYER_SOURCE}
    iqmod_rx.c
    iqmod_tx.c
)
target_compile_definitions(apm-iqplayer-1T1R PRIVATE IQMOD_RX_1T1R)
target_link_options(apm-iqplayer-1T1R PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-1T1R PUBLIC vspa-kernel)


add_executable(apm-iqplayer-1T2R
    ${COMMON_IQPLAYER_SOURCE}
    iqmod_rx_2R_decx2x4.c
    iqmod_tx_2R.c
)
target_compile_definitions(apm-iqplayer-1T2R PRIVATE IQMOD_RX_1T2R)
target_link_options(apm-iqplayer-1T2R PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-1T2R PUBLIC vspa-kernel)

add_executable(apm-iqplayer-1T4R
    ${COMMON_IQPLAYER_SOURCE}
    iqmod_rx_2R_decx2x4.c
    iqmod_tx_2R.c
)
target_compile_definitions(apm-iqplayer-1T4R PRIVATE IQMOD_RX_1T4R)
target_link_options(apm-iqplayer-1T4R PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-1T4R PUBLIC vspa-kernel)

# Need to check required definitions
add_executable(apm-iqplayer-4DDC
    ${COMMON_IQPLAYER_SOURCE}
    iqmod_rx_2R_decx2x4.c
    iqmod_tx_2R.c
)
target_compile_definitions(apm-iqplayer-4DDC PRIVATE IQMOD_RX_4DDC)
target_link_options(apm-iqplayer-4DDC PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-4DDC PUBLIC vspa-kernel)

add_executable(apm-iqplayer-8DDC
    ${COMMON_IQPLAYER_SOURCE}
    iqmod_rx_2R_decx2x4.c
    iqmod_tx.c
)
target_compile_definitions(apm-iqplayer-8DDC PRIVATE IQMOD_RX_8DDC)
target_link_options(apm-iqplayer-8DDC PUBLIC ${COMMON_LINK_OPTIONS})
target_link_libraries(apm-iqplayer-8DDC PUBLIC vspa-kernel)

# target_link_libraries(apm-iqplayer-1T4R PUBLIC
#     # libc99_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     # libc_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     # libm_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     # libnr_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     # librt_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     startup_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.eln
# )
