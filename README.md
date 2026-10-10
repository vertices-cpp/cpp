# cpp_scmall_tool

一个 C++ 小工具集合，包含寻路、字符串匹配、压缩、编码转换、CMake 工程等示例。

## 简介

每个示例都是独立的 CMake 子项目，可以单独编译运行。主要包括：

- A* / JPS 寻路
- KMP / Sunday 字符串匹配
- Huffman / Packbits / Sega RLE-LZSS 压缩
- NES 11x11 字库转换
- MMM 地图数据转换
- iconv 编码转换
- zip 压缩 / 解压（minizip-ng 1.2）
- CMake / C++ / 文件遍历生成路径
- 二叉树、Huffman 树等数据结构示例

## 编译

需要 CMake 3.17+、C++11 / C++17。

Windows：

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A Win32
cmake --build . --config Debug
Linux / macOS：

bash
mkdir build
cd build
cmake ..
make -j
可执行文件在 build/bin/ 下。

运行
运行 bin/<Config>/ 下的 exe，按提示输入即可。资源目录 Resources/ 会在编译时自动同步到 exe 同级目录。

依赖
内置第三方库：zlib、tinyxml2、minizip-ng 1.2。

说明
主要用于学习 / 研究，代码风格不统一，部分示例为移植或改写。部分示例依赖 Windows API，Linux / macOS 下可能无法编译。

许可
仅供学习交流使用。第三方库遵循其各自许可。
