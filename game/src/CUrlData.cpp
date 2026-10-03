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
#include <sstream>

// RFC 3986 percent-encoding, as curl_easy_escape does (unreserved characters stay as they are)
static std::string urlEscape(const std::string& in)
{
	static const char hex[] = "0123456789ABCDEF";
	std::string out;
	for (size_t i = 0; i < in.size(); ++i)
	{
		unsigned char c = (unsigned char)in[i];
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~')
			out += (char)c;
		else
		{
			out += '%';
			out += hex[c >> 4];
			out += hex[c & 15];
		}
	}
	return out;
}

// For sending xml via postdata
std::string base64_encode(std::string in);

CUrlData::CUrlData() { m_data = ""; }
CUrlData::~CUrlData() {}

void CUrlData::add(CString key, std::string value, int flags /* = CUrlData::NONE */)
{
	if(m_data.size() > 0)
		m_data += "&";

	std::stringstream ss;
	ss << key.s << "=";

	if(flags == MD5)
	{
		CMD5 md5(value.c_str());
		ss << md5.getMD5Digest();
	}
	else if(flags == BASE64)
	{
		console->add("\x2>Base64 Encoding Data", true);
		std::string b64 = base64_encode(value);
		console->add("\x2>Escaping Data", true);
		ss << urlEscape(b64);
	}
	else
	{
		ss << value;
	}
	
	m_data += ss.str();
}

/*void CUrlData::add(CString key, CString value, int flags /* = CUrlData::NONE *///)
/*{
	if(m_data.size() > 0)
		m_data += "&";

	if(flags == CUrlData::MD5)
	{
		CMD5 md5(value.s);
		m_data += CString("%s=%s", key.s, md5.getMD5Digest()).s;
	}
	else
	{
		m_data += CString("%s=%s", key.s, value.s).s;
	}
	
}*/

void CUrlData::add(CString key, int value, int flags /* = CUrlData::NONE */)
{
	add(key, CString("%i", value).s, flags);
}

void CUrlData::add(CString key, float value, int flags /* = CUrlData::NONE */)
{
	add(key, CString("%f", value).s, flags);
}


// For base64 encoding
static const char base64_table[] =
{ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'
};

static const char base64_pad = '=';

std::string base64_encode(std::string in)
{
	std::string r;
	std::size_t pos;
	typedef unsigned char uchar;

	// keep going until we have less than 24 bits
	for(pos = 0; in.size() - pos > 2; pos+=3)
	{
		r += base64_table[ uchar(in[pos+0]) >> 2];
		r += base64_table[(( uchar(in[pos+0]) & 0x03) << 4) + ( uchar(in[pos+1]) >> 4)];
		r += base64_table[(( uchar(in[pos+1]) & 0x0f) << 2) + ( uchar(in[pos+2]) >> 6)];
		r += base64_table[ uchar(in[pos+2]) & 0x3f];
	}

	if(in.size() - pos != 0)
	{
		r += base64_table[uchar(in[pos+0]) >> 2];
		
		if (in.size() - pos > 1)
		{
			r += base64_table[(( uchar(in[pos+0]) & 0x03) << 4) + (uchar(in[pos+1]) >> 4)];
			r += base64_table[(uchar(in[pos+1]) & 0x0f) << 2];
			r += base64_pad;
		}
		else
		{
			r += base64_table[(uchar(in[pos+0]) & 0x03) << 4];
			r += base64_pad;
			r += base64_pad;
		}

	}

	return r;
}

