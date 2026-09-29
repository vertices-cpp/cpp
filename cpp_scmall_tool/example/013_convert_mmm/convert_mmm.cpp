#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include "path_head.h"
static std::string prePath = og::checkPath("011_convert_mmm/");

#pragma warning(disable:4996)

// unsigned char file_info[18] = {
//     0x4D, 0x4D, 0x4D, 0x00, 0x00, 0x03, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
//     0x00, 0x00
// };

void convert(const string &fileName) {
	std::fstream in, out;

	in.open(fileName, std::ios::in | std::ios::binary);
	if (!in)
		return;

	string outName = fileName;
	outName = outName.substr(0, outName.find_last_of('.') + 1);

	outName += "converted_mmm_by77";

	out.open(outName, std::ios::out | std::ios::binary);
	char c = 0x0, d;

	if (in)
	{
		// for (i = 0; i < 18; ++i)
		// {
		//     c = (signed char)file_info[i];
		//     out.put(c);
		// }
		while (in) {
			in.get(d);
			out.put(c);
			out.put(d);
			// out.put(c);
			// out.put(d);

			for (int i = 0; i < 14; ++i)
			{
				out.put(c);
			}
		}
	}

	in.close();
	out.close();
}

int main(int argc, char **argv) {
	// if (argc < 2)
	// {
	//
	//     system("pause");
	//     return 0;
	// }
	std::string path = prePath + "NewTilemap.tilemap";
	// for (int i=1;i<argc;++i)
	{
		convert(path.c_str());
	}
	std::cout << "Done! Use WinHex to overwrite the hex content starting at 0x15" << std::endl;
	system("pause");
	return 0;
}