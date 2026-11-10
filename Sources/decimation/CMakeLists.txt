add_library(decimator
    decimation.c
)

target_link_libraries(decimator PUBLIC vspa-kernel vspa-sdk)

