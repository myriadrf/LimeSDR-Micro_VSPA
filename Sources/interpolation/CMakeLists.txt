add_library(interpolator
    interpolation.c
)

target_link_libraries(interpolator PUBLIC vspa-kernel vspa-sdk)
