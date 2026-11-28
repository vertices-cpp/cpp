#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <cstdint>
#include <cstring>
#include <cstdlib>

#include <tinyxml2.h>
#include <zlib.h>
#include "base64.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

using namespace tinyxml2;
using namespace orange;


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
// ★ 这里直接把你 main.cpp 里的 parseTileData 拷贝过来
// 为了保证原封不动，我就不在这里重复贴了，直接引用你上面 main.cpp 里那个完美版本。

// 递归收集所有图层用到的 GID
void collectUsedGids(XMLElement* parentNode, std::set<int>& usedGids) {
	for (XMLElement* child = parentNode->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
		std::string name = child->Value();
		if (name == "layer") {
			XMLElement* dataNode = child->FirstChildElement("data");
			std::vector<int> gids;
			if (parseTileData(dataNode, child->Attribute("name"), gids)) {
				for (int gid : gids) {
					int cleanGid = gid & 0x1FFFFFFF; // 清除翻转标志
					if (cleanGid > 0) usedGids.insert(cleanGid);
				}
			}
		}
		else if (name == "group") {
			collectUsedGids(child, usedGids);
		}
	}
}

// 递归更新所有图层的数据
void updateGids(XMLElement* parentNode, const std::map<int, int>& gidMap, int mapWidth) {
	for (XMLElement* child = parentNode->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
		std::string name = child->Value();
		if (name == "layer") {
			XMLElement* dataNode = child->FirstChildElement("data");
			std::vector<int> gids;
			if (parseTileData(dataNode, child->Attribute("name"), gids)) {
				for (int& gid : gids) {
					int cleanGid = gid & 0x1FFFFFFF;
					bool flipH = (gid & 0x80000000) != 0;
					bool flipV = (gid & 0x40000000) != 0;
					bool flipD = (gid & 0x20000000) != 0;

					int newGid = 0;
					if (cleanGid > 0 && gidMap.find(cleanGid) != gidMap.end()) {
						newGid = gidMap.at(cleanGid);
					}

					if (newGid > 0) {
						if (flipH) newGid |= 0x80000000;
						if (flipV) newGid |= 0x40000000;
						if (flipD) newGid |= 0x20000000;
					}
					gid = newGid;
				}

				// 统一写回 CSV，便于阅读（你也可以写回 Base64，但 CSV 最稳）
				std::stringstream ss;
				for (size_t i = 0; i < gids.size(); ++i) {
					ss << gids[i];
					if (i != gids.size() - 1) ss << ",";
					if (mapWidth > 0 && (i + 1) % mapWidth == 0 && i != gids.size() - 1) ss << "\n";
				}
				dataNode->SetAttribute("encoding", "csv");
				dataNode->DeleteAttribute("compression");
				while (dataNode->FirstChild()) dataNode->DeleteChild(dataNode->FirstChild());
				dataNode->SetText(ss.str().c_str());
			}
		}
		else if (name == "group") {
			updateGids(child, gidMap, mapWidth);
		}
	}
}

// ★ 封装单文件处理逻辑
bool processTmx(const std::string& tmxPath) {
	XMLDocument doc;
	if (doc.LoadFile(tmxPath.c_str()) != XML_SUCCESS) {
		std::cerr << "[Error] Failed to load TMX: " << tmxPath << "\n";
		return false;
	}
	XMLElement* mapNode = doc.FirstChildElement("map");
	if (!mapNode) return false;

	int mapW = mapNode->IntAttribute("width");
	int mapH = mapNode->IntAttribute("height");

	XMLElement* tilesetNode = mapNode->FirstChildElement("tileset");
	if (!tilesetNode) return false;

	int firstGid = tilesetNode->IntAttribute("firstgid", 1);
	int tileW = tilesetNode->IntAttribute("tilewidth", 32);
	int tileH = tilesetNode->IntAttribute("tileheight", 32);
	int columns = tilesetNode->IntAttribute("columns", 16);
	int margin = tilesetNode->IntAttribute("margin", 0);
	int spacing = tilesetNode->IntAttribute("spacing", 0);

	XMLElement* imageNode = tilesetNode->FirstChildElement("image");
	if (!imageNode) return false;
	std::string imgSource = imageNode->Attribute("source");

	// 路径处理
	std::string baseDir = ".";
	size_t slashPos = tmxPath.find_last_of("/\\");
	if (slashPos != std::string::npos) {
		baseDir = tmxPath.substr(0, slashPos);
	}
	std::string fullImgPath = imgSource;
	if (imgSource.find(":") == std::string::npos && imgSource[0] != '/' && imgSource[0] != '\\') {
		fullImgPath = baseDir + "/" + imgSource;
	}
	std::replace(fullImgPath.begin(), fullImgPath.end(), '\\', '/');

	std::set<int> usedGids;
	collectUsedGids(mapNode, usedGids);

	if (usedGids.empty()) {
		std::cout << "[" << tmxPath << "] No tiles used, skipping.\n";
		return false;
	}

	int imgW, imgH, imgChannels;
	unsigned char* srcPixels = stbi_load(fullImgPath.c_str(), &imgW, &imgH, &imgChannels, 4);
	if (!srcPixels) {
		std::cerr << "[" << tmxPath << "] Failed to load image: " << fullImgPath << "\n";
		return false;
	}

	int numTiles = usedGids.size();
	int newColumns = 16;
	int newRows = (numTiles + newColumns - 1) / newColumns;
	int newImgW = newColumns * tileW;
	int newImgH = newRows * tileH;

	std::vector<unsigned char> dstPixels(newImgW * newImgH * 4, 0);
	std::map<int, int> gidMap;
	int newIndex = 0;

	for (int oldGid : usedGids) {
		int localId = oldGid - firstGid;
		int srcCol = localId % columns;
		int srcRow = localId / columns;
		int srcX = margin + srcCol * (tileW + spacing);
		int srcY = margin + srcRow * (tileH + spacing);

		int dstCol = newIndex % newColumns;
		int dstRow = newIndex / newColumns;
		int dstX = dstCol * tileW;
		int dstY = dstRow * tileH;

		for (int y = 0; y < tileH; ++y) {
			for (int x = 0; x < tileW; ++x) {
				int sx = srcX + x;
				int sy = srcY + y;
				if (sx >= 0 && sx < imgW && sy >= 0 && sy < imgH) {
					int srcIdx = (sy * imgW + sx) * 4;
					int dstIdx = ((dstY + y) * newImgW + (dstX + x)) * 4;
					for (int c = 0; c < 4; ++c) dstPixels[dstIdx + c] = srcPixels[srcIdx + c];
				}
			}
		}
		gidMap[oldGid] = newIndex + 1;
		newIndex++;
	}
	stbi_image_free(srcPixels);

	size_t dotPos = tmxPath.rfind('.');
	std::string basePath = (dotPos == std::string::npos) ? tmxPath : tmxPath.substr(0, dotPos);
	std::string fileName = (slashPos == std::string::npos) ? basePath : basePath.substr(slashPos + 1);

	std::string newImgFileName = fileName + "_trimmed.png";
	std::string newImgFullPath = baseDir + "/" + newImgFileName;
	std::replace(newImgFullPath.begin(), newImgFullPath.end(), '\\', '/');

	stbi_write_png(newImgFullPath.c_str(), newImgW, newImgH, 4, dstPixels.data(), newImgW * 4);

	tilesetNode->SetAttribute("firstgid", 1);
	tilesetNode->SetAttribute("tilecount", numTiles);
	tilesetNode->SetAttribute("columns", newColumns);
	imageNode->SetAttribute("source", newImgFileName.c_str());
	imageNode->SetAttribute("width", newImgW);
	imageNode->SetAttribute("height", newImgH);

	updateGids(mapNode, gidMap, mapW);

	std::string outTmxName = baseDir + "/" + fileName + "_trimmed.tmx";
	std::replace(outTmxName.begin(), outTmxName.end(), '\\', '/');

	if (doc.SaveFile(outTmxName.c_str()) == XML_SUCCESS) {
		std::cout << "[Success] " << outTmxName << "\n";
		return true;
	}
	return false;
}

// ★ 终极循环入口
int main(int argc, char** argv) {
	if (argc < 2) {
		std::cout << "Usage: TmxTrim.exe <map1.tmx> [map2.tmx ...]\n";
		std::cout << "Or drag and drop multiple .tmx files onto this exe.\n";
		return -1;
	}

	int successCount = 0;
	for (int i = 1; i < argc; ++i) {
		std::string tmxPath = argv[i];

		// 去掉 Windows 拖拽自动加的双引号
		if (!tmxPath.empty() && tmxPath.front() == '"' && tmxPath.back() == '"') {
			tmxPath = tmxPath.substr(1, tmxPath.size() - 2);
		}

		// 防呆：如果后缀不是 tmx（比如批量拖拽不小心带了文件夹或别的文件），跳过
		if (tmxPath.find(".tmx") == std::string::npos && tmxPath.find(".TMX") == std::string::npos) {
			continue;
		}

		std::cout << "Processing: " << tmxPath << "\n";
		if (processTmx(tmxPath)) {
			successCount++;
		}
	}

	std::cout << "--------------------------------\n";
	std::cout << "All done! Success: " << successCount << " / " << (argc - 1) << "\n";

	// 如果你希望处理完自动关闭窗口，就不加 pause
	return 0;
}