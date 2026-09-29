#include <iostream> 
#include <vector>
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

#include "path_head.h" 

void display(unsigned char *font_buf)
{
	// 打印起始地址
//	printf("ADDR - %lX\n", addr);
	// 定位到字体数据的起始位置
	unsigned char *font = font_buf;
	// 遍历 11 行
	for (int y = 0; y < 11; y++) {
		// 遍历 11 列
		for (int x = 0; x < 11; x++) {
			// 计算当前点在字体数据中的位索引
			int bit_idx = x + y * 11;
			// 计算当前点所在的字节索引
			int fidx = bit_idx >> 3;
			// 计算当前点在字节内的偏移量
			int fofs = bit_idx & 0x7;

			// 提取当前点的位值
			int bit = (font[fidx] >> (fofs)) & 0x1;
			// 根据位值选择输出字符
			char outchar = bit > 0 ? '#' : ' ';
			// 打印字符
			printf("%c", outchar);
		}
		// 换行
		printf("\n");
	}
}



// 反向排列函数，将经过 chr_to_rom 处理后的数据还原
// src 是指向存储转换后数据的数组的指针，dst 是指向存储还原后数据的数组的指针
// void rom_to_chr(uint8_t *src, unsigned char *dst)
// {
// 	// 将 dst 数组初始化为 0
// 	memset(dst, 0, 32);
// 
// 	// 遍历 11 行
// 	for (int y = 0; y < 11; y++) {
// 		// 遍历 11 列
// 		for (int x = 0; x < 11; x++) {
// 			// 计算目标数据中当前位的全局索引
// 			int d_bit_idx = x + y * 11;
// 			// 计算目标数据中当前位所在的字节索引
// 			int d_fidx = d_bit_idx >> 3;
// 			// 计算目标数据中当前位在字节内的偏移量
// 			int d_file_offset = d_bit_idx & 0x7;
// 
// 			// 提取目标数据中的当前位
// 			int bit = (src[d_fidx] >> d_file_offset) & 0x1;
// 
// 			// 计算源数据中当前位所在的字节索引
// 			int s_fidx = y * 2 + (x / 8);
// 			// 计算源数据中当前位在字节内的偏移量
// 			int s_fofs = x & 0x7;
// 
// 			// 计算 dst 中的索引
// 			int dst_index = (s_fidx / 2) + 5;
// 			// 如果是奇数位置的字节，偏移量加 0x10
// 			if (s_fidx % 2 == 1) {
// 				dst_index += 0x10;
// 			}
// 
// 			// 将提取的位写入 dst 数组的相应位置
// 			dst[dst_index] = dst[dst_index] | (bit << (7 - s_fofs));
// 		}
// 	}
// }


// 将 chr 格式的字体数据转换为特定 ROM 格式的函数
// dst 是指向存储转换后数据的数组的指针
void chr_to_rom(uint8_t*src, uint8_t * dst)
{
	// 动态分配一个 22 字节的数组用于临时存储处理后的数据
	unsigned char* font = new unsigned char[22];

	int j = 5;
	// 从 src 中提取数据到 font 数组
	for (int i = 0; i < 11; ++i, j++)
	{
		font[i * 2] = src[j];
		font[i * 2 + 1] = src[j + 0x10];
	}

	// 遍历 11 行
	for (int y = 0; y < 11; y++) {
		// 遍历 11 列
		for (int x = 0; x < 11; x++) {
			// 计算源数据中当前位所在的字节索引
			int s_file_idx = y * 2 + (x / 8);
			// 计算源数据中当前位在字节内的偏移量
			int s_file_offset = x & 0x7;

			// 提取源数据中的当前位
			int bit = (font[s_file_idx] >> (7 - s_file_offset)) & 0x1;

			// 计算目标数据中当前位的全局索引
			int d_bit_idx = x + y * 11;
			// 计算目标数据中当前位所在的字节索引
			int d_file_idx = d_bit_idx >> 3;
			// 计算目标数据中当前位在字节内的偏移量
			int d_file_offset = d_bit_idx & 0x7;
			// 将提取的位写入目标数据的相应位置
			dst[d_file_idx] = dst[d_file_idx] | (bit << d_file_offset);
		}
	}
	// 释放动态分配的内存
	delete[] font;
}

void rom_to_chr(uint8_t *src, uint8_t *dst)
{
	// 动态分配临时数组（与原始数据格式匹配）
	unsigned char* temp_font = new unsigned char[22];
	memset(temp_font, 0, 22); // 初始化为0

	// 遍历所有像素，提取位并存储到临时数组
	for (int y = 0; y < 11; y++) {
		for (int x = 0; x < 11; x++) {
			// 计算目标位索引和字节信息
			int d_bit_idx = x + y * 11;
			int d_file_idx = d_bit_idx >> 3;
			int d_file_offset = d_bit_idx & 0x7;

			// 从 src 中提取当前位
			int bit = (src[d_file_idx] >> d_file_offset) & 0x1;

			// 计算临时数组中的存储位置
			int s_file_idx = y * 2 + (x / 8);
			int s_file_offset = x & 0x7;

			// 将位写入临时数组（注意位方向反转）
			temp_font[s_file_idx] |= (bit << (7 - s_file_offset));
		}
	}

	// 将临时数组中的数据写入 dst（与原始格式匹配）
	int j = 5;
	for (int i = 0; i < 11; ++i, j++) {
		dst[j] = temp_font[i * 2];
		dst[j + 0x10] = temp_font[i * 2 + 1];
	}

	delete[] temp_font; // 释放内存
}

int ch2int(char c)
{
	
	if (c >= 'A'&&c <= 'F')
		c +=  0x20;
 
	int ret = 0;
	if (c >= '0'&&c <= '9')
	{
		  ret = c - '0';
	}
	else  if (c >= 'a'&&c <= 'f')
	{
		  ret =  c - 'a' + 10;
	}
	return ret;
}

struct AddrInfo {
	int addr;
	int num;
};
const int start_Addr = 0x90910;
const int chr_font_size = 32;
const int rom_font_size = 16;

void unPack(std::string &name, std::string &outFile, vector<AddrInfo> &addr) {
	fstream fp(name.c_str(), ios::in | ios::binary);
	 
	if (!fp)
	{
		return;
	}


	stringstream is;
	is << fp.rdbuf();
	fp.close();

	string binary(is.str());
	string out;
	int chr_total = 0;
	for (auto &i:addr)
	{
		if (i.addr>= start_Addr)
		{
			for (int num = 0; num <= i.num; ++num)
			{
				uint8_t *src = (uint8_t*)(binary.c_str() + i.addr + num * 16);
				uint8_t * dst = new uint8_t[chr_font_size];
				
				memset(dst, 0, chr_font_size);
				rom_to_chr(src, dst);
  				out.insert(chr_total, (char*)dst, chr_font_size);
				
				delete[chr_font_size]dst;

				chr_total += chr_font_size;
			}
			
		}
	} 
	 

	fp.open(outFile, ios::out | ios::binary);
	is.str("");
	is.str(out);
	fp << is.rdbuf();
	fp.close();
}

void Pack(std::string &chr_font_file, std::string &outFile, vector<AddrInfo> &addr)
{
	fstream fp(chr_font_file.c_str(), ios::in | ios::binary);

	if (!fp)
	{
		return;
	}
	stringstream is;
	is << fp.rdbuf();
	fp.close(); 

	//备份---------------
	std::string backupFile = outFile + ".bak";
	fstream dst(backupFile, ios::out | ios::binary);
	dst << is.str();
	dst.close();
	//---------------
	string chr_binary(is.str());
	 
	fp.open(outFile, ios::in|ios::out | ios::binary);
	if (!fp)
	{
		return;
	}

	int chr_total = 0;

	for (auto &i : addr)
	{
		if (i.addr >= start_Addr)
		{
			for (int num = 0; num <= i.num; ++num)
			{
				uint8_t * src = (uint8_t*)(chr_binary.c_str()+ chr_total);
				uint8_t * dst = new uint8_t[rom_font_size];
				memset(dst, 0, rom_font_size);
				chr_to_rom(src, dst);

				fp.seekp(i.addr + num * rom_font_size, ios::beg);
// 				int pos = fp.tellp();
// 				cout <<hex<< pos << endl;
				fp.write((char*)dst, rom_font_size);
				

				delete[rom_font_size]dst;

				chr_total += chr_font_size;
			}
		}
	}
	fp.close();

}

#include <windows.h>

int main(int argc, char **argv)
{

	// 	if (argc <2)
	// 	{
	// 		cout << "如果拉进来了NES文件将生成提取的字库chr.bin,字库为11x11\n"
	// 			"如果拉进来的是chr.bin,将导入ROM中\n"
	// 			"目录下需要的文件名为\n"
	// 			"吞食天地同能版字库.txt   需要读取它的文件偏移位置\n"
	// 			"吞2同能汉化版.nes    \n"
	// 			"yychr文件格式改<1BPP>,排列方式改<FC/NES x16排列显示>即可打开chr.bin\n"
	// 			"本程序由c_cpp123编写\n";
	// 		MessageBox(nullptr,  "请把NES或CHR文件拉进来", "提示", 0);
	// 		return 0;
	// 	}

		//读取完进行操作
	fstream in(og::checkPath("吞食天地同能版字库.txt"), ios::in);

	if (!in)
	{
		cout << "exe目录缺少文件<吞食天地同能版字库.txt>" << endl;
		return 1;
	}
	string text;
	vector<AddrInfo> chr_addr;
	while (getline(in, text))
	{
		stringstream is("");
		is.str(text);
		text.clear();

		while (is >> text)
		{
			int write_flag = 0;
			int pos = 0, rom_addr = 0, num = 0;
			while ((text[pos] >= '0'&&text[pos] <= '9') || (text[pos] >= 'a'&&text[pos] <= 'f')
				|| (text[pos] >= 'A'&&text[pos] <= 'F'))
			{
				rom_addr = (rom_addr << 4) | ch2int(text[pos]);
				pos++;
				write_flag = 1;
			}
			if (text[pos] == ',' &&write_flag)
			{
				pos++;
				for (; pos < text.length(); ++pos)
				{
					if ((text[pos] >= '0'&&text[pos] <= '9') || (text[pos] >= 'a'&&text[pos] <= 'f')
						|| (text[pos] >= 'A'&&text[pos] <= 'F'))
					{
						write_flag = 2;
						num = (num << 4) | ch2int(text[pos]);
					}

				}
			}
			if (write_flag == 2)
			{
				chr_addr.push_back({ rom_addr,num });
			}

		}
	}
	in.close();

	string fileName = og::checkPath("t2t.nes");
	string outName = og::checkPath("chr.bin"); 

	unPack(fileName, outName, chr_addr);
	Pack(fileName, fileName, chr_addr);
	 

	return 0;
}