#ifndef _CONVERT_ENCODING_H_
#define _CONVERT_ENCODING_H_

#include <stdlib.h>
#include <unordered_map>
#include <functional>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>

#define LIBICONV_CONV_BEGIN namespace LIBICONV_CONV{
#define LIBICONV_CONV_END }

LIBICONV_CONV_BEGIN

/* ---------------- GB2312 tables ---------------- */
// extern const unsigned short gb2312_2charset[7445];
// extern const Summary16 gb2312_uni2indx_page00[70];
// extern const Summary16 gb2312_uni2indx_page20[101];
// extern const Summary16 gb2312_uni2indx_page30[35];
// extern const Summary16 gb2312_uni2indx_page4e[1263];
// extern const Summary16 gb2312_uni2indx_page9e[27];
// extern const Summary16 gb2312_uni2indx_pageff[15];
//
// extern const unsigned short gb2312_2uni_page21[831];
// extern const unsigned short gb2312_2uni_page30[6768];
//
// /* ---------------- GBKEXT tables ---------------- */
// extern const unsigned short gbkext_inv_2charset[14313];
//
// extern const Summary16 gbkext_inv_uni2indx_page02[14];
// extern const Summary16 gbkext_inv_uni2indx_page20[44];
// extern const Summary16 gbkext_inv_uni2indx_page25[17];
// extern const Summary16 gbkext_inv_uni2indx_page30[16];
// extern const Summary16 gbkext_inv_uni2indx_page32[30];
// extern const Summary16 gbkext_inv_uni2indx_page4e[1307];
// extern const Summary16 gbkext_inv_uni2indx_pagef9[19];
// extern const Summary16 gbkext_inv_uni2indx_pagefe[31];
//
// extern const unsigned short gbkext1_2uni_page81[6080];
// extern const unsigned short gbkext2_2uni_pagea8[8272];
//
// /* ---------------- CP936EXT tables ---------------- */
// extern const unsigned short cp936ext_page01[16];
// extern const unsigned short cp936ext_page02[24];
// extern const unsigned short cp936ext_pagefe[24];
// extern const unsigned short cp936ext_2uni_pagea6[181 - 159];
// extern const unsigned short cp936ext_2uni_pagea8[128 - 122];

/* ---------------- GB2312 / GBK / CP936 conversion functions ---------------- */
extern int gb2312_wctomb(unsigned char *r, unsigned int wc, size_t n);
extern int gb2312_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);

extern int gbkext_inv_wctomb(unsigned char *r, unsigned wc, size_t n);
extern int gbkext1_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);
extern int gbkext2_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);

extern int cp936ext_wctomb(unsigned char *r, unsigned wc, size_t n);
extern int cp936ext_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);

extern int ascii_wctomb(unsigned char *r, unsigned int wc);
extern int ascii_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);

extern int gbk_wctomb(unsigned char *r, unsigned wc, size_t n);
extern int gbk_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);

extern int ces_gbk_wctomb(unsigned char *r, unsigned int *s, size_t n);
extern int ces_gbk_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);

/* ---------------- UTF-8 / UTF-16LE / UTF-32LE ---------------- */
/* Note: the parameter order of utf8_mbtowc matches the CPP definition */
extern int utf8_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);
extern int utf8_wctomb(unsigned char *r, unsigned wc, size_t n);

extern int utf16le_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);
extern int utf16le_wctomb(unsigned char *r, unsigned wc, size_t n);

extern int utf32le_mbtowc(unsigned *pwc, const unsigned char *s, size_t n);
extern int utf32le_wctomb(unsigned char *r, unsigned wc, size_t n);

/* ---------------- Unified conversion interface functions ---------------- */

extern int ces_gbk_mbtowc_conv(unsigned char *s, unsigned *pwc, size_t n);
extern int utf8_mbtowc_conv(unsigned char *s, unsigned *pwc, size_t n);
extern int utf8_wctomb_conv(unsigned char *s, unsigned *pwc, size_t n);

extern int utf16le_mbtowc_conv(unsigned char *s, unsigned *pwc, size_t n);
extern int utf16le_wctomb_conv(unsigned char *s, unsigned *pwc, size_t n);

extern int utf32le_mbtowc_conv(unsigned char *s, unsigned *pwc, size_t n);
extern int utf32le_wctomb_conv(unsigned char *s, unsigned *pwc, size_t n);

LIBICONV_CONV_END

/* ---------------- Public main call interface ---------------- */

enum Iconv : int {

	GbkToUtf8,
	Utf8ToGbk,

	GbkToUtf16le,
	Utf16leToGbk,

	GbkToUtf32le,
	Utf32leToGbk,

	Utf8ToUtf16le,
	Utf16leToUtf8,

	Utf8ToUtf32le,
	Utf32leToUtf8,

	Utf16leToUtf32le,
	Utf32leToUtf16le
};

// extern std::unordered_map<
//     Iconv,
//     std::vector<std::function<int(unsigned char *, unsigned int *, size_t)>>
// > auto_func;

extern int convert_encoding(
	Iconv token,
	const char* src,
	int inSize,
	char **dest,
	int outSize
);

extern std::string iconv_wrapper(const std::string& inbuf, Iconv token);
extern std::u32string iconv_wrapper_to_utf32le(const std::string& inbuf, Iconv token);
extern std::string   iconv_wrapper_from_u32le(const std::u32string& inbuf, Iconv token);
extern std::u16string   iconv_wrapper_from_u32le_to_utf16le(const std::u32string& inbuf, Iconv token);
#endif