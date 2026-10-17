// Phase C step 5 (PNS-34, ADR 0021): one argument is text, copied as is, however it is passed; only two or more
// arguments format. Wire text in a CString can't run as a printf format.
#include "CString.h"

#include <cstdio>
#include <cstring>

static CString take(const CString & s) { return s; }

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };

	char wire[16] = "%s%n%x"; // e.g. a player name or chat text from a packet
	char * ptr = wire;
	check(std::strcmp(take(wire).s, "%s%n%x") == 0, "char array, implicit");
	check(std::strcmp(CString(wire).s, "%s%n%x") == 0, "char array, direct");
	check(std::strcmp(take(ptr).s, "%s%n%x") == 0, "char*, implicit");
	check(std::strcmp(CString(ptr).s, "%s%n%x") == 0, "char*, direct");
	check(std::strcmp(CString("100%").s, "100%") == 0, "literal");
	check(std::strcmp(CString((const char *)0).s, "") == 0, "null is empty");
	check(std::strcmp(CString("%i-%s", 7, "x").s, "7-x") == 0, "two or more arguments format");
	check(std::strcmp(CString("%s", wire).s, "%s%n%x") == 0, "%s keeps the text");

	if (!failures) std::printf("ok: cstring\n");
	return failures ? 1 : 0;
}
