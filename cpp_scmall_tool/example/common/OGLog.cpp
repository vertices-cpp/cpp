#include "OGPlatformMacros.h"

OG_BEGIN


void log(const char * format, ...)
{
	int bufferSize = 16 * 1024;
	char* buf = nullptr;
	int nret = 0;
	va_list args;
	do
	{
		buf = new (std::nothrow) char[bufferSize];
		if (buf == nullptr)
			return;
		va_start(args, format);
		nret = vsnprintf(buf, bufferSize - 3, format, args);
		va_end(args);

		if (nret >= 0)
		{
			if (nret <= bufferSize - 3)
				break;
			else
			{
				bufferSize = nret + 3;
				delete[] buf;
			}
		}
		else
		{
			bufferSize *= 2;
			delete[] buf;
		}
	} while (true);
	buf[nret] = '\n';
	buf[++nret] = '\0';

#if OG_TARGET_PLATFORM == OG_PLATFORM_ANDROID
	__android_log_print(ANDROID_LOG_DEBUG, "orange-x debug info", "%s", buf);

#elif OG_TARGET_PLATFORM == OG_PLATFORM_WIN32

	// UTF-8 -> UTF-16LE 用于 OutputDebugStringW
// 	std::string utf16leBuf = iconv_wrapper(std::string(buf, nret), Iconv::Utf8ToUtf16le);
// 	if (!utf16leBuf.empty()) {
// 		std::wstring wstr(reinterpret_cast<const wchar_t*>(utf16leBuf.data()),
// 			utf16leBuf.size() / sizeof(wchar_t));
// 		OutputDebugStringW(wstr.c_str());
// 	}

	// UTF-8 -> GBK 用于 printf
	std::string gbkBuf = iconv_wrapper(std::string(buf, nret), Iconv::Utf8ToGbk);
	printf("%s", gbkBuf.c_str());

	//SendLogToWindow(buf);
	fflush(stdout);
	//#else
		// Linux, Mac, iOS, etc
	fprintf(stdout, "%s", buf);
	fflush(stdout);
#endif

	//Director::getInstance()->getConsole()->log(buf);
	delete[] buf;
}

namespace StringUtils {
	std::string format(const char* format, ...)
	{
#define OG_VSNPRINTF_BUFFER_LENGTH 512
		va_list args;
		std::string buffer(OG_VSNPRINTF_BUFFER_LENGTH, '\0');

		va_start(args, format);
		int nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
		va_end(args);

		if (nret >= 0) {
			if ((unsigned int)nret < buffer.length()) {
				buffer.resize(nret);
			}
			else if ((unsigned int)nret > buffer.length()) { // VS2015/2017 or later Visual Studio Version
				buffer.resize(nret);

				va_start(args, format);
				nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
				va_end(args);

				assert(nret == buffer.length());
			}
			// else equals, do nothing.
		}
		else { // less or equal VS2013 and Unix System glibc implement.
			do {
				buffer.resize(buffer.length() * 3 / 2);

				va_start(args, format);
				nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
				va_end(args);

			} while (nret < 0);

			buffer.resize(nret);
		}

		return buffer;

	}
}
OG_END