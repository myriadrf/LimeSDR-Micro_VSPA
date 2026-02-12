CMAKE_MINIMUM_REQUIRED (VERSION 3.22)

if (VSPA_TOOLS_CONFIGURED)
    return()
endif()

set(TOOLCHAINS_STORAGE_DIR "${CMAKE_SOURCE_DIR}/artifacts")

# Use system defined tools if available, otherwise download locally
if(DEFINED ENV{"VSPA_TOOL"})
    set(VSPA_TOOLCHAIN_DIR ENV{"VSPA_TOOL"})
else()
    set(VSPA_DOWNLOAD_URL "https://www.nxp.com/lgfiles/sdk/la1224/imx-la9310-sdk-10/cwvspa.vbeta_14_00_781_vspa.linux.tgz")
    set(VSPA_PACKAGE "cwvspa.vbeta_14_00_781_vspa.linux.tar.xz")
    if (NOT EXISTS ${TOOLCHAINS_STORAGE_DIR}/${VSPA_PACKAGE})
        message(STATUS "Downloading VSPA toolchain ${VSPA_DOWNLOAD_URL} to: ${TOOLCHAINS_STORAGE_DIR}")
        file(DOWNLOAD ${VSPA_DOWNLOAD_URL} ${TOOLCHAINS_STORAGE_DIR}/${VSPA_PACKAGE}
            # EXPECTED_HASH SHA256=
            STATUS VSPA_IS_DOWNLOADED
            SHOW_PROGRESS)
    endif()
    if(NOT EXISTS ${TOOLCHAINS_STORAGE_DIR}/VSPA_Tools) # prevent repeated extractions
        if (NOT EXISTS ${TOOLCHAINS_STORAGE_DIR}/${VSPA_PACKAGE})
            message(FATAL_ERROR "Missing VSPA Tools package: ${TOOLCHAINS_STORAGE_DIR}/${VSPA_PACKAGE}")
        endif()
        file(ARCHIVE_EXTRACT INPUT ${TOOLCHAINS_STORAGE_DIR}/${VSPA_PACKAGE}
            DESTINATION ${TOOLCHAINS_STORAGE_DIR})
    endif()
    set(VSPA_TOOLCHAIN_DIR ${TOOLCHAINS_STORAGE_DIR}/VSPA_Tools)
endif()
STRING(REGEX REPLACE "\\\\" "/" VSPA_TOOLCHAIN_DIR "${VSPA_TOOLCHAIN_DIR}")

if(NOT VSPA_TOOLCHAIN_DIR)
    message(FATAL_ERROR "***Please set VSPA_TOOLS directory in envionment variables***")
endif()
message(STATUS "VSPA_TOOL: " ${VSPA_TOOLCHAIN_DIR})

set(VSPA_ARCH "vspa2" CACHE STRING "VSPA architecture")
set(AVAILABLE_ARCHITECTURES vspa2 vspa3)
set_property(CACHE VSPA_ARCH PROPERTY STRINGS ${AVAILABLE_ARCHITECTURES})
message(STATUS "VSPA_ARCH: " ${VSPA_ARCH})

set(VSPA_AU_COUNT 16 CACHE STRING "VSPA arithmetic units count")
set(VSPA_AU_COUNT_OPTIONS 2 4 8 16 32 64)
set_property(CACHE VSPA_AU_COUNT PROPERTY STRINGS ${VSPA_AU_COUNT_OPTIONS})
message(STATUS "VSPA_AU_COUNT: " ${VSPA_AU_COUNT})

set(VSPA_CORE_TYPE "sp")

# compiler description for CMake
set(VSPA_VERSION ${VSPA_ARCH})
set(CMAKE_SYSTEM_NAME Generic) # e.g. bare metal embedded devices
set(CMAKE_SYSTEM_PROCESSOR vspa)

# TOOLCHAIN EXTENSION
if(WIN32)
    set(TOOLCHAIN_EXT ".exe")
else()
    set(TOOLCHAIN_EXT "")
endif()

set(VSPA_LICENSE_FILE "" CACHE FILEPATH "VSPA License file")
if (VSPA_LICENSE_FILE)
    set(VSPA_LICENSE_ARG "-use-license-file ${VSPA_LICENSE_FILE}")
    message(STATUS "VSPA_LICENSE_FILE: " ${VSPA_LICENSE_FILE})
else()
    set(VSPA_LICENSE_ARG "")
endif()

set(TOOLCHAIN_BIN_DIR ${VSPA_TOOLCHAIN_DIR}/bin)
set(TOOLCHAIN_INC_DIR ${VSPA_TOOLCHAIN_DIR}/include)
set(TOOLCHAIN_LIB_DIR ${VSPA_TOOLCHAIN_DIR}/lib/${VSPA_ARCH})

set(CMAKE_AR ${TOOLCHAIN_BIN_DIR}/vspa-ar${TOOLCHAIN_EXT})
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> -cr <TARGET> <LINK_FLAGS> <OBJECTS>") # replaces options used for archiver

# C compiler
set(CMAKE_C_COMPILER_ARCHITECTURE_ID ${AVAILABLE_ARCHITECTURES})
set(CMAKE_C_COMPILER ${TOOLCHAIN_BIN_DIR}/fsvspacc${TOOLCHAIN_EXT})
set(CMAKE_C_COMPILER_WORKS ON) # TODO verify by compiling simple code from VSPA_TOOL/etc/*/Sources
set(CMAKE_C_FLAGS "${VSPA_LICENSE_ARG} -arch ${VSPA_ARCH} -au_count ${VSPA_AU_COUNT} -core_type ${VSPA_CORE_TYPE} -env ${VSPA_TOOLCHAIN_DIR} -L ${TOOLCHAIN_LIB_DIR} ${CMAKE_C_FLAGS}")

# Don't really need to override the COMPILE_OBJECT arguments, BUT
# if -o argument specifies the output filename, intermediate text files are generated in current working directory
# therefore if multiple targets have the same source file and are built concurently, intermediate files will collide(race condition)
# if -o argument specifies directory, then intermediate files are places inside it, but the generated output file extension is .eln, not compatible with CMake expectations
# specifying -o twice, as directory and as output filename seems to work correctly
set(CMAKE_C_COMPILE_OBJECT  "<CMAKE_C_COMPILER> <FLAGS> <DEFINES> <INCLUDES> -o <OBJECT_DIR> -o <OBJECT> -c <SOURCE>")

# Assembler, use C compiler for assembly files
# set(CMAKE_ASM_FLAGS "${VSPA_LICENSE_ARG} ${CMAKE_ASM_FLAGS}")
# set(CMAKE_ASM_COMPILER ${TOOLCHAIN_BIN_DIR}/${VSPA_ARCH}-as${TOOLCHAIN_EXT})
set(CMAKE_ASM_COMPILER_ARCHITECTURE_ID ${AVAILABLE_ARCHITECTURES})
set(CMAKE_ASM_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_ASM_COMPILER_WORKS ON)

set(CMAKE_CXX_COMPILER_WORKS OFF)

set(CMAKE_EXECUTABLE_SUFFIX_C ".eld") # produce firmware with .eld extension
set(CMAKE_FIND_LIBRARY_SUFFIXES ".eln")
set(CMAKE_STATIC_LIBRARY_SUFFIX_C ".elb") # fsvspacc fails to link .a files without -l flag, but can link .elb
set(CMAKE_STATIC_LIBRARY_SUFFIX_ASM ".elb")

# paths in this list as alternative roots to find filesystem items with find_package(), find_library() etc.
set(CMAKE_FIND_ROOT_PATH ${VSPA_TOOLCHAIN_DIR})

# If set to ONLY, then only the roots in CMAKE_FIND_ROOT_PATH will be searched.
# If set to NEVER, then the roots in CMAKE_FIND_ROOT_PATH will be ignored and only the host system root will be used.
# If set to BOTH, then the host system paths and the paths in CMAKE_FIND_ROOT_PATH will be searched.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(VSPA_TOOLS_CONFIGURED TRUE)
