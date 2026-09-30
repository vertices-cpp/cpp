#include "ZipTableManager.h"
#include "OGData.h"
#include "path_head.h"

#include <cstdio>

using namespace std;
using namespace orange;

int main()
{
	auto ztb = ZipTableManager::getInstance();

	std::string curPath = og::checkPath("222.zip");
	printf("curPath = [%s]\n", curPath.c_str());
	FILE* fp = fopen(curPath.c_str(), "rb");
	if (!fp) {
		printf("❌ fopen 失败, errno=%d (%s)\n", errno, strerror(errno));
		return 1;
	}
	printf("✅ fopen 成功\n");
	fclose(fp);
	// 1. 加载
	if (!ztb->addZip(curPath, "","123"/*,"456"*/)) {
		printf("add base.zip failed\n");
		return 1;
	}
	 

	Data data;
	// 3. 读
	if (ztb->has(curPath) && ztb->read(curPath, data))
	{

	}
	else
		cout << "失败" << endl;
	 

	return 0;
}