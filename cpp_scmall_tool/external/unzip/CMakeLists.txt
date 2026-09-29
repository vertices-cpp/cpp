#[[ 路径: game/orange/external/unzip/ ]]

cmake_minimum_required(VERSION 3.17)

set(lib_name unzip)
set(target_name ${lib_name})

project(${lib_name})

set(${target_name}_src
    ${CMAKE_CURRENT_SOURCE_DIR}/ioapi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/unzip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ioapi_mem.cpp
)

set(${target_name}_header
    ${CMAKE_CURRENT_SOURCE_DIR}/crypt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ioapi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ioapi_mem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/unzip.h
)

add_library(${target_name} STATIC
    ${${target_name}_header}
    ${${target_name}_src}
)

target_include_directories(${target_name} PUBLIC .)

# 只把 zlib 的 include 路径借过来，不链接
if(MACOSX OR ANDROID OR WIN32)
    get_target_property(zlib_header zlib INTERFACE_INCLUDE_DIRECTORIES)
    if(zlib_header)
        target_include_directories(${target_name} PRIVATE ${zlib_header})
    endif()
endif()

set_target_properties(${target_name} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  #  FOLDER "External"          # ★
)

if(MSVC OR XCODE)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
                 PREFIX "Source Files"   # ★
                 FILES ${${target_name}_header} ${${target_name}_src})
endif()