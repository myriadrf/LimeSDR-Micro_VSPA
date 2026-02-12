add_library(vspa-sdk src/vcpu.sx)

# CMake does not recognize .sx as assembly, only .s or .asm
set_source_files_properties(
	src/vcpu.sx
	PROPERTIES LANGUAGE C
)

target_include_directories(vspa-sdk PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)

# compile definition to include appropriate headers
set(VSPA_SDK_CHIP "LA9310" CACHE STRING "VSPA Chip type")
set(VSPA_SDK_CHIP_OPTIONS LA1575 LA1224 LA9358 LA9310)
set_property(CACHE VSPA_SDK_CHIP PROPERTY STRINGS ${VSPA_SDK_CHIP_OPTIONS})
target_compile_definitions(vspa-sdk PUBLIC "__${VSPA_SDK_CHIP}__")

# target_link_libraries(vspa-sdk PUBLIC
#     # libc99_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     libc_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     libm_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     libnr_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     librt_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.elb
#     startup_au${VSPA_AU_COUNT}_${VSPA_CORE_TYPE}.eln
# )
