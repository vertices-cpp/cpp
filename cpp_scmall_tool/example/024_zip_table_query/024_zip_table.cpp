#include "ZipTableManager.h"
#include "OGData.h"
#include "path_head.h"

#include <cstdio>

using namespace std;
using namespace orange;

// ================= 单文件解压（支持密码） =================
// zipFile   : zip 路径
// entryName : zip 里的 entry 名，如 "111/022_zip_compression.cpp"
// password  : 密码，空 = 不加密
// outData   : 输出解压后的数据
// 返回 true 表示解压成功
static bool unzipOne(const std::string& zipFile,
	const std::string& entryName,
	const std::string& password,
	Data& outData)
{
	outData.clear();

	unzFile fp = unzOpen(zipFile.c_str());
	if (!fp) {
		printf("Failed to open zip: %s\n", zipFile.c_str());
		return false;
	}

	int err = unzGoToFirstFile(fp);
	if (err != UNZ_OK) {
		printf("Failed to local first file");
		unzClose(fp);
		return false;
	}
	// 按名定位
	if (unzLocateFile(fp, entryName.c_str(), nullptr) != UNZ_OK) {
		printf("Failed to locate entry: %s\n", entryName.c_str());
		unzClose(fp);
		return false;
	}

	// 带/不带密码打开
	  err = password.empty()
		? unzOpenCurrentFile(fp)
		: unzOpenCurrentFilePassword(fp, password.c_str());
	if (err != UNZ_OK) {
		printf("Failed to open entry (err=%d), password wrong?\n", err);
		unzClose(fp);
		return false;
	}

	// 拿解压后大小
	unz_file_info fi = { 0 };
	if (unzGetCurrentFileInfo(fp, &fi, nullptr, 0, nullptr, 0, nullptr, 0) != UNZ_OK) {
		unzCloseCurrentFile(fp);
		unzClose(fp);
		return false;
	}

	// 分块读
	std::vector<unsigned char> buf;
	buf.reserve((size_t)fi.uncompressed_size);

	const int CHUNK = 64 * 1024;
	std::vector<char> chunk(CHUNK);
	bool hasError = false;

	for (;;) {
		int n = unzReadCurrentFile(fp, chunk.data(), CHUNK);
		if (n < 0) { hasError = true; break; }
		if (n == 0) break;
		buf.insert(buf.end(), chunk.data(), chunk.data() + n);
	}

	int closeRet = unzCloseCurrentFile(fp);   // 会做 CRC 校验
	unzClose(fp);

	if (hasError || closeRet != UNZ_OK) {
		printf("unzip error: read=%d close=%d\n", (int)hasError, closeRet);
		return false;
	}
	if (buf.size() != (size_t)fi.uncompressed_size) {
		printf("unzip error: size mismatch\n");
		return false;
	}

	outData.fastSet((unsigned char*)malloc(buf.size()), buf.size());
	memcpy(outData.getBytes(), buf.data(), buf.size());

	return true;
}

int main()
{
	auto ztb = ZipTableManager::getInstance();

	std::string curPath = og::checkPath("1.zip");
	printf("curPath = [%s]\n", curPath.c_str());
	FILE* fp = fopen(curPath.c_str(), "rb");
	if (!fp) {
		printf("❌ fopen 失败, errno=%d (%s)\n", errno, strerror(errno));
		return 1;
	}
	printf("✅ fopen 成功\n");
	fclose(fp);
		if (!ztb->addZip(curPath)) {
		printf("add base.zip failed\n");
		return 1;
	}

	// 1. 加载
// 	if (!ztb->addZip(curPath,  "123","456")) {
// 		printf("add base.zip failed\n");
// 		return 1;
// 	}
// 	 
// 
  	Data data;
// 	// 3. 读
	const char* k = "111/022_zip_compression.cpp";

	if (!ztb->has(k)) {
		cout << "has failed: " << k << endl;
	}
	else {
		Data data;
		if (ztb->read(k, data)) {
			cout << "read ok, size=" << data.getSize() << endl;
		}
		else {
			cout << "read failed: " << k << endl;   // 密码/异或/CRC 问题
		}
	}

	Data d;
	if (unzipOne(og::checkPath("1.zip"),
		"111/022_zip_compression.cpp",
		"123",     // 密码，空 = 不加密
		d)) {
		printf("ok size=%zu\n", d.getSize());
	}
	else {
		printf("failed\n");
	}

	return 0;
}