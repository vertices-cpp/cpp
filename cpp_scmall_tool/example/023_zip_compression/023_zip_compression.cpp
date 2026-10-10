#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "ZipHelper.h"
#include "path_head.h"


USING_OG;



static std::string readLine(const std::string& prompt, const std::string& def = "")
{
	std::cout << prompt;
	if (!def.empty())
		std::cout << "[" << def << "] ";
	std::string s;
	std::getline(std::cin, s);
	while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
		s.pop_back();
	if (s.empty() && !def.empty())
		return def;
	return s;
}

template<typename T,typename Ty>
T MIN_P(T a, Ty b)
{
	return a < b ? a : b;
}
static std::string readXorKeyStr(const std::string& prompt)
{
	return readLine(prompt);   // 返回整个字符串
}
int main()
{
	// 默认路径
	std::string prePath = og::checkPath("");
	std::cout << "Default path: " << prePath << "\n";

	while (true)
	{
		std::cout << "\n==============================\n";
		std::cout << "  1. Compress\n";
		std::cout << "  2. Decompress\n";
		std::cout << "  0. Exit\n";
		std::cout << "==============================\n";

		std::string c = readLine("Choice: ");
		if (c == "0") break;

		if (c == "1")
		{
			std::string src = readLine("Source dir: ") ;
 
			if (src.empty())
			{
				std::cout << "[ERROR] source dir is empty\n";
				continue;
			}
			src = og::checkPath(src);

			std::string dst  = readLine("Output zip: ");
			if (dst.empty())
			{
				std::cout << "[ERROR] output zip is empty\n";
				continue;
			}
			dst = og::checkPath(dst);


			std::string pwd = readLine("Password (Enter = none): ");
			std::string key = readXorKeyStr("Xor key (Enter = none): ");


			std::vector<std::string> srcs = { src };
			if (compress(srcs, dst, pwd, key))   // ← zip 命名空间去掉后，直接 compress
				std::cout << "[OK] compress done\n";
			else
				std::cout << "[ERROR] compress failed\n";
		}
		else if (c == "2")
		{
			std::string zfInput = readLine("Zip file: ");
			if (zfInput.empty())
			{
				std::cout << "[ERROR] zip file is empty\n";
				continue;
			}
			std::string zf = og::checkPath(zfInput);

			std::string outInput = readLine("Output dir: ");
			if (outInput.empty())
			{
				std::cout << "[ERROR] output dir is empty\n";
				continue;
			}
			std::string out = og::checkPath(outInput);

			std::string pwd = readLine("Password (Enter = none): ");
			std::string key = readXorKeyStr("Xor key (Enter = none): ");

			if (decompress(zf, out, pwd, key))
				std::cout << "[OK] decompress done\n";
			else
				std::cout << "[ERROR] decompress failed\n";
		}
	}
	return 0;
}