cmake_minimum_required(VERSION 3.17)

# ============================================================
# ★ 唯一要改的地方：项目名
# ============================================================
set(APP_NAME 003_jps)
# ============================================================

project(${APP_NAME} C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(MSVC)
    add_compile_options(/utf-8 /MP /W3 /Gm-)
    add_compile_options($<$<CONFIG:Release>:/O2>)
endif()

# ---------------- 自动扫描源文件 ----------------
file(GLOB_RECURSE _srcs CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
list(FILTER _srcs EXCLUDE REGEX "/build[^/]*/")
list(FILTER _srcs EXCLUDE REGEX "/\\.vs/")
list(FILTER _srcs EXCLUDE REGEX "/out/")

# ---------------- 目标 ----------------
# 注意：纯控制台程序不要 WIN32
add_executable(${APP_NAME} ${_srcs})

target_include_directories(${APP_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# 链接公共库（按需）
target_link_libraries(${APP_NAME} PRIVATE
    common
    # external   # 如果示例直接用到第三方，才 link
)

# ---------------- 依赖 SyncRes ----------------
if(TARGET SyncRes)
    add_dependencies(${APP_NAME} SyncRes)
endif()

# ---------------- VS 分组 ----------------
if(MSVC)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
                 PREFIX "Source Files"
                 FILES ${_srcs})
endif()

set_target_properties(${APP_NAME} PROPERTIES FOLDER "Test")