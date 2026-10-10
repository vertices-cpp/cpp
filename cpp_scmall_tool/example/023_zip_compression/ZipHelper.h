#ifndef _ZIP_HELPER_H_
#define _ZIP_HELPER_H_

#include <string>
#include <vector>
#include "OGPlatformMacros.h"

OG_BEGIN

bool compress(
	const std::vector<std::string>& srcPaths,
	const std::string& zfd,
	const std::string& password = "",
	const std::string& xorKey = ""
);

bool decompress(
	const std::string& zipFile,
	const std::string& outDir,
	const std::string& password = "",
	const std::string& xorKey = ""
);

OG_END

#endif