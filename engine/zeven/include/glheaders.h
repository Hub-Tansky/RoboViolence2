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

#ifndef BV2_GLHEADERS_H
#define BV2_GLHEADERS_H

// The only place that includes OpenGL headers. GL 2.1 (compatibility) entry points come from glad
// (engine/zeven/third_party/glad); GLU stays on the system library until the renderer is replaced.
#include "platform.h"

#ifdef BV2_PLATFORM_WINDOWS
	#include <windows.h>
#endif

// glad must come first: it refuses to follow the system gl.h, and glu.h skips gl.h once glad defined __gl_h_.
#include <glad/gl.h>

#ifdef BV2_PLATFORM_MACOS
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif

#endif
