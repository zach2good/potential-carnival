# Try to find BearLibTerminal (internally, in root/ext/)
# BearLibTerminal_FOUND - System has BearLibTerminal
# BearLibTerminal_LIBRARY - The libraries needed to use BearLibTerminal
# BearLibTerminal_INCLUDE_DIR - The BearLibTerminal include directories

# TODO: Add more platforms...
if(MSVC)
    set(PLATFORM_STRING "Windows")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "CMAKE_SIZEOF_VOID_P == 8: 64-bit build")
    set(PLATFORM_SUFFIX "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "CMAKE_SIZEOF_VOID_P == 4: 32-bit build")
    set(PLATFORM_SUFFIX "32")
endif()

find_library(BearLibTerminal_LIBRARY 
    NAMES 
        "BearLibTerminal"
    PATHS
        ${PROJECT_SOURCE_DIR}/ext/BearLibTerminal/${PLATFORM_STRING}${PLATFORM_SUFFIX}/)

find_file(BearLibTerminal_RUNTIME_LIBRARY 
    NAMES 
        "BearLibTerminal.dll"
    PATHS
        ${PROJECT_SOURCE_DIR}/ext/BearLibTerminal/${PLATFORM_STRING}${PLATFORM_SUFFIX}/)

find_path(BearLibTerminal_INCLUDE_DIR 
    NAMES 
        BearLibTerminal.h
    PATHS
        ${PROJECT_SOURCE_DIR}/ext/BearLibTerminal/Include/C/)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BearLibTerminal DEFAULT_MSG BearLibTerminal_LIBRARY BearLibTerminal_INCLUDE_DIR)

message(STATUS "BearLibTerminal_FOUND: ${BearLibTerminal_FOUND}")
message(STATUS "BearLibTerminal_LIBRARY: ${BearLibTerminal_LIBRARY}")
message(STATUS "BearLibTerminal_RUNTIME_LIBRARY: ${BearLibTerminal_RUNTIME_LIBRARY}")
message(STATUS "BearLibTerminal_INCLUDE_DIR: ${BearLibTerminal_INCLUDE_DIR}")
