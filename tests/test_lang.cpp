// Phase B step 13: the PO parser, English fallback per key, OS locale matching and the glyph skip (game/src/Lang.cpp).
#include "Lang.h"

#include <cstdio>
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

	// PO parser: header, comments, escapes, multi-line strings, untranslated entries, entries without msgctxt
	lang::Texts t;
	bool glyphs = false;
	check(lang::parsePo("msgid \"\"\nmsgstr \"Content-Type: text/plain; charset=UTF-8\\n\"\n\n"
		"# comment\nmsgctxt \"lang_a\"\nmsgid \"A\"\nmsgstr \"\\x4Zw\\\"x\\\\y\\n\"\n\"%s cd\"\n\n"
		"msgctxt \"lang_b\"\nmsgid \"B\"\nmsgstr \"\"\n\nmsgid \"C\"\nmsgstr \"c\"\n"
		"msgctxt \"lang_e\"\r\nmsgid \"E\"\r\nmsgstr \"\xc3\xa9t\xc3\xa9\"\r\n", t, &glyphs), "PO parses");
	check(t["lang_a"] == "\x04Zw\"x\\y\n%s cd", "escapes and continuation lines");
	check(!t.count("lang_b"), "untranslated entry skipped");
	check(t.size() == 2 && t["lang_e"] == "\x82t\x82", "no msgctxt skipped; CRLF; UTF-8 e-acute is font byte 130");
	check(glyphs, "Latin-1 letters have glyphs");
	check(!lang::parsePo("msgctxt \"a\"\nmsgid \"A\"\nmsgstr \"unterminated\n", t, 0), "bad quote rejected");
	check(!lang::parsePo("msgctxt \"a\"\nmsgstr[0] \"x\"\n", t, 0), "unknown keyword rejected");
	check(!lang::parsePo("\"orphan\"\n", t, 0), "continuation without a keyword rejected");
	lang::Texts pl;
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
	check(lang::pickFile(dir, {"pl_PL", "de"}) == dir + "pl_PL.lang", "exact ll_CC first");
	check(lang::pickFile(dir, {"de_AT"}) == dir + "de.po", "ll when ll_CC is missing");
	check(lang::pickFile(dir, {"ja_JP", "fr_CA"}) == dir + "fr.lang", "PO without glyphs skipped");
	check(lang::pickFile(dir, {"en_US", "de"}) == dir + "en.lang", "English preferred stays English");
	check(lang::pickFile(dir, {"xx"}).empty() && lang::pickFile(dir, {}).empty(), "unknown locale: English");

	if (!failures) std::printf("ok: lang\n");
	return failures ? 1 : 0;
}
