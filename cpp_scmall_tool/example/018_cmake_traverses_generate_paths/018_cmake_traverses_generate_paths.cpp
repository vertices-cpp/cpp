#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <algorithm>
#include <cctype>
namespace fs = std::filesystem;

// 判断文件名是否包含 "cmake"（不区分大小写）
bool containsIgnoreCase(const std::string& str, const std::string& sub) {
	auto it = std::search(
		str.begin(), str.end(),
		sub.begin(), sub.end(),
		[](char ch1, char ch2) {
		return std::tolower(static_cast<unsigned char>(ch1)) ==
			std::tolower(static_cast<unsigned char>(ch2));
	}
	);
	return it != str.end();
}

// 处理单个文件写入
void processFile(const fs::path& filePath, std::ofstream& outFile) {
	std::string filename = filePath.filename().string();
	if (containsIgnoreCase(filename, "cmake")) {
		// 1. 写入路径
		outFile << filePath.string() << "\n";

		// 2. 换行写入文件内容
		std::ifstream file(filePath);
		if (file.is_open()) {
			outFile << file.rdbuf() << "\n";
		}
		else {
			outFile << "[无法打开该文件]\n";
		}
		outFile << "\n----------------------------------------\n\n";
	}
}

// 递归遍历文件夹写入
void processDirectory(const fs::path& dirPath, std::ofstream& outFile) {
	try {
		for (const auto& entry : fs::recursive_directory_iterator(dirPath)) {
			if (entry.is_regular_file()) {
				processFile(entry.path(), outFile);
			}
		}
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "遍历错误: " << e.what() << std::endl;
	}
}

int main(int argc, char* argv[]) {
	// 文本输出目标文件
	fs::path outputPath = fs::current_path() / "cmake_sources.txt";
	std::ofstream outFile(outputPath, std::ios::trunc);

	if (!outFile.is_open()) {
		std::cerr << "无法创建或写入输出文件: " << outputPath.string() << std::endl;
		return 1;
	}

	// 1. 如果没有命令行参数，提示拖入路径
	if (argc < 2) {
		std::cout << "请将 [文件夹] 或 [文件] 拖入此窗口（或直接拖到 .exe 图标上），然后按回车：\n";
		std::string inputPath;
		std::getline(std::cin, inputPath);

		// 清理 Windows 拖入路径自动添加的双引号
		if (!inputPath.empty() && inputPath.front() == '"' && inputPath.back() == '"') {
			inputPath = inputPath.substr(1, inputPath.length() - 2);
		}

		if (!inputPath.empty()) {
			fs::path targetPath(inputPath);
			if (fs::is_directory(targetPath)) {
				processDirectory(targetPath, outFile);
			}
			else if (fs::is_regular_file(targetPath)) {
				processFile(targetPath, outFile);
			}
		}
	}
	// 2. 支持直接拖入文件/文件夹到 .exe 上运行
	else {
		for (int i = 1; i < argc; ++i) {
			fs::path targetPath(argv[i]);
			if (fs::is_directory(targetPath)) {
				processDirectory(targetPath, outFile);
			}
			else if (fs::is_regular_file(targetPath)) {
				processFile(targetPath, outFile);
			}
		}
	}

	outFile.close();
	std::cout << "\n导出成功！文件已生成至:\n" << outputPath.string() << std::endl;
	std::cout << "按回车键退出...";
	std::cin.get();
	return 0;
}