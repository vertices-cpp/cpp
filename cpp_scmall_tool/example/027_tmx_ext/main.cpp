#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cmath>

// ★ 引入 zlib 和 你的 base64 库
#include <zlib.h>
#include "base64.h" // 根据你的实际路径调整

#include <tinyxml2.h>
 

using namespace tinyxml2;
using namespace orange; // 方便直接调用 base64Decode

// 解析 CSV 数据（无视空格和换行）
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

// 将 GID 数组序列化为小端字节流
static std::vector<unsigned char> gidsToLittleEndianBytes(const std::vector<int>& gids) {
	std::vector<unsigned char> bytes;
	bytes.reserve(gids.size() * 4);
	for (int val : gids) {
		uint32_t gid = static_cast<uint32_t>(val);
		bytes.push_back(static_cast<unsigned char>(gid & 0xFF));
		bytes.push_back(static_cast<unsigned char>((gid >> 8) & 0xFF));
		bytes.push_back(static_cast<unsigned char>((gid >> 16) & 0xFF));
		bytes.push_back(static_cast<unsigned char>((gid >> 24) & 0xFF));
	}
	return bytes;
}

static bool compressZlib(const std::vector<unsigned char>& src, std::vector<unsigned char>& out) {
	uLongf bound = compressBound(static_cast<uLong>(src.size()));
	out.resize(bound);
	int res = compress2(out.data(), &bound, src.data(), static_cast<uLong>(src.size()), Z_DEFAULT_COMPRESSION);
	if (res != Z_OK) {
		return false;
	}
	out.resize(bound);
	return true;
}
// 辅助函数：解压 gzip 字节流
static bool decompressGzip(const unsigned char* src, size_t srcLen, std::vector<unsigned char>& out) {
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
static bool compressGzip(const std::vector<unsigned char>& src, std::vector<unsigned char>& out) {
	z_stream strm{};
	if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
		return false;
	}

	uLong bound = deflateBound(&strm, static_cast<uLong>(src.size()));
	out.resize(bound);

	strm.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(src.data()));
	strm.avail_in = static_cast<uInt>(src.size());
	strm.next_out = reinterpret_cast<Bytef*>(out.data());
	strm.avail_out = static_cast<uInt>(out.size());

	int ret = deflate(&strm, Z_FINISH);
	if (ret != Z_STREAM_END) {
		deflateEnd(&strm);
		return false;
	}

	out.resize(strm.total_out);
	deflateEnd(&strm);
	return true;
}

// 处理单个图块层的数据
void processLayer(XMLElement* layerNode, int oldW, int oldH, int newW, int newH) {
	if (!layerNode) return;

	XMLElement* dataNode = layerNode->FirstChildElement("data");
	if (!dataNode) return;

	const char* layerName = layerNode->Attribute("name");
	const char* safeName = layerName ? layerName : "Unnamed";

	const char* encoding = dataNode->Attribute("encoding");
	const char* compression = dataNode->Attribute("compression");
	std::string encStr = encoding ? encoding : "";
	std::string compStr = compression ? compression : "";

	std::vector<int> oldData;

	// ★ 核心：解析各种格式
	if (encStr == "csv") {
		std::string csvText = dataNode->GetText() ? dataNode->GetText() : "";
		oldData = splitCsv(csvText);
	}
	else if (encStr == "base64") {
		std::string b64Text = dataNode->GetText() ? dataNode->GetText() : "";

		// 去除换行符（有些平台生成的 base64 会带换行符，导致解码失败）
		b64Text.erase(std::remove(b64Text.begin(), b64Text.end(), '\n'), b64Text.end());
		b64Text.erase(std::remove(b64Text.begin(), b64Text.end(), '\r'), b64Text.end());

		unsigned char* binData = nullptr;
		int binSize = base64Decode((const unsigned char*)b64Text.c_str(), (unsigned int)b64Text.length(), &binData);

		if (binSize <= 0 || binData == nullptr) {
			std::cerr << "    [Error] Base64 decode failed or empty for layer '" << safeName << "'!\n";
			if (binData) free(binData);
			return;
		}

		std::cout << "    [Debug] Base64 decoded size: " << binSize << " bytes.\n";

		if (compStr == "zlib") {
			size_t uncompressedSize = binSize * 4 + 1024;
			std::vector<unsigned char> unzipped(uncompressedSize);
			uLongf destLen = unzipped.size();

			int z_res = uncompress(unzipped.data(), &destLen, binData, binSize);
			while (z_res == Z_BUF_ERROR && uncompressedSize < 1024 * 1024 * 50) {
				uncompressedSize *= 2;
				unzipped.resize(uncompressedSize);
				destLen = unzipped.size();
				z_res = uncompress(unzipped.data(), &destLen, binData, binSize);
			}

			if (z_res != Z_OK) {
				std::cerr << "    [Error] Zlib uncompress failed! Code: " << z_res << "\n";
				free(binData);
				return;
			}

			unzipped.resize(destLen);
			size_t numTiles = unzipped.size() / 4;
			std::cout << "    [Debug] Zlib uncompress success, tiles: " << numTiles << "\n";
			oldData.reserve(numTiles);
			for (size_t i = 0; i < numTiles; ++i) {
				uint32_t gid = unzipped[i * 4]
					| (unzipped[i * 4 + 1] << 8)
					| (unzipped[i * 4 + 2] << 16)
					| (unzipped[i * 4 + 3] << 24);
				oldData.push_back((int)gid);
			}
		}
		else if (compStr == "gzip") {
			std::vector<unsigned char> unzipped;
			if (!decompressGzip(binData, (size_t)binSize, unzipped)) {
				std::cerr << "    [Error] Gzip decompress failed!\n";
				free(binData);
				return;
			}

			size_t numTiles = unzipped.size() / 4;
			std::cout << "    [Debug] Gzip decompress success, tiles: " << numTiles << "\n";
			oldData.reserve(numTiles);
			for (size_t i = 0; i < numTiles; ++i) {
				uint32_t gid = unzipped[i * 4]
					| (unzipped[i * 4 + 1] << 8)
					| (unzipped[i * 4 + 2] << 16)
					| (unzipped[i * 4 + 3] << 24);
				oldData.push_back((int)gid);
			}
		}
		else if (compStr.empty()) {
			size_t numTiles = binSize / 4;
			std::cout << "    [Debug] Raw Base64 tiles: " << numTiles << "\n";
			oldData.reserve(numTiles);
			for (size_t i = 0; i < numTiles; ++i) {
				uint32_t gid = binData[i * 4]
					| (binData[i * 4 + 1] << 8)
					| (binData[i * 4 + 2] << 16)
					| (binData[i * 4 + 3] << 24);
				oldData.push_back((int)gid);
			}
		}
		else {
			std::cout << "  [Skip] Layer '" << safeName << "' uses unsupported compression: " << compStr << "\n";
			free(binData);
			return;
		}

		free(binData);
	}
	else {
		std::cout << "  [Skip] Layer '" << safeName << "' has unsupported encoding: " << encStr << "\n";
		return;
	}

	// ★ 容错：在补0之前先打印一下，看是不是真读到了
	if (!oldData.empty()) {
		std::cout << "    [Debug] First 10 tiles before resize: ";
		for (int i = 0; i < std::min((int)oldData.size(), 10); ++i) {
			std::cout << oldData[i] << " ";
		}
		std::cout << "...\n";
	}

	if (oldData.size() < (size_t)(oldW * oldH)) {
		std::cerr << "  [Warning] Data size mismatch for layer '" << safeName << "', padding with 0.\n";
		oldData.resize((size_t)(oldW * oldH), 0);
	}

	// 构建新数据
	std::vector<int> outData;
	outData.reserve((size_t)newW * newH);
	for (int y = 0; y < newH; y++) {
		for (int x = 0; x < newW; x++) {
			if (x < oldW && y < oldH) {
				outData.push_back(oldData[(size_t)y * oldW + x]);
			}
			else {
				outData.push_back(0);
			}
		}
	}

	layerNode->SetAttribute("width", newW);
	layerNode->SetAttribute("height", newH);
	while (dataNode->FirstChild()) dataNode->DeleteChild(dataNode->FirstChild());

	// ★ 按原编码与压缩格式写回
	if (encStr == "csv") {
		std::stringstream outSS;
		for (size_t i = 0; i < outData.size(); ++i) {
			outSS << outData[i];
			if (i != outData.size() - 1) {
				outSS << ",";
			}
			if ((i + 1) % newW == 0 && i != outData.size() - 1) {
				outSS << "\n";
			}
		}
		dataNode->SetAttribute("encoding", "csv");
		dataNode->DeleteAttribute("compression");
		dataNode->SetText(outSS.str().c_str());
		std::cout << "  [OK] Layer '" << safeName << "' processed & saved as CSV.\n";
	}
	else if (encStr == "base64") {
		std::vector<unsigned char> rawBytes = gidsToLittleEndianBytes(outData);
		std::vector<unsigned char> payloadBytes;

		if (compStr.empty()) {
			payloadBytes = std::move(rawBytes);
		}
		else if (compStr == "zlib") {
			if (!compressZlib(rawBytes, payloadBytes)) {
				std::cerr << "  [Error] Zlib compress failed for layer '" << safeName << "'.\n";
				return;
			}
		}
		else if (compStr == "gzip") {
			if (!compressGzip(rawBytes, payloadBytes)) {
				std::cerr << "  [Error] Gzip compress failed for layer '" << safeName << "'.\n";
				return;
			}
		}
		else {
			std::cout << "  [Skip] Layer '" << safeName << "' cannot re-encode compression '" << compStr << "'.\n";
			return;
		}

		char* b64Out = nullptr;
		int b64Len = base64Encode(payloadBytes.data(), static_cast<unsigned int>(payloadBytes.size()), &b64Out);
		if (b64Len <= 0 || !b64Out) {
			std::cerr << "  [Error] Base64 encode failed for layer '" << safeName << "'.\n";
			if (b64Out) free(b64Out);
			return;
		}

		dataNode->SetAttribute("encoding", "base64");
		if (compStr.empty()) {
			dataNode->DeleteAttribute("compression");
		}
		else {
			dataNode->SetAttribute("compression", compStr.c_str());
		}
		dataNode->SetText(b64Out);
		free(b64Out);

		std::cout << "  [OK] Layer '" << safeName << "' processed & saved as base64"
			<< (compStr.empty() ? "" : (" (" + compStr + ")")) << ".\n";
	}
}

// 递归遍历整棵树寻找 <layer>
void traverseAndProcess(XMLElement* parentNode, int oldW, int oldH, int newW, int newH) {
	for (XMLElement* child = parentNode->FirstChildElement(); child != nullptr; child = child->NextSiblingElement()) {
		std::string nodeName = child->Value();
		if (nodeName == "layer") {
			processLayer(child, oldW, oldH, newW, newH);
		}
		else if (nodeName == "group") {
			traverseAndProcess(child, oldW, oldH, newW, newH);
		}
	}
}

int main(int argc, char** argv) {
	std::string tmxPath;

	if (argc < 2) {
		std::cout << "Please drag and drop a .tmx file here: ";
		std::getline(std::cin, tmxPath);
		if (!tmxPath.empty() && tmxPath.front() == '"' && tmxPath.back() == '"') {
			tmxPath = tmxPath.substr(1, tmxPath.size() - 2);
		}
	}
	else {
		tmxPath = argv[1];
	}

	if (tmxPath.empty()) {
		std::cerr << "No file provided. Exiting.\n";
		return -1;
	}

	XMLDocument doc;
	if (doc.LoadFile(tmxPath.c_str()) != XML_SUCCESS) {
		std::cerr << "Failed to load file: " << tmxPath << "\n";
		return -1;
	}

	XMLElement* mapNode = doc.FirstChildElement("map");
	if (!mapNode) return -1;

	int oldW = 0, oldH = 0;
	mapNode->QueryIntAttribute("width", &oldW);
	mapNode->QueryIntAttribute("height", &oldH);

	int inputW = 0, inputH = 0;
	std::cout << "Target file: " << tmxPath << "\n";
	std::cout << "Original Size: " << oldW << " x " << oldH << "\n";

	std::cout << "Enter new Width (0 or <0 to keep original): ";
	std::cin >> inputW;
	std::cout << "Enter new Height (0 or <0 to keep original): ";
	std::cin >> inputH;

	int newW = (inputW <= 0) ? oldW : inputW;
	int newH = (inputH <= 0) ? oldH : inputH;

	if (newW <= 0 || newH <= 0) {
		std::cerr << "Invalid width or height. Exiting.\n";
		return -1;
	}

	mapNode->SetAttribute("width", newW);
	mapNode->SetAttribute("height", newH);

	std::cout << "Processing layers...\n";
	traverseAndProcess(mapNode, oldW, oldH, newW, newH);

	size_t dotPos = tmxPath.rfind('.');
	std::string outPath = (dotPos == std::string::npos) ? (tmxPath + "-out.tmx") : (tmxPath.substr(0, dotPos) + "-out.tmx");

	if (doc.SaveFile(outPath.c_str(), false) == XML_SUCCESS) {
		std::cout << "\n[SUCCESS] Saved to: " << outPath << "\n";
		std::cout << "Result Size: " << newW << " x " << newH << "\n";
	}
	else {
		std::cerr << "\n[ERROR] Save failed!\n";
	}

	return 0;
}