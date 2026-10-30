// scan_includes.cpp
// C++17
// 用法：scan_includes.exe <目录或文件路径> [更多路径...]

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

// ============================================================
// 是否扫描这个文件
// ============================================================
static bool isTargetFile(const fs::path& p)
{
	auto ext = p.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return (char)std::tolower(c); });

	static const char* exts[] = {
		".cpp", ".h", ".cc", ".hpp", ".cxx", ".c",
		".inl", ".ipp", ".tpp"
	};
	for (auto e : exts)
		if (ext == e) return true;

	return false;
}

// ============================================================
// 判断 pos 前面的行首到 pos 之间，是否有 // 或 /* 注释
// ============================================================
static bool isCommentedOut(const std::string& content, size_t pos)
{
	size_t lineStart = content.rfind('\n', pos);
	lineStart = (lineStart == std::string::npos) ? 0 : lineStart + 1;

	std::string prefix = content.substr(lineStart, pos - lineStart);

	size_t i = 0;
	while (i < prefix.size() && (prefix[i] == ' ' || prefix[i] == '\t')) ++i;
	prefix = prefix.substr(i);

	if (prefix.size() >= 2 &&
		(prefix.compare(0, 2, "//") == 0 || prefix.compare(0, 2, "/*") == 0))
		return true;

	return false;
}

// ============================================================
// 从一段文本里找所有 #include 行
// 返回每行的原文（去 \r）
// ============================================================
static void parseIncludes(const std::string& content,
	std::vector<std::string>& out)
{
	const std::string key = "#include";
	size_t pos = 0;

	while ((pos = content.find(key, pos)) != std::string::npos)
	{
		if (isCommentedOut(content, pos))
		{
			pos += key.size();
			continue;
		}

		size_t lineStart = content.rfind('\n', pos);
		lineStart = (lineStart == std::string::npos) ? 0 : lineStart + 1;

		size_t lineEnd = content.find('\n', pos);
		if (lineEnd == std::string::npos) lineEnd = content.size();

		std::string line = content.substr(lineStart, lineEnd - lineStart);
		if (!line.empty() && line.back() == '\r') line.pop_back();

		out.push_back(std::move(line));

		pos = lineEnd + 1;
	}
}

// ============================================================
// 扫一个文件
// ============================================================
static void scanFile(const fs::path& file,
	std::vector<std::pair<std::string, std::vector<std::string>>>& out)
{
	std::ifstream ifs(file, std::ios::binary);
	if (!ifs) return;

	std::string content((std::istreambuf_iterator<char>(ifs)),
		std::istreambuf_iterator<char>());

	std::vector<std::string> lines;
	parseIncludes(content, lines);

	if (lines.empty()) return;   // 没 include 的文件不输出

	out.emplace_back(file.string(), std::move(lines));
}

// ============================================================
// main
// ============================================================
int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "用法: " << argv[0] << " <目录或文件路径> [更多路径...]\n";
		return 1;
	}

	// { 文件路径, 该文件的所有 include 行 }
	std::vector<std::pair<std::string, std::vector<std::string>>> results;

	fs::path outDir = fs::current_path();

	for (int a = 1; a < argc; ++a)
	{
		fs::path p = fs::path(argv[a]);

		if (!fs::exists(p))
		{
			std::cerr << "路径不存在: " << p << "\n";
			continue;
		}

		if (a == 1)
		{
			outDir = p.parent_path();
			if (outDir.empty()) outDir = fs::current_path();
		}

		if (fs::is_regular_file(p))
		{
			if (isTargetFile(p))
				scanFile(p, results);
		}
		else if (fs::is_directory(p))
		{
			for (auto& entry : fs::recursive_directory_iterator(p,
				fs::directory_options::skip_permission_denied))
			{
				if (!entry.is_regular_file()) continue;
				const auto& f = entry.path();
				if (!isTargetFile(f)) continue;
				scanFile(f, results);
			}
		}
	}

	// 输出
	fs::path outPath = outDir / "cpp_include.txt";
	std::ofstream ofs(outPath, std::ios::binary);
	if (!ofs)
	{
		std::cerr << "无法写入: " << outPath << "\n";
		return 1;
	}

	for (auto& kv : results)
	{
		// 文件路径，只打一次
		ofs << kv.first << "\n";

		// 该文件的所有 include 行
		for (auto& line : kv.second)
			ofs << "    " << line << "\n";

		// 文件之间空一行，可选
		ofs << "\n";
	}

	ofs.close();

	if (ofs.fail())
	{
		std::cerr << "写入失败: " << outPath << "\n";
		return 1;
	}

	size_t total = 0;
	for (auto& kv : results) total += kv.second.size();

	std::cout << "扫描完成，共 " << results.size() << " 个文件，"
		<< total << " 条 include，输出: " << outPath << "\n";
	return 0;
}