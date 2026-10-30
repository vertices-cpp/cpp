# ============================================================
# Path: game/orange/external/minizip/
# ============================================================

cmake_minimum_required(VERSION 3.17)

set(lib_name minizip)
set(target_name ${lib_name})

project(${lib_name} C CXX)

# ============================================================
# Source files
# ============================================================
set(${target_name}_src
    ${CMAKE_CURRENT_SOURCE_DIR}/zip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/unzip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ioapi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ioapi_mem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/crypt.cpp
)

set(${target_name}_header
    ${CMAKE_CURRENT_SOURCE_DIR}/crypt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ioapi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ioapi_mem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/unzip.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zip.h
)

if(WIN32)
    list(APPEND ${target_name}_src "${CMAKE_CURRENT_SOURCE_DIR}/iowin32.cpp")
    list(APPEND ${target_name}_header "${CMAKE_CURRENT_SOURCE_DIR}/iowin32.h")
endif()

# ============================================================
# Library
# ============================================================
add_library(${target_name} STATIC
    ${${target_name}_header}
    ${${target_name}_src}
)

target_include_directories(${target_name} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================
# zlib headers
# ============================================================
if(TARGET zlib)
    get_target_property(zlib_header zlib INTERFACE_INCLUDE_DIRECTORIES)
    if(zlib_header)
        target_include_directories(${target_name} PRIVATE ${zlib_header})
    endif()
endif()

# ============================================================
# Output
# ============================================================
set_target_properties(${target_name} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    FOLDER "External"
)

# ============================================================
# VS grouping
# ============================================================
if(MSVC OR XCODE)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
                 PREFIX "Source Files"
                 FILES ${${target_name}_header} ${${target_name}_src})
endif()