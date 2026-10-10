#ifndef _OG_PLATFORM_MACROS_H_
#define _OG_PLATFORM_MACROS_H_




#define OG orange

#ifdef __cplusplus
#define OG_BEGIN                   namespace orange {
#define OG_END                     }

#define USING_OG                     using namespace orange
#define NS_OG                           ::orange
#else
#define  OG_BEGIN
#define  OG_END
#endif



#if defined(OG_STATIC)
#define OG_DLL
#else
#if defined(_USRDLL)
#define OG_DLL     __declspec(dllexport)
#else         /* use a DLL library */
#define OG_DLL     __declspec(dllimport)
#endif
#endif

#include "iconv_wrapper.h"
#include <stdarg.h>
#include <string>
#include <assert.h>
//#include <windows.h>

OG_BEGIN

#define Instance(CLASS_TYPE)\
	static CLASS_TYPE *getInstance() {\
		static CLASS_TYPE instance_;\
		return &instance_;\
	}


// #define CREATE_FUNC(__TYPE__) \
// static __TYPE__* create() \
// { \
//     __TYPE__ *pRet = new(std::nothrow) __TYPE__(); \
//     if (pRet && pRet->init()) \
//     { \
//         pRet->autorelease(); \
//         return pRet; \
//     } \
//     else \
//     { \
//         delete pRet; \
//         pRet = nullptr; \
//         return nullptr; \
//     } \
// }


//  #define CREATE_FUNC(__TYPE__) \
//  static __TYPE__* create() \
//  { \
//     static  __TYPE__ pRet; \
//      if (&pRet!=nullptr && pRet.init()) \
//      { \
//          return &pRet; \
//      } \
//      else \
//      { \
//          return nullptr; \
//      } \
//  }

#define CREATE_FUNC(__TYPE__) \
 static __TYPE__* create() \
 { \
     __TYPE__ *pRet = new(std::nothrow) __TYPE__(); \
     if (pRet && pRet->init()) \
     { \
		pRet->autorelease(); \
         return pRet; \
     } \
     else \
     { \
		 delete pRet; \
		 pRet = nullptr; \
         return nullptr; \
     } \
 }

#define OG_SAFE_DELETE(p)           do { delete (p); (p) = nullptr; } while(0)
#define OG_SAFE_DELETE_ARRAY(p)     do { if(p) { delete[] (p); (p) = nullptr; } } while(0)
#define OG_SAFE_FREE(p)             do { if(p) { free(p); (p) = nullptr; } } while(0)
#define OG_SAFE_RELEASE(p)          do { if(p) { (p)->release(); } } while(0)
#define OG_SAFE_RELEASE_NULL(p)     do { if(p) { (p)->release(); (p) = nullptr; } } while(0)
#define OG_SAFE_RETAIN(p)           do { if(p) { (p)->retain(); } } while(0)
#define OG_SAFE_NULL(p)           do { if(p) { (p) = nullptr; } } while(0)




#define OG_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete; \
    TypeName &operator =(const TypeName &) = delete;

#define AnimationFrameDisplayedNotification "OGAnimationFrameDisplayedNotification"


//#define OG_CONTENT_SCALE_FACTOR() Director::getInstance()->getContentScaleFactor()


//----------------------------------------------
// #define OG_RECT_PIXELS_TO_POINTS(__rect_in_pixels__)                                                                        \
//     orange::Rect( (__rect_in_pixels__).origin.x / OG_CONTENT_SCALE_FACTOR().x, (__rect_in_pixels__).origin.y / OG_CONTENT_SCALE_FACTOR().y,    \
//             (__rect_in_pixels__).size.width / OG_CONTENT_SCALE_FACTOR().x, (__rect_in_pixels__).size.height / OG_CONTENT_SCALE_FACTOR().y )

/** @def OG_RECT_POINTS_TO_PIXELS
 Converts a rect in points to pixels
 */
 //#define OG_RECT_SCALE(__rect__)                                                                        \
 //    orange::Rect( (__rect__).origin.x * OG_CONTENT_SCALE_FACTOR().x, (__rect__).origin.y * OG_CONTENT_SCALE_FACTOR().y,    \
 //            (__rect__).size.width * OG_CONTENT_SCALE_FACTOR().x, (__rect__).size.height * OG_CONTENT_SCALE_FACTOR().y )

  /** @def OG_POINT_PIXELS_TO_POINTS
   Converts a rect in pixels to points
   */
   // #define OG_POINT_PIXELS_TO_POINTS(__pixels__)                                                                        \
   // orange::Vec2( (__pixels__).x / OG_CONTENT_SCALE_FACTOR().x, (__pixels__).y / OG_CONTENT_SCALE_FACTOR().y)

	 /** @def OG_POINT_POINTS_TO_PIXELS
	  Converts a rect in points to pixels
	  */
	  //#define OG_POSITION_SACALE(__position__)                                                                        \
	  // orange::Vec2( (__position__)  * OG_CONTENT_SCALE_FACTOR() )

		 /** @def OG_POINT_PIXELS_TO_POINTS
		  Converts a rect in pixels to points
		  */
		  // #define OG_SIZE_PIXELS_TO_POINTS(__size_in_pixels__)                                                                        \
		  // orange::Size( (__size_in_pixels__).width / OG_CONTENT_SCALE_FACTOR().x, (__size_in_pixels__).height / OG_CONTENT_SCALE_FACTOR().y)

			  /** @def OG_POINT_POINTS_TO_PIXELS
			   Converts a rect in points to pixels
			   */
			   // #define OG_SIZE_SACALE(__size__)                                                                        \
			   // orange::Size( (__size__) * OG_CONTENT_SCALE_FACTOR())

			   /*
			   #if ((_WIN32)||(_WIN64)) && (__GNUC__)
			   #include <stdlib.h>
			   typedef long long __int64;
			   #endif
			   */

#if !defined _W64
#define _W64
#endif

#if defined(_WIN64)
	typedef __int64 INT_PTR, *PINT_PTR;
typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

typedef __int64 LONG_PTR, *PLONG_PTR;
typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

#define __int3264   __int64

#else
	typedef _W64 int INT_PTR, *PINT_PTR;
typedef _W64 unsigned int UINT_PTR, *PUINT_PTR;

typedef _W64 long LONG_PTR, *PLONG_PTR;
typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;

#define __int3264   __int32

#endif

typedef LONG_PTR SSIZE_T, *PSSIZE_T;

#ifndef __SSIZE_T
#define __SSIZE_T


typedef SSIZE_T ssize_t;
#endif // __SSIZE_T
//添加一个
#ifndef FLT_EPSILON
#define FLT_EPSILON      1.192092896e-07F 
#endif

#ifndef DBL_EPSILON

#define DBL_EPSILON      2.2204460492503131e-016
#endif

#define OG_PLATFORM_WIN32   1
#define OG_PLATFORM_ANDROID 2
#define OG_PLATFORM_IOS     3
#define OG_PLATFORM_MAC     4
#define OG_PLATFORM_LINUX   5

#if defined(_WIN32)
#define OG_TARGET_PLATFORM OG_PLATFORM_WIN32
#elif defined(__ANDROID__) 
#define OG_TARGET_PLATFORM OG_PLATFORM_ANDROID
#elif defined(__APPLE__)
	// 再判断是 iOS 还是 macOS
#else 
#define OG_TARGET_PLATFORM OG_PLATFORM_LINUX
#endif
 
#ifndef OGASSERT
	#if ORANGE_DEBUG > 0
	//#if OG_ENABLE_SCRIPT_BINDING
	//extern bool   og_assert_script_compatible(const char *msg);
	#define OGASSERT(cond, msg) do {                              \
				if (!(cond)) {                                          \
				if (/*!og_assert_script_compatible(msg) && */strlen(msg)) \
					orange::log("Assert failed: %s", msg);             \
				OG_ASSERT(cond);                                      \
				} \
			} while (0)
	#else
	#define OGASSERT(cond, msg) OG_ASSERT(cond)
	#endif
#else
#define OGASSERT(cond, msg)
#endif

 

#if OG_DISABLE_ASSERT > 0
#define OG_ASSERT(cond)
#else
#define OG_ASSERT(cond)    assert(cond)
#endif

extern void log(const char * format, ...);
namespace StringUtils {
	extern std::string format(const char* format, ...);
}

#define __OGLOGWITHFUNCTION(s, ...) \
    orange::log("%s : %s",__FUNCTION__, orange::StringUtils::format(s, ##__VA_ARGS__).c_str())

#if !defined(ORANGE_DEBUG) || ORANGE_DEBUG == 0
#define OGLOG(...)       do {} while (0)
#define OGLOGINFO(...)   do {} while (0)
#define OGLOGERROR(...)  do {} while (0)
#define OGLOGWARN(...)   do {} while (0)

#elif ORANGE_DEBUG == 1
#define OGLOG(format, ...)      orange::log(format, ##__VA_ARGS__)
#define OGLOGERROR(format,...)  orange::log(format, ##__VA_ARGS__)
#define OGLOGINFO(format,...)   do {} while (0)
#define OGLOGWARN(...) __OGLOGWITHFUNCTION(__VA_ARGS__)

#elif ORANGE_DEBUG > 1
#define OGLOG(format, ...)      orange::log(format, ##__VA_ARGS__)
#define OGLOGERROR(format,...)  orange::log(format, ##__VA_ARGS__)
#define OGLOGINFO(format,...)   orange::log(format, ##__VA_ARGS__)
#define OGLOGWARN(...) __OGLOGWITHFUNCTION(__VA_ARGS__)
#endif // ORANGE_DEBUG

#define OG_CALLBACK_0(__selector__,__target__, ...) std::bind(&__selector__,__target__, ##__VA_ARGS__)
#define OG_CALLBACK_1(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, ##__VA_ARGS__)
#define OG_CALLBACK_2(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define OG_CALLBACK_3(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)


OG_END


#endif