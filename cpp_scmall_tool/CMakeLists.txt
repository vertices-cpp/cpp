cmake_minimum_required(VERSION 3.17)
project(cpp_scmall_tool C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ============================================================
# ★ 全局资源镜像（只做一次）
# ============================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/Resources")
    add_custom_target(SyncRes ALL
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/Resources"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Resources"
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/Resources"
        COMMENT "Mirror Resources -> $<CONFIG>"
        VERBATIM)
    set_target_properties(SyncRes PROPERTIES FOLDER "Utils")
endif()

# ============================================================
# ★ 第三方库（全局一份）
# ============================================================
add_subdirectory(external)

# ============================================================
# ★ 公共 C++ 库
# ============================================================
add_subdirectory(example/common)

# ============================================================
# ★ 所有示例
# ============================================================
add_subdirectory(example/001_jps)
add_subdirectory(example/002_jps)
add_subdirectory(example/003_jps_win32)
add_subdirectory(example/004_kmp)
add_subdirectory(example/005_sunday_search)
add_subdirectory(example/006_Packbits_konami)
add_subdirectory(example/007_binary_tree)
add_subdirectory(example/008_huffman_tree)
add_subdirectory(example/009_huffman_compression)
add_subdirectory(example/010_huffman_compression)
add_subdirectory(example/011_huffman_compression)
add_subdirectory(example/012_sega_rle_lzss_compression)
add_subdirectory(example/013_convert_mmm)
add_subdirectory(example/014_iconv)