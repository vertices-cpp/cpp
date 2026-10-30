#ifndef _HEADER_H_
#define _HEADER_H_

#if defined(_WIN32)||defined(_WIN64)

//	#include "SDL.h"
//	#include "SDL_image.h"
//	#include "SDL_ttf.h"
//	#include "SDL_mixer.h"

#elif defined(__ANDROID__)
 
   
	#include "SDL2/SDL.h"
//	#include "SDL2/SDL_image.h"
	#include "SDL2/SDL_ttf.h"
	#include "SDL2/SDL_mixer.h"

#endif

#include <cassert>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>
#include <memory>
#include <iomanip>
#include <set>
#include <mutex>
//#include <stdlib.h>


#include <iostream>



#endif	//_HEADER_H_