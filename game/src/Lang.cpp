#include "Lang.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace lang
{
	namespace
	{
		// The Latin-1 letters CFont draws at 128 + index (the original BV2 font)
		const int LATIN1[32] = {199, 252, 233, 226, 228, 224, 229, 231, 234, 235, 232, 239, 238, 236, 196, 197, 201, 230,
			198, 244, 246, 242, 251, 249, 255, 214, 220, 248, 163, 216, 215, 170};

		// Font byte for a code point, or -1: ASCII, the colour codes \x1-\x9, newline and the letters above
		int fontByte(unsigned int c)
		{
			if ((c >= 32 && c < 128) || (c >= 1 && c <= 9) || c == '\n')
				return (int)c;
			for (int i = 0; i < 32; ++i)
				if ((unsigned int)LATIN1[i] == c)
					return 128 + i;
			return -1;
		}

		// UTF-8 to font bytes; a character without a glyph (or a bad byte) becomes '?' and clears ok
		std::string utf8ToFont(const std::string & s, bool & ok)
		{
			std::string out;
			for (size_t i = 0; i < s.size();)
			{
				unsigned char b = (unsigned char)s[i];
				int len = b < 0x80 ? 1 : (b >> 5) == 6 ? 2 : (b >> 4) == 14 ? 3 : (b >> 3) == 30 ? 4 : 0;
				unsigned int c = len == 1 ? b : len == 2 ? (b & 0x1fu) : len == 3 ? (b & 0x0fu) : (b & 0x07u);
				for (int k = 1; k < len && i + k < s.size(); ++k)
					c = (c << 6) | ((unsigned char)s[i + k] & 0x3fu);
				int f = len && i + len <= s.size() ? fontByte(c) : -1;
				if (f < 0)
					ok = false;
				out += f < 0 ? '?' : (char)f;
				i += len ? len : 1;
			}
			return out;
		}

		// Appends a quoted PO string's content to out; false if it isn't one
		bool unquote(const std::string & s, std::string & out)
		{
			if (s.size() < 2 || s[0] != '"' || s[s.size() - 1] != '"')
				return false;
			for (size_t i = 1; i + 1 < s.size(); ++i)
			{
				char c = s[i];
				if (c == '"')
					return false;
				if (c == '\\')
				{
					if (++i + 1 >= s.size())
						return false;
					c = s[i];
					if (c == 'n') c = '\n';
					else if (c == 't') c = '\t';
					else if (c == 'x' && i + 2 < s.size() && s[i + 1] >= '1' && s[i + 1] <= '9') c = (char)(s[++i] - '0');
					else if (c != '"' && c != '\\') return false;
				}
				out += c;
			}
			return true;
		}

		bool exists(const std::string & path)
		{
			std::ifstream f(path.c_str());
			return f.good();
		}

		bool endsWith(const std::string & s, const char * end)
		{
			size_t n = strlen(end);
			return s.size() >= n && s.compare(s.size() - n, n, end) == 0;
		}
	}

	bool readLang(const std::string & path, Texts & out)
	{
		FILE * fic = fopen(path.c_str(), "r");
		if (!fic) return false;

		char varName[256];
		char varValue[512];
		while (true)
		{
			if (fscanf(fic, "%255s", varName) != 1) { fclose(fic); return false; } // no END line
			if (strcmp(varName, "END") == 0)
			{
				fclose(fic);
				return true;
			}
			for (int i = 0;;)
			{
				int character = fgetc(fic);
				for (int k = 0; k < 32; ++k)
					if (character == LATIN1[k]) { character = 128 + k; break; }
				if (character == '\t')
					continue;
				if (character == '\n' || character == EOF || i >= 510) // end of line, end of file or buffer full
				{
					varValue[i] = '\0';
					break;
				}
				if (character == '\\')
				{
					character = fgetc(fic);
					switch (character)
					{
					case 'x':
						character = fgetc(fic);
						if (character >= '1' && character <= '9') character -= '0';
						break;
					case 'n':
						character = '\n';
						break;
					}
				}
				varValue[i] = (char)character;
				++i;
			}
			out[varName] = varValue;
		}
	}

	bool parsePo(const std::string & text, Texts & out, bool * allGlyphs)
	{
		std::string ctxt, str, id;
		std::string * cur = 0;
		bool haveStr = false, ok = true;
		std::istringstream in(text);
		std::string line;
		while (true)
		{
			bool more = (bool)std::getline(in, line);
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			size_t start = line.find_first_not_of(" \t");
			line = more && start != std::string::npos ? line.substr(start) : "";
			std::string keyword = line.substr(0, line.find(' '));
			bool next = !more || keyword == "msgctxt" || (keyword == "msgid" && haveStr);
			if (next && haveStr && !ctxt.empty() && !str.empty())
				out[ctxt] = utf8ToFont(str, ok);
			if (next)
			{
				ctxt.clear(); id.clear(); str.clear();
				haveStr = false;
				cur = 0;
			}
			if (!more)
				break;
			if (line.empty() || line[0] == '#')
				continue;
			if (line[0] == '"')
			{
				if (!cur || !unquote(line, *cur)) return false;
				continue;
			}
			cur = keyword == "msgctxt" ? &ctxt : keyword == "msgid" ? &id : keyword == "msgstr" ? &str : 0;
			if (!cur || line.size() <= keyword.size() || !unquote(line.substr(keyword.size() + 1), *cur))
				return false;
			haveStr = haveStr || cur == &str;
		}
		if (allGlyphs) *allGlyphs = ok;
		return true;
	}

	bool readPo(const std::string & path, Texts & out, bool * allGlyphs)
	{
		std::ifstream f(path.c_str(), std::ios::binary);
		if (!f) return false;
		std::stringstream text;
		text << f.rdbuf();
		return parsePo(text.str(), out, allGlyphs);
	}

	std::string pickFile(const std::string & dir, const std::vector<std::string> & locales)
	{
		for (size_t i = 0; i < locales.size(); ++i)
		{
			std::vector<std::string> names(1, locales[i]);
			size_t us = locales[i].find('_');
			if (us != std::string::npos)
				names.push_back(locales[i].substr(0, us));
			for (size_t n = 0; n < names.size(); ++n)
			{
				Texts texts;
				bool glyphs = false;
				if (readPo(dir + names[n] + ".po", texts, &glyphs) && glyphs)
					return dir + names[n] + ".po";
				if (exists(dir + names[n] + ".lang"))
					return dir + names[n] + ".lang";
			}
		}
		return "";
	}

	bool load(const std::string & dir, const std::string & file, Texts & out)
	{
		if (!readLang(dir + "en.lang", out))
			return false;
		if (file.empty())
			return true;
		return endsWith(file, ".po") ? readPo(file, out, 0) : readLang(file, out);
	}
}
