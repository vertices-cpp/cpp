#ifndef _PATH_HELPER_H_
#define _PATH_HELPER_H_

#include <string>

// ============================================================
// Platform detection
// ============================================================
#if defined(__ANDROID__)
#define OG_PLATFORM_ANDROID 1
#elif defined(_WIN32)
#define OG_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
#define OG_PLATFORM_APPLE 1
#elif defined(__linux__)
#define OG_PLATFORM_LINUX 1
#endif

#if defined(OG_PLATFORM_WINDOWS)
#include <windows.h>
#endif

namespace og {

	inline std::string convertPathFormatToUnixStyle(const std::string& path)
	{
		std::string ret = path;
		for (size_t i = 0; i < ret.size(); ++i)
			if (ret[i] == '\\') ret[i] = '/';
		return ret;
	}

	// ============================================================
	// Executable name (without .exe)
	// ============================================================
	inline std::string getExecutableName()
	{
#if defined(OG_PLATFORM_WINDOWS)
		char buf[MAX_PATH];
		DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
		if (len == 0 || len == MAX_PATH) return "";
		std::string full(buf, len);
		size_t slash = full.find_last_of("\\/");
		std::string name = (slash != std::string::npos) ? full.substr(slash + 1) : full;
		size_t dot = name.find_last_of('.');
		if (dot != std::string::npos) name = name.substr(0, dot);
		return name;
#else
		return "";
#endif
	}

	// ============================================================
	// Resources directory: <exeDir>/Resources/<exeName>/
	// ============================================================
	inline std::string getResourceDir()
	{
#if defined(OG_PLATFORM_WINDOWS)
		char buf[MAX_PATH];
		DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
		if (len == 0 || len == MAX_PATH) return "Resources/";
		std::string full(buf, len);
		size_t pos = full.find_last_of("\\/");
		std::string dir = (pos != std::string::npos) ? full.substr(0, pos + 1) : "";
		for (auto& c : dir) if (c == '\\') c = '/';

		return dir + "Resources/" + getExecutableName() + "/";

#elif defined(OG_PLATFORM_ANDROID)
		return "Resources/";

#elif defined(OG_PLATFORM_APPLE)
		return "Resources/";

#elif defined(OG_PLATFORM_LINUX)
		return "Resources/";

#else
		return "Resources/";
#endif
	}

	// ============================================================
	// Unified path join
	// ============================================================
	inline std::string checkPath(const std::string& path)
	{
		return convertPathFormatToUnixStyle(getResourceDir() + path);
	}

} // namespace og

#endif