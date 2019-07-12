// add your includes to this file instead of to individual .cpp files
// to enjoy the benefits of precompiled headers:
// - fast compilation
// - solve issues with the order of header files once (here)
// do not include headers in header files (ever).
#include <math.h>

#define SCRWIDTH		800
#define SCRHEIGHT		800
#define NLAYERS			3 // 1 = L1 cache, 3 = L1-L2-L3 cache
#define POLICY			1 // 0 = RandomReplacement, 1 = TreePseudoLRU, 2 = BitPseudoLRU, 3 = FirstInFirstOut
#define DRAWPERF		1 // 1 = draw hit ratio over time on screen, 0 = draw no hit ratio on screen

const unsigned int UINT_MAX = -1;

// #define FULLSCREEN
// #define ADVANCEDGL	// faster if your system supports it

#include <inttypes.h>
extern "C" 
{ 
#include "glew.h" 
}
#include "gl.h"
#include "io.h"
#include <fstream>
#include <stdio.h>
#include "fcntl.h"
#include "SDL.h"
#include "wglext.h"
#include "freeimage.h"
#include "math.h"
#include "stdlib.h"
#include "emmintrin.h"
#include "immintrin.h"
#include "windows.h"
#include "template.h"
#include "surface.h"
#include "threads.h"
#include <assert.h>

using namespace std;
using namespace Tmpl8;

#include "cache.h"
#include "game.h"