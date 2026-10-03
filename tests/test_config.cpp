// dksvar: layered config order, transient values never saved, secrets masked.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

#include "dksvar.h"

static int failures = 0;

#define CHECK(cond)                                                    \
	do                                                                 \
	{                                                                  \
		if (!(cond))                                                   \
		{                                                              \
			std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			++failures;                                                \
		}                                                              \
	} while (0)

class Sink : public CStringInterface
{
public:
	std::string last;
	void updateString(CString * string, char * newValue) override
	{
		last = newValue;
		if (string)
			*string = CString("%s", newValue);
	}
};

static void write(const std::string & path, const std::string & text)
{
	std::ofstream f(path.c_str());
	f << text;
}

static std::string slurp(const std::string & path)
{
	std::ifstream f(path.c_str());
	return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

int main(int argc, char ** argv)
{
	if (argc < 2)
	{
		std::printf("usage: test_config <scratch dir>\n");
		return 2;
	}
	std::string dir = argv[1];
	Sink sink;
	dksvarInit(&sink);

	int level = 1;
	int kept = 1;
	CString adminPass("none");
	CString masters("");
	dksvarRegister(CString("test_level [int : (default 1)]"), &level, 0, 100, 0, true);
	dksvarRegister(CString("test_kept [int : (default 1)]"), &kept, 0, 100, 0, true);
	dksvarRegister(CString("test_adminPass [string : (default none)]"), &adminPass, true);
	dksvarRegister(CString("test_masters [string : (default empty)]"), &masters, true);

	// Names carry a help suffix, as in GameVar.cpp. Entries are blank-line separated, as dksvarSaveConfig writes them.
	// Later layers override earlier ones.
	std::string base = dir + "/base.cfg", user = dir + "/user.cfg", local = dir + "/local.cfg", saved = dir + "/saved.cfg";
	write(base, "test_level 10\n\ntest_kept 7\n\n");
	write(user, "test_level 20\n\n");
	write(local, "test_level 30\n\ntest_masters \"example.invalid:1\"\n\ntest_adminPass \"hunter2\"\n");
	dksvarLoadConfig((char *)base.c_str());
	CHECK(level == 10);
	dksvarLoadConfig((char *)user.c_str());
	CHECK(level == 20);

	// Transient layers apply but are not written back.
	dksvarLoadConfigTransient((char *)local.c_str());
	CHECK(level == 30);
	CHECK(masters == CString("example.invalid:1"));
	CHECK(dksvarSetTransient("test_masters", "other.invalid:2"));
	CHECK(masters == CString("other.invalid:2"));
	CHECK(!dksvarSetTransient("no_such_variable", "x"));

	dksvarSaveConfig((char *)saved.c_str());
	std::string out = slurp(saved);
	CHECK(kept == 7);
	CHECK(out.find("test_kept 7") != std::string::npos);
	CHECK(out.find("test_level") == std::string::npos);
	CHECK(out.find("example.invalid") == std::string::npos);
	CHECK(out.find("other.invalid") == std::string::npos);
	CHECK(out.find("hunter2") == std::string::npos);

	// Secrets are masked when a variable is echoed.
	CHECK(dksvarIsSecret("test_adminPass"));
	CHECK(!dksvarIsSecret("test_level"));
	sink.last.clear();
	CString echoed;
	dksvarGetFormatedVar((char *)"test_adminPass", &echoed);
	CHECK(sink.last.find("hunter2") == std::string::npos);
	CHECK(sink.last.find("***") != std::string::npos);

	if (failures == 0)
		std::printf("ok: config layering, transient, secrets\n");
	return failures ? 1 : 0;
}
