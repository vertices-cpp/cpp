cmake_minimum_required(VERSION 3.17)
project(common CXX)

# 扫描所有源文件 + 头文件
file(GLOB COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
list(FILTER COMMON_SOURCES EXCLUDE REGEX "/build[^/]*/")
list(FILTER COMMON_SOURCES EXCLUDE REGEX "/\\.vs/")
list(FILTER COMMON_SOURCES EXCLUDE REGEX "/out/")

add_library(common STATIC ${COMMON_SOURCES})

target_include_directories(common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# ============================================================
# ★ 第三方依赖（按需）
# ============================================================
# 如果 common 里用到了 zlib / tinyxml2 / freetype，才 link
target_link_libraries(common PUBLIC
    zlib
    tinyxml2
    # freetype   # 如果 common 没用到 freetype，不用 link
    # unzip      # 如果 common 没用到 unzip，不用 link
)

# VS 分组
if(MSVC)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
                 PREFIX "Source Files"
                 FILES ${COMMON_SOURCES})
endif()

set_target_properties(common PROPERTIES FOLDER "Common")

# 调试宏
target_compile_definitions(common PUBLIC
    $<$<CONFIG:Debug>:ORANGE_DEBUG=1>
)

# Windows 平台宏
if(WIN32)
    target_compile_definitions(common PUBLIC
        WIN32 _WIN32 _WINDOWS UNICODE _UNICODE
        _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
    )
endif()

if(MSVC)
    target_compile_options(common PUBLIC /MP)
endif()