// Phase C step 5 (PNS-34): a char* never turns into a CString implicitly (it would run as a printf format); string
// literals and char arrays convert by copying the text as is.
#include "CString.h"

#include <cstdio>
#include <cstring>
#include <type_traits>

static_assert(!std::is_convertible<char *, CString>::value, "char* must need CString(\"%s\", text)");
static_assert(std::is_convertible<const char (&)[4], CString>::value, "literals convert");

static CString take(const CString & s) { return s; }

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };

	char wire[16] = "%s%n%x";  // e.g. chat text from a packet
	check(std::strcmp(take(wire).s, "%s%n%x") == 0, "char array copied verbatim");
	check(std::strcmp(take("100%").s, "100%") == 0, "literal copied verbatim");
	char unterminated[4] = {'a', 'b', 'c', 'd'};
	check(std::strcmp(take(unterminated).s, "abcd") == 0, "array without NUL stops at its size");
	check(std::strcmp(CString("%i-%s", 7, "x").s, "7-x") == 0, "explicit format still formats");

	if (!failures) std::printf("ok: cstring\n");
	return failures ? 1 : 0;
}
