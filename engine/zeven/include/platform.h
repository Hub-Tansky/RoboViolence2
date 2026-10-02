/*
	Copyright 2012 bitHeads inc.

	This file is part of the BaboViolent 2 source code.

	The BaboViolent 2 source code is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by the 
	Free Software Foundation, either version 3 of the License, or (at your option) 
	any later version.

	The BaboViolent 2 source code is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with the 
	BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

#ifndef BV2_PLATFORM_H
#define BV2_PLATFORM_H

/* Platform detection. Force-included into every translation unit by CMake. */
#if defined(_WIN32)
	#define BV2_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
	#define BV2_PLATFORM_MACOS 1
	#define BV2_POSIX 1
#elif defined(__linux__)
	#define BV2_PLATFORM_LINUX 1
	#define BV2_POSIX 1
#else
	#error "Unsupported platform"
#endif

#include <stdint.h>

/* 32-bit wire types (the old code used long on 32-bit and int on LP64). */
typedef int32_t INT4;
typedef uint32_t UINT4;

#if defined(BV2_POSIX) && defined(__cplusplus)

#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <pthread.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <errno.h>
#include <time.h>
#include <ctime>

// --- Floyd Davidson's macro ---
#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])
#define IFRSIZE   ((int)(size * sizeof (struct ifreq)))

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define amax(X,Y) ((X) < (Y) ? (Y) : (X))

/* Win32 type shims used by the window, input and GL code until Step 3 replaces it. */
	typedef void * LPVOID;
	typedef unsigned long HRESULT;

	#define APIENTRY


	#define SUCCEEDED(o) ((o) == 0)
	#define FAILED(o) ((o) != 0)

	typedef unsigned long HWND;
	typedef unsigned long HINSTANCE;
	typedef bool BOOL;
	typedef unsigned long HDC;
	typedef unsigned long HGLRC;

	#define TRUE 1
	#define FALSE 0


	typedef unsigned long LRESULT;
	typedef unsigned int UINT;
	typedef unsigned short WPARAM;
	typedef unsigned long LPARAM;


	struct POINT
	{
	  long x;
	  long y;

	  inline POINT() : x(0), y(0)
	  {
	  }

	  inline POINT(long _x, long _y) : x(_x), y(_y)
	  {
	  }

	  inline POINT(const POINT & rp)
	  {
		x = rp.x;
		y = rp.y;
	  }
	};


	typedef POINT * LPPOINT;

	struct RECT
	{
	  long left;
	  long top;
	  long right;
	  long bottom;

	  inline RECT() : left(0), top(0), right(0), bottom(0)
	  {
	  }

	  inline RECT(long l, long t, long r, long b) : left(l), top(t), right(r), bottom(b)
	  {
	  }
	  inline RECT(const RECT & rr)
	  {
		left = rr.left;
		top = rr.top;
		right = rr.right;
		bottom = rr.bottom;
	  }
	};

	typedef RECT * LPRECT;

	struct DIMOUSESTATE2
	{
	  long lX;
	  long lY;
	  long lZ;
	  unsigned char rgbButtons[8];
  
	};

	typedef  DIMOUSESTATE2 * LPDIMOUSESTATE2;

#endif /* BV2_POSIX && __cplusplus */

#endif
