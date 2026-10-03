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

#include "CCurl.h"
#include "Console.h"
#include "md5class.h"
using std::min;
using std::max;

// Could be changed to dynamically allocate
// but for now this is enough for 40 friends
// with full info
// changed to 65536 since it's used for other things that require bigger buffer (cnik)
const int CCurl::s_maxResponse = 65536;




CCurl::CCurl(CString url, std::string data): m_handle(0), m_data(data), m_url(url), m_recieved(0), buffer(0)
{
}


CCurl::~CCurl()
{
	if (m_handle != 0)
	{
		curl_easy_cleanup(m_handle);
	}
	if (buffer) delete[] buffer;
}

void CCurl::execute(void* pArg)
{
	// First, create a handle
	m_handle = curl_easy_init();

	// Ensure handle was created and response string provided
	if(m_handle == 0)
		return;

	//--- Timeout in seconds for connection
	curl_easy_setopt(m_handle, CURLOPT_CONNECTTIMEOUT, 3);

	//--- Timeout for whole connection+transfer, thus must be > connect timeout
	curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, 5);

	// Create response buffer
	CCurl::SResponse r = {"", 0};
//	char* buffer = new char[s_maxResponse+1];
	if (buffer) delete [] buffer;
	buffer = new char[s_maxResponse+1];
	r.data = buffer;

	// Set curl settings
	curl_easy_setopt(m_handle, CURLOPT_URL, m_url.s);
	curl_easy_setopt(m_handle, CURLOPT_NOSIGNAL, true);
	curl_easy_setopt(m_handle, CURLOPT_POSTFIELDS, m_data.c_str());
	curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, CCurl::write_data);
	curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &r); 

	// Send to server
	curl_easy_perform(m_handle);

	// Check response size
	m_recieved = (int)(r.size);
	if(m_recieved > 0)
	{
		buffer[m_recieved] = '\0';
		m_response = buffer;
	}

	// Clean up
	if (buffer) delete[] buffer;
	buffer = 0;

	return;
}

size_t CCurl::write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	if(userp != 0)
	{
		SResponse& r = *((SResponse*)userp);
		size_t can_handle = max(min(static_cast<int>(CCurl::s_maxResponse-r.size), static_cast<int>(size*nmemb)),0);
		memcpy(r.data + r.size, buffer, can_handle);
		r.size += can_handle;
		return can_handle;
	}
	else
		return 0;
}
