#[[路径: orange/external/zlib/]]

cmake_minimum_required(VERSION 3.17)
project(zlib_lib C)

set(ZLIB_PUBLIC_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/zconf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zlib.h
)

set(ZLIB_PRIVATE_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/crc32.h
    ${CMAKE_CURRENT_SOURCE_DIR}/deflate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gzguts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inffast.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inffixed.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inflate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inftrees.h
    ${CMAKE_CURRENT_SOURCE_DIR}/trees.h
    ${CMAKE_CURRENT_SOURCE_DIR}/zutil.h
)

set(ZLIB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/adler32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/compress.c
    ${CMAKE_CURRENT_SOURCE_DIR}/crc32.c
    ${CMAKE_CURRENT_SOURCE_DIR}/deflate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gzclose.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gzlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gzread.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gzwrite.c
    ${CMAKE_CURRENT_SOURCE_DIR}/inflate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/infback.c
    ${CMAKE_CURRENT_SOURCE_DIR}/inftrees.c
    ${CMAKE_CURRENT_SOURCE_DIR}/inffast.c
    ${CMAKE_CURRENT_SOURCE_DIR}/trees.c
    ${CMAKE_CURRENT_SOURCE_DIR}/uncompr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/zutil.c
)

set(ALL_ZLIB_FILES ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS} ${ZLIB_SRCS})

add_library(zlib STATIC ${ALL_ZLIB_FILES})

target_include_directories(zlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

#set_target_properties(zlib PROPERTIES FOLDER "External")   # ★ 项目挂在 External 下

if(MSVC OR XCODE)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
                 PREFIX "Source Files"    # ★ 项目内部根分组
                 FILES ${ALL_ZLIB_FILES})
endif()