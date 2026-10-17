// Language files (Phase B step 13, ADR 0011): original .lang files and gettext PO, English fallback per key, and the
// OS language picked when languageFile is empty. Texts come out in the font's byte encoding (CFont glyphs 32-159).
#pragma once
#include <map>
#include <string>
#include <vector>

namespace lang
{
	typedef std::map<std::string, std::string> Texts; // key (lang_*) -> text

	// .lang: "key<TAB>text" lines up to an "END" line; Latin-1 accented letters map to the font's 128-159
	bool readLang(const std::string & path, Texts & out);
	// PO: msgctxt is the key, msgstr the UTF-8 text, applied over out (English). Untranslated entries, keys out
	// lacks and texts whose printf conversions differ from out's are skipped. allGlyphs (optional) is set to false
	// when a translation uses a character the font lacks, which shows as '?'
	bool parsePo(const std::string & text, Texts & out, bool * allGlyphs);
	bool readPo(const std::string & path, Texts & out, bool * allGlyphs);
	// The file for the OS locales ("ll_CC" or "ll", most preferred first): <dir>ll_CC, then <dir>ll, .po before .lang,
	// each name in every dir in order (the package's, then the player's data), skipping a PO the font can't render
	// (parsed here and again by load: one small file, once per start); "" (English) when none exists.
	// dirs[0] holds en.lang.
	std::string pickFile(const std::vector<std::string> & dirs, const std::vector<std::string> & locales);
	// The saved languageFile to use: "" (the OS language) for "main/languages/en.lang", the value builds before
	// Phase B step 13 saved as their default, so upgraded configs follow the OS (PNS-47)
	std::string savedFile(const std::string & languageFile);
	// <dir>en.lang, then file ("" for none) on top of it, so a key the file lacks stays English
	bool load(const std::string & dir, const std::string & file, Texts & out);
}
