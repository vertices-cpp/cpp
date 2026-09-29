#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	// 检查是否有拖入的文件/文件夹（argv[1] 即为拖入路径）
	if (argc < 2) {
		return 1;
	}

	fs::path target_path(argv[1]);

	if (!fs::exists(target_path)) {
		return 1;
	}

	// 结果生成在 exe 所在的同级目录下，名为 output.txt
	std::ofstream outFile("output.txt", std::ios::trunc);
	if (!outFile.is_open()) {
		return 1;
	}

	auto write_entry = [&](const fs::directory_entry& entry) {
		try {
			// 获取相对于拖入目标父目录的相对路径
			auto rel_path = fs::relative(entry.path(), target_path.parent_path());
			outFile << rel_path.string() << "\n";
		}
		catch (...) {
			// 忽略读取失败的项
		}
	};

	if (fs::is_regular_file(target_path)) {
		write_entry(fs::directory_entry(target_path));
	}
	else if (fs::is_directory(target_path)) {
		auto options = fs::directory_options::skip_permission_denied;
		for (const auto& entry : fs::recursive_directory_iterator(target_path, options)) {
			write_entry(entry);
		}
	}

	outFile.close();
	return 0;
}