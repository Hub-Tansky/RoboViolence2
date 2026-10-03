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

#ifndef BB_SOCKET_COMPAT_H
#define BB_SOCKET_COMPAT_H

#include "platform.h"

#ifndef BV2_PLATFORM_WINDOWS
	#include <errno.h>
	#include <sys/socket.h>
#endif

// send() that retries when a signal interrupts it (POSIX). SIGPIPE is ignored once in bb_init(), so a peer that
// disconnected makes send() fail with EPIPE instead of killing the process.
inline int bbSend(int fd, const char * buf, int len)
{
#ifdef BV2_PLATFORM_WINDOWS
	return send(fd, buf, len, 0);
#else
	int r;
	do
	{
		r = (int)send(fd, buf, (size_t)len, 0);
	} while (r < 0 && errno == EINTR);
	return r;
#endif
}

#endif
