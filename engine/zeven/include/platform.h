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

/* The wire and file formats are little endian and read by memcpy. */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
	#error "big-endian targets are not supported"
#endif

#ifdef __cplusplus
	// The game relied on windows.h's min/max macros; NOMINMAX is set, so provide them as functions everywhere.
	#include <algorithm>
	using std::min;
	using std::max;

	#include <cstdio>
	#include <cstring>
	// Reads n bytes. A truncated file leaves the rest zeroed instead of uninitialised, and returns false.
	inline bool bv2ReadBytes(FILE * f, void * dst, size_t n)
	{
		size_t got = fread(dst, 1, n, f);
		if (got < n) memset(static_cast<char *>(dst) + got, 0, n - got);
		return got == n;
	}
#endif

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

#endif /* BV2_POSIX && __cplusplus */

#endif
