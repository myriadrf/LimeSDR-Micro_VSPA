add_library(vspa-kernel
	src/ddc2x4x.sx
	src/fftDIF512_hfx_hfx.sx
	src/txiqcomp.sx
	src/txiqcomp_x32chf_5t.sx
	src/X2_interp_tap32_filter.sx
	src/X4_interp_tap64_filter.sx
)

# CMake does not recognize .sx as assembly, only .s
set_source_files_properties(
	src/ddc2x4x.sx
	src/fftDIF512_hfx_hfx.sx
	src/txiqcomp.sx
	src/txiqcomp_x32chf_5t.sx
	src/X2_interp_tap32_filter.sx
	src/X4_interp_tap64_filter.sx
	PROPERTIES LANGUAGE C
)

target_include_directories(vspa-kernel PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
target_link_libraries(vspa-kernel PUBLIC vspa-sdk)
