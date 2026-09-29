// Written by FeiFei77Yu
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "path_head.h"
static std::string prePath = og::checkPath("011_convert_mmm/");

class Packbits
{
public:
	Packbits() = default;
	/* Pass in data. 1: data. 2: mode "code" or "decode". 3: start position, default 0 */
	void move_data(const std::string&, const std::string&, int);
	/* Encode */
	void packbits();
	/* Decode */
	void unpackbits();
	std::string getdata() { return data; }
	std::string getout_d() { return out_d; }
	std::string getunpack_src() { return unpack_src; }
	std::size_t cur() { return start_cur; }
	const char* data_c_str()
	{
		return data.c_str();
	}
	void free() {
		data.clear();
		out_d.clear();
		unpack_src.clear();
	}
	/* Return the decoded string */
	const char* out_d_c_str()
	{
		return out_d.c_str();
	}
	//private:
	int start_cur = 0;
	std::string data;
	std::string out_d;
	std::string unpack_src;
};

void Packbits::move_data(const std::string& str,
	const std::string& control_str = "", int p = 0)
{
	if (control_str == "" || control_str == "code")
	{
		unpack_src = str;
		packbits();
	}
	else
	{
		start_cur = p;
		data = str;
		unpackbits();
	}
}

void Packbits::packbits()
{
	int count;
	int Length = unpack_src.size(), i = 0, t;

	while (i < Length)
	{
		if (i <= (Length - 2) &&
			unpack_src[i] == unpack_src[i + 1])
		{
			for (t = i + 2; t < Length && unpack_src[t] == unpack_src[i]
				&& t - i < 0x7E;) // 0x7F changed to 0x7E, otherwise Contra will write to screen address
				++t;
			count = t - i;
			out_d += count;
			out_d += unpack_src[i];
		}
		else
		{
			for (t = i; t < Length && t - i < 0x7E;) // 0x7F changed to 0x7E, otherwise it becomes 0xFF terminator after loop
				if (t <= (Length - 3) && unpack_src[t] == unpack_src[t + 1]
					&& unpack_src[t] == unpack_src[t + 2])
					break;
				else
					++t;
			count = t - i;
			out_d += (count | 0x80);
			out_d += unpack_src.substr(i, count);
		}
		i += count;
	}
	/* Terminator */
	out_d += (char)0xFF;
}

void Packbits::unpackbits()
{
	int len;
	int val, i = start_cur;
	int Length = data.size();
	while (i < Length)
	{
		len = data[i++] & 0xFF;
		/* Stop on 0xFF */
		if (len == 0xFF)
			break;
		if (len < 0x80)
		{
			/* Take one byte */
			val = data[i++] & 0xFF;
			/* Append len copies of val to the output */
			out_d += std::string(len, val);
		}
		else
		{
			/* Clear the highest bit */
			len &= 0x7F;
			/* Copy len bytes starting at i from the input */
			out_d += data.substr(i, len);
			/* Advance the offset */
			i += len;
		}
	}
	std::cout << "Decoded " << std::hex << i << " bytes" << std::endl;
}

std::fstream& input(std::fstream &in, Packbits &rhs,
	const std::string& str, int pos)
{
	std::ostringstream os_n;
	os_n << in.rdbuf();
	rhs.move_data(os_n.str(), str, pos);
	return in;
}

std::fstream& output(std::fstream &os, Packbits &rhs)
{
	os.write(rhs.out_d_c_str(), rhs.getout_d().size());
	return os;
}

std::fstream& outputScoure(std::fstream &os, Packbits &rhs)
{
	std::string s = rhs.getdata();
	auto first = s.begin() + rhs.cur(),
		last = s.begin() + (rhs.cur() + rhs.getout_d().size());
	s.replace(first, last, rhs.getout_d());
	os.write(s.c_str(), s.size());
	return os;
}

void Test(Packbits &p, const std::string &control_str, int cur)
{
	std::string fileName, outName;
	if (control_str == "decode")
		fileName = prePath + "1.bin", outName = prePath + "2.bin";
	else
		fileName = prePath + "2.bin", outName = prePath + "3.bin";

	std::fstream in(fileName, std::ios::in | std::ios::binary);
	if (!in)
	{
		std::cerr << "Missing " << fileName << ", press any key to exit!" << std::endl;
		std::cin.get();
		exit(1);
	}
	/* The file is fileName, pass it to p, call encode/decode, start position is cur */
	input(in, p, control_str, cur);
	in.close();

	if (remove(outName.c_str()) == 0)
		std::cout << "ok" << std::endl;

	std::fstream out(outName, std::ios::out | std::ios::binary);
	if (!out)
		std::cerr << "Failed to create file" << std::endl;
	output(out, p);
	out.close();
	std::cout << outName << " output complete" << std::endl;

	if (fileName == "2.bin" && p.getdata().size() != 0)
	{
		char c;
		std::cerr << "Write back into the original file 1.bin? Y/N\n";
		std::cin >> c;
		while (c != 'N' && c != 'n')
		{
			if (c == 'N' || c == 'n' || c == 'Y' || c == 'y')
			{
				std::string tst = "1.bin";
				std::fstream outfile;
				outfile.open(tst, std::iostream::out | std::iostream::binary);
				outputScoure(outfile, p);
				outfile.close();
				std::cout << "Output " << tst << " complete!" << std::endl;
				break;
			}
			else
			{
				std::cerr << "Invalid input, please try again!" << std::endl;
				std::cin >> c;
			}
		}
	}
}

int main()
{
	std::string control_str;

	Packbits p;
	while ([&control_str]()->bool {
		std::cout << "===============\n"
			"Choose a file: source file 1.bin (place it in the cpp directory)\n"
			"Input mode:\n"
			"decode: decode (decode 1.bin, output to 2.bin)\n"
			"code: encode (encode 2.bin, output to 3.bin; you must decode 1.bin to 2.bin first)\n"
			"end: quit!\n"
			"Please enter your choice:" << std::endl;
		std::cin >> control_str; return control_str != "end"; }())
	{
		if (control_str != "code" && control_str != "decode")
		{
			std::cerr << "Invalid input! Please try again" << std::endl;
			continue;
		}

		int cur;

		if (control_str == "decode")
		{
			std::cout << "Enter the start position for processing:" << std::endl;
			while ([&cur]() {std::cin >> cur; return cur <= -1; }())
			{
				std::cerr << "Invalid input! Please try again" << std::endl;
			}
		}
		else
		{
			std::cout << "Encode mode uses position 0 of 2.bin by default" << std::endl;
			cur = 0;
		}

		p.free();
		Test(p, control_str, cur);
	}
		return 0;
}