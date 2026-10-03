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

// Stand-in for CCurl when BV2_WITH_HTTP is OFF (ADR 0002): same API, every request finishes at once with
// 0 bytes received. The account, friends, ladder and report-upload backends are gone.
#include "CCurl.h"

const int CCurl::s_maxResponse = 65536;

CCurl::CCurl(CString url, std::string data): m_data(data), m_url(url), m_recieved(0), buffer(0)
{
}

CCurl::~CCurl()
{
}

void CCurl::execute(void* pArg)
{
}
