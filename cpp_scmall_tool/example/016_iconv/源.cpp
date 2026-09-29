#include "iconv_wrapper.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "path_head.h"

static std::string prePath = og::checkPath("016_iconv/");

static std::string read_file(const std::string& path) {
	std::ifstream ifs(prePath + path, std::ios::binary);
	if (!ifs) throw std::runtime_error("cannot open: " + path);
	std::ostringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

static void write_file(const std::string& path, const std::string& data) {
	std::ofstream ofs(prePath + path, std::ios::binary);
	if (!ofs) throw std::runtime_error("cannot write: " + path);
	ofs.write(data.data(), data.size());
}

static bool compare_files(const std::string& a, const std::string& b) {
	return read_file(a) == read_file(b);
}

int main(int argc, char** argv) {
	//     if (argc < 2) {
	//         std::cerr << "usage: " << argv[0] << " <gbk_input_file>\n";
	//         return 1;
	//     }
	//     std::string in_path = argv[1];
	std::string in_path = "gbk.txt";
	std::string gbk_data = read_file(in_path);
	std::cout << "input size: " << gbk_data.size() << " bytes\n";

	// 1. GBK -> UTF-8
	std::string utf8 = iconv_wrapper(gbk_data, Iconv::GbkToUtf8);
	write_file("utf8.txt", utf8);
	std::cout << "utf8 size: " << utf8.size() << "\n";

	// 2. UTF-8 -> GBK (round-trip)
	std::string gbk_roundtrip = iconv_wrapper(utf8, Iconv::Utf8ToGbk);
	write_file("gbk_roundtrip.txt", gbk_roundtrip);
	bool ok = (gbk_roundtrip == gbk_data);
	std::cout << "GBK->UTF8->GBK round-trip: " << (ok ? "OK" : "FAIL") << "\n";

	// 3. UTF-8 -> UTF-16LE
	std::string utf16le = iconv_wrapper(utf8, Iconv::Utf8ToUtf16le);
	write_file("utf16le.txt", utf16le);
	std::cout << "utf16le size: " << utf16le.size() << "\n";

	// 4. UTF-16LE -> UTF-8 (round-trip)
	std::string utf8_from_utf16 = iconv_wrapper(utf16le, Iconv::Utf16leToUtf8);
	write_file("utf8_from_utf16.txt", utf8_from_utf16);
	ok = (utf8_from_utf16 == utf8);
	std::cout << "UTF8->UTF16->UTF8 round-trip: " << (ok ? "OK" : "FAIL") << "\n";

	// 5. UTF-8 -> UTF-32LE
	std::string utf32le = iconv_wrapper(utf8, Iconv::Utf8ToUtf32le);
	write_file("utf32le.txt", utf32le);
	std::cout << "utf32le size: " << utf32le.size() << "\n";

	// 6. UTF-32LE -> UTF-8 (round-trip)
	std::string utf8_from_utf32 = iconv_wrapper(utf32le, Iconv::Utf32leToUtf8);
	write_file("utf8_from_utf32.txt", utf8_from_utf32);
	ok = (utf8_from_utf32 == utf8);
	std::cout << "UTF8->UTF32->UTF8 round-trip: " << (ok ? "OK" : "FAIL") << "\n";

	// 7. UTF-16LE -> GBK
	std::string gbk_from_utf16 = iconv_wrapper(utf16le, Iconv::Utf16leToUtf8);
	// There is actually another path here: it needs to go through
	// Utf16leToUtf8 and then Utf8ToGbk, or do it manually,
	// because auto_func has no direct utf16->gbk path.
	// So we can only do UTF16->UTF8->GBK first.

	return 0;
}