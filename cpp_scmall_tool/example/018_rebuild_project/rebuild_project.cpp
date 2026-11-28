#include <iostream>
#include <filesystem>
#include <system_error>
#include <cstdlib>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

bool iequals(const std::string& a, const std::string& b) {
	if (a.size() != b.size()) return false;
	return std::equal(a.begin(), a.end(), b.begin(),
		[](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) ==
			std::tolower(static_cast<unsigned char>(b));
	});
}

// ★ 白名单：这些文件 / 目录不会被删（忽略大小写）
static const std::vector<std::string> KEEP_LIST = {
	"main.cpp",
	"main.h",
	"resource.h",
	"game.rc",
	"res",           // 目录也可以列
	"CMakeLists.txt", // 如果想保留自己的 cmake 脚本
	"create_directory.txt"
};

// 判断名字是否在白名单里
bool is_kept(const std::string& name) {
	for (const auto& keep : KEEP_LIST) {
		if (iequals(name, keep)) return true;
	}
	return false;
}

// 清理 proj.win32 目录（保留白名单文件）
void cleanup_win32_dir(const fs::path& target_dir) {
	if (!fs::exists(target_dir)) {
		std::cout << "[WARN] 目录不存在: " << target_dir << std::endl;
		return;
	}

	std::error_code ec;
	for (const auto& entry : fs::directory_iterator(target_dir, ec)) {
		if (ec) {
			std::cerr << "[ERROR] 读取目录出错: " << ec.message() << std::endl;
			continue;
		}

		fs::path filename = entry.path().filename();

		// ★ 白名单判断
		if (is_kept(filename.string())) {
			std::cout << "[SKIP] 跳过保留项: " << entry.path() << std::endl;
			continue;
		}

		std::uintmax_t removed_count = fs::remove_all(entry.path(), ec);
		if (ec) {
			std::cerr << "[ERROR] 删除失败 " << entry.path() << ": " << ec.message() << std::endl;
			ec.clear();
		}
		else {
			std::cout << "[CLEAN] 已删除 (" << removed_count << " 项): " << entry.path() << std::endl;
		}
	}
}

int main() {
	fs::path win32_path = fs::current_path() / "proj.win32";

	std::cout << "=== 1. 开始清理目录: " << win32_path << " ===" << std::endl;
	cleanup_win32_dir(win32_path);

	std::cout << "\n=== 2. 执行 CMake 构建命令 ===" << std::endl;

	std::string cmake_cmd = "cd /d \"" + win32_path.string() +
		"\" && cmake .. -G \"Visual Studio 15 2017\" -A win32";

	std::cout << "[EXEC] " << cmake_cmd << std::endl;

	int ret = std::system(cmake_cmd.c_str());

	if (ret == 0) {
		std::cout << "\n[SUCCESS] CMake 工程生成成功！" << std::endl;
	}
	else {
		std::cerr << "\n[FAILED] CMake 指令执行失败，退出码: " << ret << std::endl;
	}

	return 0;
}