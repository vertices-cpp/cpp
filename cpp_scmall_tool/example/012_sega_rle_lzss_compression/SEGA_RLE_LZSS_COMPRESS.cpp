// Sega game decompression
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <memory>
#include <climits>

#define NOMINMAX
#include "path_head.h"
static std::string prePath = og::checkPath("012_sega_rle_lzss_compression/");

using namespace std;

class UseTime {
	clock_t start;
public:
	UseTime() {
		startTime();
	}
	void startTime() {
		start = clock();
	}
	friend ostream& operator<<(ostream& os, const UseTime &r)
	{
		return os << "Elapsed:" << ((double)(clock() - r.start) / CLOCKS_PER_SEC) << "s";
	}
};

// Read file into data
void ReadFile(const char *file, string &data)
{
	fstream fpr(file, ios::in | ios::binary);
	if (!fpr.is_open())
	{
		cerr << "Failed to open: " << file << endl;
		return;
	}

	fpr.seekg(0, ios::end);
	size_t size = fpr.tellg();
	fpr.seekg(0, ios::beg);

	data.resize(size);
	fpr.read(&data[0], size);
	fpr.close();
}

// Write file
void WriteFile(const char *file, string data)
{
	fstream fpw(file, ios::out | ios::binary);
	fpw.write(data.c_str(), data.size());
	fpw.close();
}

int bclear(int value, int index) {
	return ((value >> index) & 1) ? (value ^ (1 << index)) : value;
}

int rotate_bits_left(int value = 0x0, int bits_to_rotate = 0)
{
	return (value << bits_to_rotate % CHAR_BIT) &
		((int)pow(2, CHAR_BIT) - 1) |
		((value & ((int)pow(2, CHAR_BIT) - 1)) >> (CHAR_BIT - (bits_to_rotate % CHAR_BIT)));

}

void decompressFile(string &data, int offset) {

	int data_cursor = offset, // current offset being read
		low = 0, high = 0, compressed_size = 0, _readed = 0;

	if (data[data_cursor] == 0)
	{
		cout << "Decompression failed!" << endl;
		return;
	}
	else
	{
		string _output;

		low = data[data_cursor++] & 0xff;   // read low byte
		high = data[data_cursor++] & 0xff;  // read high byte
		compressed_size = ((high << 8) | low);

		while (data_cursor < offset + compressed_size)
		{

			int ctrl = data[data_cursor++] & 0xff;
			int repeats = 0, position = 0;
			if (!(bclear(ctrl, 7) == ctrl))
			{
				ctrl = bclear(ctrl, 7);
				repeats = rotate_bits_left((ctrl & 0x60), 3) + 4;
				position = ((ctrl & 0x1f) << 8) | (data[data_cursor++] & 0xff);

				while (true)
				{
					for (int i = 0; i < repeats; ++i)
						_output += _output[_output.size() - position];

					ctrl = data[data_cursor++] & 0xff;
					if ((ctrl & 0xe0) == 0x60)
						repeats = (ctrl & 0x1f);
					else
					{
						data_cursor -= 1;
						break;
					}
				}
			}
			else if (!(bclear(ctrl, 6) == ctrl))
			{
				ctrl = bclear(ctrl, 6);
				if (!(bclear(ctrl, 4) == ctrl))
				{
					ctrl = bclear(ctrl, 4);
					repeats = (ctrl <<= 8) | data[data_cursor++] & 0xff; // read new address
				}
				else
					repeats = ctrl;

				repeats += 4;
				_readed = data[data_cursor++];

				for (int i = 0; i < repeats; ++i)
					_output.append(string(1, _readed));
			}
			else
			{
				if (!(bclear(ctrl, 5) == ctrl))
				{
					ctrl = bclear(ctrl, 5);
					ctrl = (ctrl << 8) | data[data_cursor++] & 0xff; // read new address
				}

				repeats = ctrl;
				for (int i = 0; i < repeats; ++i)
				{
					_output += data[data_cursor++];
				}
			}
		}
		printf("Original size:%.8x Decompressed size:%.8x\n", compressed_size + 1, _output.size());
		data = _output;
	}
}

int find_best_rle_match(string &data, int _encoded);
shared_ptr<int> find_best_lz_match(string &data, int _encoded);
void flush_window(string &_output, string &_window);

string compress(string &data)
{
	string _window;

	string _output;
	_output.append(string(1, 0x0));
	_output.append(string(1, 0x0));
	int _encoded = 0, _data_cursor = 0, _readed = 0;

	while (_encoded < data.size()) {
		// Search for RLE match
		int rle_match = find_best_rle_match(data, _encoded);
		// Search for LZ matches
		auto lz_match = find_best_lz_match(data, _encoded);
		// RAW
		if (rle_match < 4 && *lz_match < 4)
		{
			_readed = data[_data_cursor++];
			_window.append(string(1, _readed));
			if (_window.size() > 0x1FFF)
				flush_window(_output, _window);
			_encoded += 1;
		}
		else if (rle_match >= lz_match.get()[0])
		{
			if (_window.size() > 0)
				flush_window(_output, _window);
			for (int i = 0; i < rle_match; ++i)
				_readed = data[_data_cursor++] & 0xff;
			_encoded += rle_match;
			rle_match -= 4;
			if (rle_match > 0xF)
			{
				_output.append(string(1, 0x40 | 0x10 | ((rle_match >> 8) & 0xF)));
				_output.append(string(1, rle_match & 0xFF));
			}
			else
				_output.append(string(1, 0x40 | (rle_match & 0xF)));
			_output.append(string(1, _readed));
		}
		// LZ
		else
		{
			if (_window.size() > 0)
				flush_window(_output, _window);
			_data_cursor += lz_match.get()[0];
			_encoded += *lz_match;
			int lz_length = lz_match.get()[0], lz_offset = lz_match.get()[1];
			lz_length -= 4;
			int length = lz_length > 3 ? 3 : lz_length;

			_output.append(string(1, 0x80 | (length << 5) | ((lz_offset >> 8) & 0x1F)));
			_output.append(string(1, lz_offset & 0xFF));
			lz_length -= length;
			while (lz_length > 0)
			{
				length = lz_length > 0x1F ? 0x1f : lz_length;
				_output.append(string(1, 0x60 | length));
				lz_length -= length;
			}
		}
	}
	if (_window.size() > 0)
		flush_window(_output, _window);
	_output[0] = _output.size() & 0xFF;
	_output[1] = _output.size() >> 8;
	_output.append(string(1, 0x0));
	printf("Original size:%.8x Compressed size:%.8x\n", data.size(), _output.size());

	return _output;
}

int find_best_rle_match(string &data, int _encoded)
{
	int best_match = 0;
	for (int i = 0; i < (std::min)(0xFFF + 4, (int)data.size() - _encoded); ++i)
	{
		best_match = i;
		if (data[_encoded] != data[_encoded + i])
			break;
	}
	return best_match;
}

shared_ptr<int> find_best_lz_match(string &data, int _encoded) {
	int best_match_length = 0;
	int best_match_offset = 0;
	for (int i = 1; i < (std::min)(0x1FFF, _encoded); ++i)
	{
		for (int j = 0; j < (data.size() - _encoded); ++j)
		{
			if (data[_encoded + j] != data[_encoded - i + j])
				break;
			if (j + 1 >= best_match_length)
			{
				best_match_length = j + 1;
				best_match_offset = i;
			}
		}
	}
	return shared_ptr<int>(new int[2]{ best_match_length, best_match_offset },
		[](int *p) {if (p != NULL) { delete[] p; }});
}

void flush_window(string &_output, string &_window) {
	if (_window.size() > 0x1F)
	{
		_output.append(string(1, 0x20 | ((_window.size() >> 8) & 0x1F)));
		_output.append(string(1, _window.size() & 0xFF));
	}
	else
		_output.append(string(1, (char)_window.size()));
	for (int i = 0; i < _window.size(); ++i)
		_output.append(string(1, _window[i]));

	_window.clear();
}

int main()
{
	string compressFileName = prePath + "compress.bin";
	string decompressFileName = prePath + "1.bin";
	string compressFileName1 = prePath + "compress1.bin";

	string data;
	ReadFile(compressFileName.c_str(), data);

	UseTime useTime;
	decompressFile(data, 0x0);
	cout << useTime << endl;

	WriteFile(decompressFileName.c_str(), data);

	ReadFile(decompressFileName.c_str(), data);

	useTime.startTime();
	data = compress(data);
	cout << useTime << endl;

	WriteFile(compressFileName1.c_str(), data);
}


// 
// 		*(binary: d7 d6 d5 d4 d3 d2 d1 d0)
// 		*   (binary: 0 0 0 0 0 0 0 0)
// 		UNZ_DAT:
// 		MOVEM.L	D0 - D2 / A2, -(A7); 图像数据解压缩
// 		UDAT1 :
// 		MOVE.L	A0, D1; 读取当前a偏移的地址
// 		MOVEQ	#$00, D2; d2清0
// 		MOVE.B(A0) + , D2; 读低位
// 		MOVE.B(A0) + , D0; 读高位
// 		LSL.W	#8, D0; 左移8位
// 		OR.W	D0, D2; (或操作)合并高低位地址
// 		ADD.L	D1, D2; 然后加上偏移的值
// 		UDAT2 :
// 		CMP.L	D2, A0; 判断a0是否结束
// 		BEQ	UDAT10; 如果结束则跳转到UDAT10
// 		MOVEQ	#$00, D0; 清除d0
// 		MOVE.B(A0) + , D0; 读取一个8位
// 		BCLR	#$07, D0; 检测是0x80(d7 bit)
// 		BNE.s	UDAT8; 如果是, 则清d7位并跳转到UDAT8    余下作为计数器
// 		BCLR	#$06, D0; 检测是0x40(d6, bit)
// 		BNE.s	UDAT5; 如果是, 则清(d6 bit)   余下作为计数器
// 		BCLR	#$05, D0;; 检测是0x20(d5, bit)
// 		BEQ.s	UDAT3; 如果是, 则清d5   余下作为计数器高位
// 		LSL.W	#8, D0; 如果是0, 则把d0剩余的数进行左移8位
// 		MOVE.B(A0) + , D0; 组成一个新的计数器
// 		UDAT3 :
// 		SUBQ.W	#$1, D0; 计数器减1
// 		UDAT4 :
// 		MOVE.B(A0) + , (A1)+; 拷贝内容
// 		DBRA	D0, UDAT4; 循环
// 		BRA.s	UDAT2; 拷贝完就返回UDAT2继续判断是否结束
// 		UDAT5 :
// 		BCLR	#$04, D0; 检测是不是0x10，是则继续
// 		BEQ.s	UDAT6
// 		LSL.W	#8, D0; 如果d0的0x10不是0，则左移8位
// 		MOVE.B(A0) + , D0; 组成新计数器
// 		UDAT6 :
// 		ADDQ.W	#$3, D0; 然后 + 3
// 		MOVE.B(A0) + , D1; 然后把重复数据放入d1
// 		UDAT7 :
// 		MOVE.B	D1, (A1)+; 拷贝
// 		DBRA	D0, UDAT7
// 		BRA.s	UDAT2; 结束完继续判断是否结束
// 		UDAT8 :
// 		MOVE.L	D0, D1; 拷贝一个长字节放到d1
// 		AND.B	#$60, D0; 获取d0的d6, d5位的值，屏蔽其它位
// 		ROL.B	#3, D0; 0~3
// 		ADDQ.W	#$3, D0; +31
// 		AND.B	#$1F, D1; 得到低五位0~1f给d1
// 		LSL.W	#8, D1; 左移八位00 xx ~1f xx
// 		MOVE.B(A0) + , D1; 获得低位
// 		MOVE.L	A1, A2; 把a1拷贝到a2
// 		SUB.L	D1, A2; 把a2减d1
// 		UDAT9 :
// 		MOVE.B(A2) + , (A1)+; 把a2地址的值存到a1
// 		DBRA	D0, UDAT9; 等循环处理完
// 		MOVE.B(A0), D1; 读取一个a0的byte传给d1
// 		AND.B	#$E0, D1; 获取高三位的d1
// 		CMP.B	#$60, D1; 判断是不是0x60
// 		BNE	UDAT2; 如果不是就跳转udat2
// 		CMP.L	D2, A0; 否则判断d2是不是a0
// 		BEQ.s	UDAT10; 相同就转到udat10
// 		MOVE.B(A0) + , D0; 拷贝数据
// 		AND.L	#$0000001F, D0; 获取0~1f之间的值
// 		SUBQ.W	#$1, D0; 减1
// 		BRA.s	UDAT9; 循环
// 		UDAT10 :
// 		MOVE.B(A0) + , D0
// 		BNE	UDAT1
// 		MOVEM.L(A7) + , D0 - D2 / A2
// 		RTS
