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

#if defined(OG_PLATFORM_ANDROID)
#include <android/asset_manager.h>
extern AAssetManager* g_assetManager;   // defined in some .cpp file
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
	// Resources directory
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
		return dir + "Resources/";

#elif defined(OG_PLATFORM_ANDROID)
		return "Resources/";   // path relative to assets

#elif defined(OG_PLATFORM_APPLE)
		return "Resources/";   // adjust for macOS / iOS as needed

#elif defined(OG_PLATFORM_LINUX)
		return "Resources/";   // adjust for Linux as needed

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

	// ============================================================
	// Unified file read
	// ============================================================
	inline bool readFile(const std::string& path, std::string& out)
	{
#if defined(OG_PLATFORM_ANDROID)
		if (!g_assetManager) return false;
		AAsset* asset = AAssetManager_open(g_assetManager, path.c_str(), AASSET_MODE_BUFFER);
		if (!asset) return false;
		off_t len = AAsset_getLength(asset);
		out.resize((size_t)len);
		AAsset_read(asset, &out[0], (size_t)len);
		AAsset_close(asset);
		return true;
#else
		FILE* f = fopen(path.c_str(), "rb");
		if (!f) return false;
		fseek(f, 0, SEEK_END);
		long len = ftell(f);
		fseek(f, 0, SEEK_SET);
		out.resize((size_t)len);
		fread(&out[0], 1, (size_t)len, f);
		fclose(f);
		return true;
#endif
	}

} // namespace og

#endif