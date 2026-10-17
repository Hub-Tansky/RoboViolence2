#include "Lang.h"

#include <cctype>
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
				if ((unsigned char)c < 32)
					return false; // raw control bytes: 1-9 are colour codes
				if (c == '"')
					return false;
				if (c == '\\')
				{
					if (++i + 1 >= s.size())
						return false;
					c = s[i];
					if (c == 'n') c = '\n';
					else if (c == 'x' && i + 2 < s.size() && s[i + 1] >= '1' && s[i + 1] <= '9') c = (char)(s[++i] - '0');
					else if (c != '"' && c != '\\') return false;
				}
				out += c;
			}
			return true;
		}

		// The printf conversions in a text, "%%" left out; the game formats lang_* texts with them
		std::string formatSpecs(const std::string & s)
		{
			std::string specs;
			for (size_t i = 0; i < s.size(); ++i)
			{
				if (s[i] != '%') continue;
				if (i + 1 < s.size() && s[i + 1] == '%') { ++i; continue; }
				size_t j = i + 1;
				while (j < s.size() && s[j] && strchr("-+ #0123456789.hlLqjzt", s[j])) ++j;
				if (j < s.size() && isalpha((unsigned char)s[j])) ++j;
				specs += s.substr(i, j - i) + ' ';
				i = j - 1;
			}
			return specs;
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
		std::string field[3]; // msgctxt (the key), msgid (English), msgstr
		bool seen[3] = {false, false, false};
		int cur = -1;
		bool ok = true;
		auto flush = [&]()
		{
			// Only keys out already has (English), and only with the same printf conversions: the game formats the
			// text with the English arguments
			Texts::iterator en = out.find(field[0]);
			if (en != out.end() && !field[2].empty() && formatSpecs(en->second) == formatSpecs(field[2]))
				en->second = utf8ToFont(field[2], ok);
			for (int k = 0; k < 3; ++k)
			{
				field[k].clear();
				seen[k] = false;
			}
			cur = -1;
		};
		std::istringstream in(text);
		std::string line;
		while (std::getline(in, line))
		{
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			size_t start = line.find_first_not_of(" \t");
			if (start == std::string::npos || line[start] == '#')
				continue;
			line = line.substr(start);
			if (line[0] == '"')
			{
				if (cur < 0 || !unquote(line, field[cur])) return false;
				continue;
			}
			size_t space = line.find(' ');
			std::string keyword = line.substr(0, space);
			int k = keyword == "msgctxt" ? 0 : keyword == "msgid" ? 1 : keyword == "msgstr" ? 2 : -1;
			if (k < 0 || space == std::string::npos)
				return false;
			if (k < 2 && seen[2])
				flush(); // msgctxt or msgid after a msgstr starts the next entry
			if (seen[k])
				return false; // a keyword twice in one entry
			seen[k] = true;
			cur = k;
			if (!unquote(line.substr(space + 1), field[k]))
				return false;
		}
		flush();
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

	std::string pickFile(const std::vector<std::string> & dirs, const std::vector<std::string> & locales)
	{
		Texts english;
		readLang(dirs[0] + "en.lang", english);
		for (size_t i = 0; i < locales.size(); ++i)
		{
			std::vector<std::string> names(1, locales[i]);
			size_t us = locales[i].find('_');
			if (us != std::string::npos)
				names.push_back(locales[i].substr(0, us));
			for (size_t n = 0; n < names.size(); ++n)
				for (size_t d = 0; d < dirs.size(); ++d)
				{
					Texts texts = english;
					bool glyphs = false;
					if (readPo(dirs[d] + names[n] + ".po", texts, &glyphs) && glyphs)
						return dirs[d] + names[n] + ".po";
					if (exists(dirs[d] + names[n] + ".lang"))
						return dirs[d] + names[n] + ".lang";
				}
		}
		return "";
	}

	std::string savedFile(const std::string & languageFile)
	{
		return languageFile == "main/languages/en.lang" ? "" : languageFile;
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
