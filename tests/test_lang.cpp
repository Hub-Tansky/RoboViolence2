// Phase B step 13: the PO parser, English fallback per key, OS locale matching and the glyph skip (game/src/Lang.cpp).
#include "Lang.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

static void writeFile(const std::string & path, const std::string & text)
{
	std::ofstream(path.c_str(), std::ios::binary) << text;
}

int main(int argc, char ** argv)
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };
	if (argc < 2) { std::printf("usage: test_lang <scratch dir>\n"); return 2; }
	std::string dir = std::string(argv[1]) + "/lang-";

	// PO parser over English texts: header, comments, escapes, multi-line strings, untranslated entries, entries
	// without msgctxt, keys English lacks
	lang::Texts t = {{"lang_a", "A %s"}, {"lang_b", "B"}, {"lang_e", "E"}};
	bool glyphs = false;
	check(lang::parsePo("msgid \"\"\nmsgstr \"Content-Type: text/plain; charset=UTF-8\\n\"\n\n"
		"# comment\nmsgctxt \"lang_a\"\nmsgid \"A %s\"\nmsgstr \"\\x4Zw\\\"x\\\\y\\n\"\n\"%s cd\"\n\n"
		"msgctxt \"lang_b\"\nmsgid \"B\"\nmsgstr \"\"\n\nmsgid \"C\"\nmsgstr \"c\"\n"
		"msgctxt \"lang_x\"\nmsgid \"X\"\nmsgstr \"x\"\n"
		"msgctxt \"lang_e\"\r\nmsgid \"E\"\r\nmsgstr \"\xc3\xa9t\xc3\xa9\"\r\n", t, &glyphs), "PO parses");
	check(t["lang_a"] == "\x04Zw\"x\\y\n%s cd", "escapes and continuation lines");
	check(t["lang_b"] == "B", "untranslated entry keeps English");
	check(t.size() == 3 && t["lang_e"] == "\x82t\x82", "no msgctxt and unknown key skipped; CRLF; e-acute is font byte 130");
	check(glyphs, "Latin-1 letters have glyphs");
	check(!lang::parsePo("msgctxt \"a\"\nmsgid \"A\"\nmsgstr \"unterminated\n", t, 0), "bad quote rejected");
	check(!lang::parsePo("msgctxt \"a\"\nmsgstr[0] \"x\"\n", t, 0), "unknown keyword rejected");
	check(!lang::parsePo("\"orphan\"\n", t, 0), "continuation without a keyword rejected");
	check(!lang::parsePo("msgctxt \"a\"\nmsgid \"A\"\nmsgstr \"x\"\nmsgstr \"y\"\n", t, 0), "repeated keyword rejected");
	check(!lang::parsePo("msgctxt \"a\"\nmsgid \"A\"\nmsgstr \"x\\ty\"\n", t, 0), "\\t escape rejected (byte 9 is a colour code)");
	check(!lang::parsePo("msgctxt \"a\"\nmsgid \"A\"\nmsgstr \"x\ty\"\n", t, 0), "raw tab rejected");
	check(!lang::parsePo("msgctxt \"a\"\nmsgid \"A\"\nmsgstr \"x\x02y\"\n", t, 0), "raw colour byte rejected");

	// printf conversions: a translation must keep the English text's (not its own msgid's), or English stays
	lang::Texts f = {{"ok", "Spawn in %i:%02i, 100%%"}, {"type", "%s joined"}, {"extra", "%s joined"},
		{"n", "Quit"}, {"lone", "Quit"}, {"msgid", "%s joined"}};
	check(lang::parsePo("msgctxt \"ok\"\nmsgid \"Spawn in %i:%02i, 100%%\"\nmsgstr \"Za %i:%02i, 100%%\"\n"
		"msgctxt \"type\"\nmsgid \"%s joined\"\nmsgstr \"%i dolaczyl\"\n"
		"msgctxt \"extra\"\nmsgid \"%s joined\"\nmsgstr \"%s %s dolaczyl\"\n"
		"msgctxt \"n\"\nmsgid \"Quit\"\nmsgstr \"%n\"\n"
		"msgctxt \"lone\"\nmsgid \"Quit\"\nmsgstr \"Wyjdz %\"\n"
		"msgctxt \"msgid\"\nmsgid \"%i joined\"\nmsgstr \"%i dolaczyl\"\n", f, 0), "PO with bad conversions parses");
	check(f["ok"] == "Za %i:%02i, 100%%", "same conversions: translated");
	check(f["type"] == "%s joined" && f["extra"] == "%s joined" && f["n"] == "Quit" && f["lone"] == "Quit",
		"changed, extra, %n or lone % conversions: English stays");
	check(f["msgid"] == "%s joined", "a msgid that agrees with a bad msgstr doesn't count");

	lang::Texts pl = {{"lang_a", "A"}};
	check(lang::parsePo("msgctxt \"lang_a\"\nmsgid \"A\"\nmsgstr \"\xc5\x81\xc3\xb3" "d\xc5\xba \xc3\xa9\"\n", pl, &glyphs) && !glyphs
		&& pl["lang_a"] == "??d? \x82", "letters without glyphs (Polish, o-acute) become '?'");

	// .lang and the English fallback: en.lang first, the chosen file on top
	writeFile(dir + "en.lang", "lang_a\t\t\tApple\nlang_b\t\t\t%s\\x8 bee\\n\xe9\n\nEND\n");
	writeFile(dir + "fr.lang", "lang_a\t\t\tPomme\nEND\n");
	writeFile(dir + "de.po", "msgctxt \"lang_a\"\nmsgid \"Apple\"\nmsgstr \"Apfel\"\n");
	writeFile(dir + "ja.po", "msgctxt \"lang_a\"\nmsgid \"Apple\"\nmsgstr \"\xe3\x82\xa2\"\n");
	writeFile(dir + "pl.po", "msgctxt \"lang_a\"\nmsgid \"Apple\"\nmsgstr \"Jablko\"\n");
	lang::Texts en;
	check(lang::load(dir, "", en) && en["lang_a"] == "Apple" && en["lang_b"] == "%s\x08 bee\n\x82", ".lang escapes and Latin-1");
	lang::Texts fr;
	check(lang::load(dir, dir + "fr.lang", fr) && fr["lang_a"] == "Pomme" && fr["lang_b"] == en["lang_b"], ".lang over English");
	lang::Texts de;
	check(lang::load(dir, dir + "de.po", de) && de["lang_a"] == "Apfel" && de["lang_b"] == en["lang_b"], "PO over English");
	lang::Texts ja;
	check(lang::load(dir, dir + "ja.po", ja) && ja["lang_a"] == "?", "a PO set by hand loads without glyphs");
	lang::Texts none;
	check(!lang::load(dir, dir + "xx.po", none), "a missing file fails");
	writeFile(dir + "noend.lang", "lang_a\tA\n");
	check(!lang::readLang(dir + "noend.lang", none), ".lang without END fails");

	// Locale matching: exact ll_CC, then ll, in preference order; PO before .lang; a PO without glyphs is skipped
	writeFile(dir + "pl_PL.lang", "lang_a\tPL\nEND\n");
	check(lang::pickFile({dir}, {"pl_PL", "de"}) == dir + "pl_PL.lang", "exact ll_CC first");
	check(lang::pickFile({dir}, {"de_AT"}) == dir + "de.po", "ll when ll_CC is missing");
	check(lang::pickFile({dir}, {"ja_JP", "fr_CA"}) == dir + "fr.lang", "PO without glyphs skipped");
	check(lang::pickFile({dir}, {"en_US", "de"}) == dir + "en.lang", "English preferred stays English");
	check(lang::pickFile({dir}, {"xx"}).empty() && lang::pickFile({dir}, {}).empty(), "unknown locale: English");

	// Several folders (package, then player's data): a name in an earlier folder wins, a later folder fills gaps
	const std::string data = dir + "data/";
	std::filesystem::create_directories(data);
	writeFile(data + "nl.lang", "lang_a\tAppel\nEND\n");
	writeFile(data + "de.lang", "lang_a\tApfel\nEND\n");
	check(lang::pickFile({dir, data}, {"nl_NL"}) == data + "nl.lang", "player's data fills a missing language");
	check(lang::pickFile({dir, data}, {"de"}) == dir + "de.po", "package wins over the player's data");
	check(lang::pickFile({dir, data}, {"nl", "de"}) == data + "nl.lang", "locale order beats folder order");

	// PNS-47: the pre-step-13 saved default follows the OS; anything else is kept
	check(lang::savedFile("main/languages/en.lang").empty(), "old default means the OS language");
	check(lang::savedFile("main/languages/fr.lang") == "main/languages/fr.lang" && lang::savedFile("").empty(), "other values kept");

	if (!failures) std::printf("ok: lang\n");
	return failures ? 1 : 0;
}
