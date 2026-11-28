#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdlib>

#include <tinyxml2.h>
#include <zlib.h>
#include "json.hpp"
#include "base64.h"

using namespace tinyxml2;
using namespace orange;
using json = nlohmann::json;

// 强化版 CSV 解析：无视换行符、空格、制表符，只提取纯数字
#include <cstdlib>
#include <cctype>

// 钢铁防弹版 CSV 解析：无视换行、空格、BOM头、以及任何非法字符
static std::vector<int> splitCsv(const std::string& s)
{
	std::vector<int> res;
	std::string cleanStr;

	// 1. 过滤掉 BOM 头 (EF BB BF) 和所有非数字/非负号/非逗号的字符
	for (size_t i = 0; i < s.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(s[i]);
		// 跳过 UTF-8 BOM
		if (i == 0 && c == 0xEF) { i += 2; continue; }
		if (i == 0 && c == 0xBB) { i += 2; continue; }

		// 只保留数字、负号和逗号（如果你想支持空格分隔，这里也可以保留空格）
		if (std::isdigit(c) || c == '-' || c == ',') {
			cleanStr += c;
		}
		// 遇到换行、空格、回车、奇怪的乱码，都自动忽略
	}

	// 2. 安全地解析数字
	std::string numStr;
	for (char c : cleanStr) {
		if (c == ',' || c == '\n' || c == '\r') {
			if (!numStr.empty()) {
				// 使用 strtol 替代 stoi，它绝不会抛异常，且能处理超长数字
				char* endptr;
				long val = std::strtol(numStr.c_str(), &endptr, 10);
				if (endptr != numStr.c_str()) {
					res.push_back(static_cast<int>(val));
				}
				numStr.clear();
			}
		}
		else {
			numStr += c;
		}
	}

	// 处理最后一个数字
	if (!numStr.empty()) {
		char* endptr;
		long val = std::strtol(numStr.c_str(), &endptr, 10);
		if (endptr != numStr.c_str()) {
			res.push_back(static_cast<int>(val));
		}
	}

	return res;
}

// 一维数组转带换行的 CSV 字符串（最后没有逗号）
static std::string toCsv(const std::vector<int>& arr, int width)
{
	std::stringstream ss;
	for (size_t i = 0; i < arr.size(); ++i) {
		ss << arr[i];
		if (i != arr.size() - 1) {
			ss << ",";
		}
		if (width > 0 && (i + 1) % static_cast<size_t>(width) == 0 && i != arr.size() - 1) {
			ss << "\n";
		}
	}
	return ss.str();
}

// 解压 gzip 字节流
static bool decompressGzip(const unsigned char* src, size_t srcLen, std::vector<unsigned char>& out)
{
	z_stream strm{};
	if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
		return false;
	}

	strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(src));
	strm.avail_in = static_cast<uInt>(srcLen);

	out.clear();
	out.resize(srcLen * 4 + 1024);

	int ret = Z_OK;
	while (true) {
		if (strm.total_out >= out.size()) {
			if (out.size() >= 1024 * 1024 * 50) {
				inflateEnd(&strm);
				return false;
			}
			out.resize(out.size() * 2);
		}
		strm.next_out = reinterpret_cast<Bytef*>(out.data() + strm.total_out);
		strm.avail_out = static_cast<uInt>(out.size() - strm.total_out);

		ret = inflate(&strm, Z_NO_FLUSH);
		if (ret == Z_STREAM_END) {
			out.resize(strm.total_out);
			break;
		}
		if (ret != Z_OK) {
			inflateEnd(&strm);
			return false;
		}
	}

	inflateEnd(&strm);
	return true;
}

// 将解压/解码后的小端字节流按每 4 字节解析为图块 GID 数组
static void appendLittleEndianTiles(const unsigned char* bytes, size_t byteLen, std::vector<int>& outGids)
{
	size_t numTiles = byteLen / 4;
	outGids.reserve(outGids.size() + numTiles);
	for (size_t i = 0; i < numTiles; ++i) {
		uint32_t gid = static_cast<uint32_t>(bytes[i * 4])
			| (static_cast<uint32_t>(bytes[i * 4 + 1]) << 8)
			| (static_cast<uint32_t>(bytes[i * 4 + 2]) << 16)
			| (static_cast<uint32_t>(bytes[i * 4 + 3]) << 24);
		outGids.push_back(static_cast<int>(gid));
	}
}

// 解析 TMX 图块层 <data>：支持 csv、base64（无压缩 / zlib / gzip）
static bool parseTileData(XMLElement* dataNode, const char* layerName, std::vector<int>& outGids)
{
	outGids.clear();
	if (!dataNode) return false;

	const char* safeName = layerName ? layerName : "Unnamed";
	const char* encoding = dataNode->Attribute("encoding");
	const char* compression = dataNode->Attribute("compression");
	std::string encStr = encoding ? encoding : "";
	std::string compStr = compression ? compression : "";

	if (encStr == "csv") {
		std::string csvText = dataNode->GetText() ? dataNode->GetText() : "";
		outGids = splitCsv(csvText);
		return true;
	}

	if (encStr == "base64") {
		std::string b64Text = dataNode->GetText() ? dataNode->GetText() : "";
		b64Text.erase(std::remove(b64Text.begin(), b64Text.end(), '\n'), b64Text.end());
		b64Text.erase(std::remove(b64Text.begin(), b64Text.end(), '\r'), b64Text.end());

		unsigned char* binData = nullptr;
		int binSize = base64Decode(
			reinterpret_cast<const unsigned char*>(b64Text.c_str()),
			static_cast<unsigned int>(b64Text.length()),
			&binData
		);

		if (binSize <= 0 || binData == nullptr) {
			std::cerr << "Error: Base64 decode failed or empty for layer '" << safeName << "'.\n";
			if (binData) std::free(binData);
			return false;
		}

		if (compStr == "zlib") {
			size_t uncompressedSize = static_cast<size_t>(binSize) * 4 + 1024;
			std::vector<unsigned char> unzipped(uncompressedSize);
			uLongf destLen = static_cast<uLongf>(unzipped.size());

			int z_res = uncompress(unzipped.data(), &destLen, binData, static_cast<uLong>(binSize));
			while (z_res == Z_BUF_ERROR && uncompressedSize < 1024 * 1024 * 50) {
				uncompressedSize *= 2;
				unzipped.resize(uncompressedSize);
				destLen = static_cast<uLongf>(unzipped.size());
				z_res = uncompress(unzipped.data(), &destLen, binData, static_cast<uLong>(binSize));
			}

			std::free(binData);
			if (z_res != Z_OK) {
				std::cerr << "Error: Zlib uncompress failed for layer '" << safeName << "'! Code: " << z_res << "\n";
				return false;
			}

			unzipped.resize(destLen);
			appendLittleEndianTiles(unzipped.data(), unzipped.size(), outGids);
			return true;
		}

		if (compStr == "gzip") {
			std::vector<unsigned char> unzipped;
			bool ok = decompressGzip(binData, static_cast<size_t>(binSize), unzipped);
			std::free(binData);
			if (!ok) {
				std::cerr << "Error: Gzip decompress failed for layer '" << safeName << "'.\n";
				return false;
			}

			appendLittleEndianTiles(unzipped.data(), unzipped.size(), outGids);
			return true;
		}

		if (compStr.empty()) {
			appendLittleEndianTiles(binData, static_cast<size_t>(binSize), outGids);
			std::free(binData);
			return true;
		}

		std::cerr << "Error: Unsupported compression '" << compStr << "' in layer '" << safeName << "'.\n";
		std::free(binData);
		return false;
	}

	std::cerr << "Error: Unsupported encoding '" << encStr << "' in layer '" << safeName << "'.\n";
	return false;
}

struct ScreenInfo
{
	std::vector<std::vector<int>> rows;
	int w;
	int h;
};

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cout << "Usage:\n";
		std::cout << "  Single Mode: TmxMerge.exe <screens.json> [-w <width>]\n";
		std::cout << "  Merge Mode : TmxMerge.exe <base.tmx> <screens.json> [-w <width>]\n";
		return -1;
	}

	std::string path1 = argv[1];
	std::string path2 = (argc >= 3) ? argv[2] : "";
	std::string tmxPath = "";
	std::string jsonPath = "";

	// 解析参数：判断是单 JSON 还是双文件
	if (path2.empty() || path2.find(".tmx") != std::string::npos || path1.find(".tmx") != std::string::npos) {
		if (path1.find(".tmx") != std::string::npos) tmxPath = path1;
		else if (path1.find(".json") != std::string::npos) jsonPath = path1;

		if (path2.find(".tmx") != std::string::npos) tmxPath = path2;
		else if (path2.find(".json") != std::string::npos) jsonPath = path2;
	}
	else {
		tmxPath = path1;
		jsonPath = path2;
	}

	int forceScrW = -1;
	for (int i = 1; i < argc; i++) {
		if (std::string(argv[i]) == "-w" && i + 1 < argc) {
			forceScrW = std::stoi(argv[i + 1]);
		}
	}

	if (jsonPath.empty()) {
		std::cerr << "No JSON file provided.\n";
		return -1;
	}

	// 读取 JSON
	std::ifstream fin(jsonPath);
	if (!fin.is_open()) {
		std::cerr << "Cannot open JSON: " << jsonPath << "\n";
		return -1;
	}
	json j;
	fin >> j;
	auto& screens = j["Screens"];

	if (screens.empty()) {
		std::cerr << "JSON has no screens.\n";
		return -1;
	}

	// 获取屏幕数据
	std::vector<ScreenInfo> allScreenInfo;
	int totalAddWidth = 0;
	int baseH = 0;

	for (auto& scr : screens)
	{
		int scrH = scr["height"].get<int>();
		int scrW = (forceScrW > 0) ? forceScrW : scr["width"].get<int>();

		auto& lay = scr["layers"][0];
		std::vector<int> dArr = lay["data"].get<std::vector<int>>();

		if (dArr.size() != static_cast<size_t>(scrW * scrH))
		{
			std::cerr << "Warning: Data size (" << dArr.size() << ") doesn't match " << scrW << "x" << scrH << "! Auto-adjusting...\n";
			scrW = static_cast<int>(dArr.size()) / scrH;
			std::cerr << "Adjusted width to: " << scrW << "\n";
		}

		ScreenInfo info;
		info.w = scrW;
		info.h = scrH;
		info.rows.resize(scrH, std::vector<int>(scrW));

		for (int y = 0; y < scrH; y++)
		{
			for (int x = 0; x < scrW; x++)
			{
				// 默认 +1，如果不需要 +1 可以自行去掉
				int val = dArr[static_cast<size_t>(y) * scrW + x];
				info.rows[y][x] = val + 1;
			}
		}
		if (baseH == 0) baseH = scrH;
		totalAddWidth += scrW;
		allScreenInfo.push_back(info);
	}

	XMLDocument doc;
	XMLElement* mapNode = nullptr;
	XMLElement* layerNode = nullptr;
	XMLElement* dataNode = nullptr;
	int baseW = 0;
	std::vector<int> baseAll;

	// ================== 模式 1：单 JSON 转换 ==================
	if (tmxPath.empty())
	{
		std::cout << "[Mode] Single JSON to TMX conversion...\n";
		baseW = 0;

		mapNode = doc.NewElement("map");
		doc.InsertFirstChild(mapNode);
		mapNode->SetAttribute("version", "1.10");
		mapNode->SetAttribute("tiledversion", "1.10.2");
		mapNode->SetAttribute("orientation", "orthogonal");
		mapNode->SetAttribute("renderorder", "right-down");
		mapNode->SetAttribute("width", totalAddWidth);
		mapNode->SetAttribute("height", baseH);
		mapNode->SetAttribute("tilewidth", 32);
		mapNode->SetAttribute("tileheight", 32);
		mapNode->SetAttribute("infinite", 0);
		mapNode->SetAttribute("nextlayerid", 2);
		mapNode->SetAttribute("nextobjectid", 1);

		XMLElement* tilesetNode = doc.NewElement("tileset");
		tilesetNode->SetAttribute("firstgid", 1);
		tilesetNode->SetAttribute("name", "Tiles");
		tilesetNode->SetAttribute("tilewidth", 32);
		tilesetNode->SetAttribute("tileheight", 32);
		tilesetNode->SetAttribute("tilecount", 112);
		tilesetNode->SetAttribute("columns", 16);

		XMLElement* imageNode = doc.NewElement("image");
		imageNode->SetAttribute("source", "CadLevel.png");
		imageNode->SetAttribute("width", 512);
		imageNode->SetAttribute("height", 224);
		tilesetNode->InsertEndChild(imageNode);
		mapNode->InsertEndChild(tilesetNode);

		layerNode = doc.NewElement("layer");
		layerNode->SetAttribute("id", 1);
		layerNode->SetAttribute("name", "Layer1");
		layerNode->SetAttribute("width", totalAddWidth);
		layerNode->SetAttribute("height", baseH);

		dataNode = doc.NewElement("data");
		dataNode->SetAttribute("encoding", "csv");
		layerNode->InsertEndChild(dataNode);
		mapNode->InsertEndChild(layerNode);
	}
	// ================== 模式 2：TMX 合并 ==================
	else
	{
		std::cout << "[Mode] Merging TMX with JSON...\n";
		if (doc.LoadFile(tmxPath.c_str()) != XML_SUCCESS) {
			std::cerr << "Failed to load TMX: " << tmxPath << "\n";
			return -1;
		}
		mapNode = doc.FirstChildElement("map");
		if (!mapNode) {
			std::cerr << "Missing <map> element in TMX.\n";
			return -1;
		}
		mapNode->QueryIntAttribute("width", &baseW);
		mapNode->QueryIntAttribute("height", &baseH);

		layerNode = mapNode->FirstChildElement("layer");
		if (!layerNode) {
			std::cerr << "Missing <layer> element in TMX.\n";
			return -1;
		}
		dataNode = layerNode->FirstChildElement("data");
		if (!dataNode) {
			std::cerr << "Missing <data> element in TMX.\n";
			return -1;
		}

		if (!parseTileData(dataNode, layerNode->Attribute("name"), baseAll)) {
			return -1;
		}

		if (baseAll.size() < static_cast<size_t>(baseW * baseH)) {
			std::cerr << "Warning: Base TMX data size is " << baseAll.size()
				<< ", but expected " << baseW * baseH << ". Padding with 0...\n";
			baseAll.resize(static_cast<size_t>(baseW * baseH), 0);
		}
	}

	// ================== 通用拼接逻辑 ==================
	int newW = baseW + totalAddWidth;
	std::vector<std::vector<int>> outRows(baseH, std::vector<int>(newW, 0));

	// 如果有原底图数据，先铺左边
	if (baseW > 0) {
		for (int y = 0; y < baseH; y++) {
			for (int x = 0; x < baseW; x++) {
				outRows[y][x] = baseAll[static_cast<size_t>(y) * baseW + x];
			}
		}
	}

	// 铺右侧
	int currentX = baseW;
	for (const auto& info : allScreenInfo)
	{
		for (int y = 0; y < baseH; y++) {
			for (int x = 0; x < info.w; x++) {
				if (y < info.h) {
					outRows[y][currentX + x] = info.rows[y][x];
				}
			}
		}
		currentX += info.w;
	}

	// 展平并统一存为 CSV
	std::vector<int> outAll;
	outAll.reserve(static_cast<size_t>(newW) * baseH);
	for (const auto& r : outRows) {
		outAll.insert(outAll.end(), r.begin(), r.end());
	}
	std::string outCsv = toCsv(outAll, newW);

	mapNode->SetAttribute("width", newW);
	mapNode->SetAttribute("height", baseH);
	layerNode->SetAttribute("width", newW);
	layerNode->SetAttribute("height", baseH);

	dataNode->SetAttribute("encoding", "csv");
	dataNode->DeleteAttribute("compression");

	while (dataNode->FirstChild()) {
		dataNode->DeleteChild(dataNode->FirstChild());
	}
	dataNode->SetText(outCsv.c_str());

	// ================== 决定输出文件名 ==================
	std::string outPath;
	if (tmxPath.empty()) {
		size_t dotPos = jsonPath.rfind('.');
		outPath = (dotPos == std::string::npos) ? (jsonPath + "-out.tmx") : (jsonPath.substr(0, dotPos) + "-out.tmx");
	}
	else {
		size_t dotPos = tmxPath.rfind('.');
		outPath = (dotPos == std::string::npos) ? (tmxPath + "-out.tmx") : (tmxPath.substr(0, dotPos) + "-out.tmx");
	}

	if (doc.SaveFile(outPath.c_str(), false) == XML_SUCCESS)
	{
		std::cout << "Saved -> " << outPath << "\n";
		std::cout << "Size: " << newW << " x " << baseH << "\n";
	}
	else
	{
		std::cerr << "Save failed\n";
		return -1;
	}
	return 0;
}