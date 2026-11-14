# Loaded via CMAKE_USER_MAKE_RULES_OVERRIDE_C from vspacc_toolchain.cmake.
# VSPA assembly sources (.s, .sx) are preprocessed and assembled by fsvspacc,
# so register them as C sources. This must happen here: setting the list in the
# toolchain file is overwritten by CMakeCCompiler.cmake, and after project() is too late.
list(APPEND CMAKE_C_SOURCE_FILE_EXTENSIONS s sx)
