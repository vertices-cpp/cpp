#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <cctype>

namespace fs = std::filesystem;

// 检查文件扩展名是否为 C/C++ 源码或头文件
bool is_cpp_source_or_header(const fs::path& p) {
	static const std::unordered_set<std::string> valid_extensions = {
		".c", ".cpp", ".cxx", ".cc", ".h", ".hpp", ".hxx"
	};
	std::string ext = p.extension().generic_string();
	for (char& c : ext) c = static_cast<char>(std::tolower(c));
	return valid_extensions.count(ext) > 0;
}

// 获取扩展名的排序优先级：.h(1) -> .hpp(2) -> .c(3) -> .cpp(4) -> 其他(5)
int get_ext_priority(const std::string& filename) {
	std::string ext = fs::path(filename).extension().generic_string();
	for (char& c : ext) c = static_cast<char>(std::tolower(c));

	if (ext == ".h")   return 1;
	if (ext == ".hpp") return 2;
	if (ext == ".c")   return 3;
	if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") return 4;
	return 5;
}

// 自定义比较函数：优先级高的在前；优先级相同的按文件名字母顺序排序
bool compare_files(const std::string& a, const std::string& b) {
	int prio_a = get_ext_priority(a);
	int prio_b = get_ext_priority(b);

	if (prio_a != prio_b) {
		return prio_a < prio_b;
	}
	return a < b; // 同类后缀按字母序 (A-Z)
}

void collect_sources(const fs::path& target_path, std::map<fs::path, std::vector<std::string>>& dir_files_map) {
	std::error_code ec;
	if (!fs::exists(target_path, ec)) return;

	if (fs::is_regular_file(target_path, ec)) {
		if (is_cpp_source_or_header(target_path)) {
			dir_files_map[target_path.parent_path()].push_back(target_path.filename().generic_string());
		}
		return;
	}

	if (fs::is_directory(target_path, ec)) {
		auto options = fs::directory_options::skip_permission_denied;
		for (const auto& entry : fs::recursive_directory_iterator(target_path, options, ec)) {
			if (entry.is_regular_file(ec) && is_cpp_source_or_header(entry.path())) {
				fs::path parent_dir = entry.path().parent_path();
				std::string file_name = entry.path().filename().generic_string();
				dir_files_map[parent_dir].push_back(file_name);
			}
		}
	}
}

int main(int argc, char* argv[]) {
	std::cout << "=== CMake C/C++ 源码树提取器 (后缀分类排序版) ===\n\n";

	if (argc < 2) {
		std::cout << "提示：请直接将包含源码的文件夹【拖入此程序图标上】。\n\n";
		std::cout << "按任意键退出...";
		std::cin.get();
		return 0;
	}

	std::map<fs::path, std::vector<std::string>> dir_files_map;

	for (int i = 1; i < argc; ++i) {
		fs::path input_path(argv[i]);
		collect_sources(input_path, dir_files_map);
	}

	std::string output_filename = "cpp_sources.txt";
	std::ofstream out_file(output_filename, std::ios::out | std::ios::trunc);

	if (!out_file.is_open()) {
		std::cerr << "错误：无法创建输出文件 " << output_filename << std::endl;
		return 1;
	}

	out_file << "# CMake Source Files Auto-generated List\n";
	out_file << "set(SOURCE_FILES\n";

	for (auto&[dir_path, files] : dir_files_map) {
		// 对该目录下的文件按照 .h -> .hpp -> .c -> .cpp 排序
		std::sort(files.begin(), files.end(), compare_files);

		out_file << dir_path.generic_string() << "\n";
		for (const auto& file : files) {
			out_file << "    " << file << "\n";
		}
	}

	out_file << ")\n";
	out_file.close();

	std::cout << "完成！CMake 列表已保存至: " << fs::absolute(output_filename).string() << "\n";
	std::cout << "按任意键退出...";
	std::cin.get();

	return 0;
}