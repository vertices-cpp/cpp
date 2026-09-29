#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <fstream>

#include "path_head.h"
static std::string prePath = og::checkPath("013_huffman_compression/");

#if defined(_WIN32)||defined(_WIN64)
#pragma warning(disable:4996)
#endif

// #define CHAR_4SIZE

#define COMPRESSION   // method selection

using namespace std;

typedef struct HuffmanNode
{
	// Includes parent, left/right child, code, code length, run length, frequency and symbol
	int parent, left, right, code, codelen, runlen, freq;
	char symbol;
	HuffmanNode() :parent(0), left(0), right(0), code(0), codelen(0), runlen(0), freq(0), symbol(' ') {}
	HuffmanNode(int p, int l, int r, int c, int clen, int rlen, int f, char s) :
		parent(p), left(l), right(r), code(c), codelen(clen), runlen(rlen), freq(f), symbol(s) {}
	HuffmanNode(const HuffmanNode &r) :parent(r.parent),
		left(r.left), right(r.right), code(r.code), codelen(r.codelen), runlen(r.runlen), freq(r.freq), symbol(r.symbol) {}

	bool operator<(const HuffmanNode &r)const {
		return freq < r.freq;
	}
	bool operator==(const HuffmanNode &r)const {
		return symbol == r.symbol && runlen == r.runlen;
	}
	friend ostream& operator<<(ostream &os, const HuffmanNode &r) {
		os << "parent:" << r.parent << " left:" << r.left << " right:" << r.right << "  ";
		string binCode = "";
		int val = r.code;
		for (int i = 0; i < r.codelen; ++i)
		{
			binCode += ((val & 1) + '0');
			val >>= 1;
		}
		binCode = string(binCode.rbegin(), binCode.rend());
		os << "<code " << r.code << ": " << binCode << "> code length:" << r.codelen << " run length:" << r.runlen << " symbol:" << r.symbol;
		return os;
	}
}HuffmanNode;

class HuffmanCoding {
	static const int arr_size = 100;
	vector<HuffmanNode> hTree;
	int chars[256];
	char outFileNamePre[arr_size];
	char outFileName[arr_size];
	int dataSize, binarySize;
	// string buffer;

	fstream fIn, fOut;

public:
	friend ostream& operator<<(ostream &os, const HuffmanCoding &r) {
		for (size_t i = 0; i < r.hTree.size(); ++i)
		{
			os << i << " " << r.hTree[i] << "\n";
		}
		return os;
	}
	HuffmanCoding() {
		for (int i = 0; i < 256; ++i)
		{
			chars[i] = 0;
		}
	}

	void toList(int p) {
		if (hTree[p].left == 0 && hTree[p].right == 0)
		{
			char code = hTree[p].symbol;
			hTree[p].right = chars[(unsigned char)code];
			chars[(unsigned char)code] = p;
		}
		else
		{
			toList(hTree[p].left);
			toList(hTree[p].right);
		}
	}

	void Select(int end, int &m1, int &m2) {
		int w1, w2;
		w1 = w2 = INT_MAX;

		for (int i = 1; i <= end; ++i)
		{
			if (hTree[i].parent != 0)continue; // skip if parent already processed
			// if current run length is less than current
			if (hTree[i].freq < w1)
			{
				// shift the second run length to 1
				w2 = w1, m2 = m1;
				// get current length and index
				w1 = hTree[i].freq, m1 = i;
			}
			// if current is >= node1 and < node2, get the second smallest position
			else if (hTree[i].freq >= w1 &&
				hTree[i].freq < w2) {
				w2 = hTree[i].freq;
				m2 = i;
			}
		}
	}
	void createHuffmanTree() {

		int hTreeCnt = dataSize * 2 - 1;

		hTree.resize(hTreeCnt + 1);

		int m1, m2;
		for (int i = dataSize + 1; i <= hTreeCnt; ++i)
		{
			Select(i - 1, m1, m2);
			// both left and right nodes get their parent
			hTree[m1].parent = hTree[m2].parent = i;
			// assign left and right nodes
			hTree[i].left = m1, hTree[i].right = m2;
			// compute total length
			hTree[i].freq = hTree[m1].freq + hTree[m2].freq;
		}

#ifdef COMPRESSION
		// traverse from the root backwards
		int f = 0, j;

		for (int i = 1; i <= dataSize; ++i)
		{
			int level = 0, code = 0;
			for (j = i, f = hTree[i].parent;
				f != 0;
				j = f, f = hTree[f].parent) {

				if (hTree[f].left == j)
				{
					level++;
				}
				else if (hTree[f].right == j) {
					code += (1 << level); // insert from the tail backwards
					level++;
				}

			}
			hTree[i].code = code, hTree[i].codelen = level;
		}
#else
		// preorder traversal
		int r = hTreeCnt, len = 0;

		auto H = hTree;

		for (auto &i : H)
		{
			i.runlen = 0;
		}

		int level = 0, code = 0;

		while (r != 0)
		{
			if (H[r].runlen == 0) {
				// mark as going left first
				H[r].runlen = 1;

				if (H[r].left != 0) {
					r = H[r].left;
					level++;
					code <<= 1;
				}
				else if (H[r].right == 0) {

					hTree[r].code = code, hTree[r].codelen = level;
				}
			}
			else if (H[r].runlen == 1) {
				// mark as going right
				H[r].runlen = 2;

				if (H[r].right != 0) {
					r = H[r].right;
					level++;
					code = (code << 1) + 1;
				}
			}
			else {
				r = H[r].parent;
				code >>= 1;
				level--;
			}
		}
#endif
	}

	void readNormalFile() {

		char ch, ch2;
		HuffmanNode r;
		vector<HuffmanNode>::iterator i;

		for (fIn.get(ch); !fIn.eof(); ch = ch2) {
			r.freq = 1;
			// keep reading the next character, counting run length (until different from ch or end)
			for (r.runlen = 1, fIn.get(ch2); !fIn.eof() && ch2 == ch; r.runlen++)
				fIn.get(ch2);

			// buffer += ch; // used to write the encoding order
			r.symbol = ch; // store the character

			i = find(hTree.begin(), hTree.end(), r);
			if (i == hTree.end())
				hTree.push_back(r);
			else
				i->freq++;

		}
		// sorted afterwards for writing to file
		//     std::sort(hTree.begin(), hTree.end());
		// get file length
		fIn.clear();

		dataSize = hTree.size();
		binarySize = (int)fIn.tellg();
		hTree.insert(hTree.begin(), HuffmanNode(0, 0, 0, 0, 0, dataSize, binarySize, ' '));
	}

	int read4Byte() {
		int num = 0; 
		for (int i = 0; i < sizeof(num); ++i)
		{
			num <<= 8;
			num += fIn.get();
		}
		return num;
	}
	void write4Byte(int num) {
		int bit_v = 32;
		int len = sizeof(num);
		for (size_t i = 0; i < len; i++)
		{
			bit_v -= 8;
			// write values from left to right xx xx xx xx
			fOut.put((num >> bit_v) & 0xff);
		}
	}
	void writeCompressionFile(const char *fileName) {

		memset(outFileName, 0, arr_size);
		int len = strlen(fileName);

		strcpy(outFileName, outFileNamePre);
		strcat(outFileName, fileName);

		if (strrchr(outFileName, '.'))
		{
			strcpy(strrchr(outFileName, '.') + 1, "bin");
		}
		else strcat(outFileName, ".bin");

		cout << outFileName << endl;

		fOut.open(outFileName, ios::out | ios::binary);
		// write file name length
		write4Byte(len);
		// write file name
		fOut.write(fileName, len);
		// write data length
		write4Byte(dataSize);
		// write file length
		write4Byte(binarySize);

		// write code table
		for (int i = 1; i < dataSize + 1; ++i)
		{
			// write symbol and run length separately
			fOut.put(hTree[i].symbol);
#ifdef CHAR_4SIZE
			write4Byte(hTree[i].runlen);
#else
			fOut.put(hTree[i].runlen >> 8).put(hTree[i].runlen);
#endif
			write4Byte(hTree[i].freq);
		}

		// write the binary encoding sequence
		long packCnt = 0, maxPack = 32, pack = 0, bitsLeft = 0;
		// the first one is an empty node
		char c, c2;
		int runlen = 1, p = 0;
		for (fIn.get(c); !fIn.eof();)
		{

			for (runlen = 1, fIn.get(c2); !fIn.eof() && c == c2; runlen++)
			{
				fIn.get(c2);
			}

			for (p = chars[(unsigned char)c];
				p != 0 && runlen != hTree[p].runlen;
				p = hTree[p].right)
				//     cout << runlen << " " << p << " " << hTree[p].runlen << endl;

				if (p == 0)
					cerr << "encode error " << endl;

			// 32-bit max minus counter, rounded
			if (hTree[p].codelen < maxPack - packCnt) {
				// shift content left by code length + the code itself
				pack = (pack << hTree[p].codelen) | hTree[p].code;
				// add code length to counter
				packCnt += hTree[p].codelen;
			}
			else
			{
				// when code length exceeds available space, compute the left-shift value
				bitsLeft = maxPack - packCnt;
				// (pack holds the previous value) first shift pack left by this value
				pack <<= bitsLeft;
				// if not equal, the code length is larger than the shift value
				if (hTree[p].codelen > bitsLeft)
				{
					// get the code
					int hold = hTree[p].code;
					// right-shift the code by (code length - available length)
					hold = hold >> (hTree[p].codelen - bitsLeft);
					// then add only the high bits remaining
					pack |= hold;
				}

				else // if equal, just add the code directly to pack
					pack |= hTree[p].code;

				// write the int value
				write4Byte(pack);

				// if larger than bitsLeft
				if (hTree[p].codelen > bitsLeft)
				{
					// handled earlier, just get the code
					pack = hTree[p].code;
					// get remaining length after shifting
					packCnt = hTree[p].codelen - bitsLeft;
				}
				else
					packCnt = 0;

			}
			c = c2;
		}
		// if not zero, there is still data not stored
		if (packCnt != 0) {
			pack = pack << (maxPack - packCnt);
			write4Byte(pack);
		}
		fOut.close();
	}
	void compress(const char *fileName) {
		fIn.open(fileName, ios::in | ios::binary);
		if (!fIn)
		{
			cerr << "Missing <" << fileName << "> file" << endl;
			getchar();
			return;
		}
		readNormalFile();

		createHuffmanTree();
		// start from the root node
		fIn.seekg(0, ios::beg);
		toList(dataSize * 2 - 1);

		std::string finalName = fileName;
		finalName = finalName.substr(finalName.find_last_of('/')+1);
		writeCompressionFile(finalName.c_str());

		fIn.close();

		cout << "Compression complete" << endl;
	}
	void readCompressionFile()
	{
		memset(outFileName, 0, arr_size);

		// read file name length, file name, data count, output file length
		int len = read4Byte();
		char tmp[arr_size]{ 0 };

		fIn.read(tmp, len);

		strcpy(outFileName, outFileNamePre);
		strcat(outFileName, tmp); // copy to output file
		strcpy(strrchr(outFileName, '.'), "--decompressed"); // then copy the extension content
		strcat(outFileName, strchr(tmp, '.')); // append the tail

		dataSize = read4Byte();
		binarySize = read4Byte();
		// get node contents
		HuffmanNode data;
		for (int i = 0; i < dataSize; ++i)
		{
			// read one symbol at a time
			data.symbol = fIn.get();
#ifdef CHAR_4SIZE
			data.runlen = read4Byte();
#else
			data.runlen = 0;
			data.runlen = fIn.get() << 8;
			data.runlen |= fIn.get();
#endif
			data.freq = read4Byte();
			hTree.push_back(data);
		}
		hTree.insert(hTree.begin(), HuffmanNode(0, 0, 0, 0, 0, dataSize, binarySize, ' '));

	}
	void writeOutFile() {

		cout << outFileName << endl;
		fOut.open(outFileName, ios::out | ios::binary);
		char c;
		int chars = 0, m = dataSize * 2 - 1, bitCnt = 1;
		for (chars = 0, fIn.get(c); !fIn.eof() && chars < binarySize;)
		{
			for (int p = m;;)
			{
				if (hTree[p].left == 0 && hTree[p].right == 0)
				{
					for (int j = 0; j < hTree[p].runlen; ++j)
					{
						fOut.put(hTree[p].symbol);
					}
					chars += hTree[p].runlen;
					break;
				}
				else if ((c & 0x80) == 0)
					p = hTree[p].left;
				else
					p = hTree[p].right;
				if (bitCnt++ == CHAR_BIT) {
					fIn.get(c);
					bitCnt = 1;
				}
				else
					c <<= 1;
			}
		}

		fOut.close();
	}
	void decompress(const char *fileName) {
		fIn.open(fileName, ios::in | ios::binary);
		if (!fIn)
		{
			cerr << "Missing <" << fileName << "> file" << endl;
			getchar();
			return;
		}
		// read the compressed file
		readCompressionFile();
		// build the tree
		createHuffmanTree();
		writeOutFile();
		fIn.close();
		cout << "Decompression complete" << endl;
	}
	void GetFile(const string &fileName) {
		string s = fileName;
		int i;
		if ((i = s.find_last_of('\\')) != string::npos || (i = s.find_last_of('/')) != string::npos)
		{
			memset(outFileNamePre, 0, arr_size);
			memcpy(outFileNamePre, s.substr(0, i + 1).c_str(), s.substr(0, i + 1).size());
			s = s.substr(i + 1);

		}
		if (s.find(".bin") != string::npos)
		{
			decompress(fileName.c_str());
		}
		else
			compress(fileName.c_str());

	}
};

int main(int argc, char *argv[]) {
	// Drag the file in directly, to decompress or compress
	//     if (argc >= 2)
	//     for (int i = 1; i < argc; ++i)
	//     {
	//         HuffmanCoding h, h2;
	//         h.GetFile(argv[i]);
	//         cout << "Press Enter to continue" << endl;
	//         // getchar();
	//     }
	//     else
	//     {
	//         cout << "No file! Press Enter to continue" << endl;
	//         getchar();
	//     }
	HuffmanCoding h, h2;
	h.GetFile(prePath + "1.txt");
	h2.GetFile(prePath + "1.bin");
}