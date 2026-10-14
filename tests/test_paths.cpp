// Phase C step 1 (ADR 0016): package root vs data root, contentFile, findDataIn, the saved folder and auto-detect.
#include "Paths.h"
#include "dksvar.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static void setEnv(const char * name, const std::string & value)
{
#ifdef _WIN32
	_putenv_s(name, value.c_str());
#else
	if (value.empty())
		unsetenv(name);
	else
		setenv(name, value.c_str(), 1);
#endif
}

static void touch(const fs::path & file)
{
	fs::create_directories(file.parent_path());
	std::ofstream(file) << "x\n";
}

class Sink : public CStringInterface
{
public:
	void updateString(CString * string, char * newValue) override
	{
		if (string)
			*string = CString("%s", newValue);
	}
};

static std::string root(const fs::path & dir) { return fs::weakly_canonical(dir).generic_string() + "/"; }

int main(int argc, char ** argv)
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };
	if (argc < 2) { std::printf("usage: test_paths <scratch dir>\n"); return 2; }
	const fs::path base = fs::absolute(fs::path(argv[1]) / "paths");
	fs::remove_all(base);

	// The package holds the repo's content; the player's BV2 copy is the upstream zip in ~/Downloads
	const fs::path pkg = base / "pkg", home = base / "home";
	const fs::path content = home / "Downloads" / "BaboViolent2-master" / "BaboViolent2" / "Content";
	touch(pkg / "main" / "LaunchScript" / "CTF.cfg");
	touch(pkg / "main" / "languages" / "en.lang");
	touch(content / "main" / "LaunchScript" / "CTF.cfg");
	touch(content / "main" / "LaunchScript" / "TDM.cfg");
	touch(content / "main" / "languages" / "en.lang");
	touch(content / "main" / "textures" / "a.tga");
	setEnv("BV2_PREF_DIR", (base / "pref").string());
	setEnv("HOME", home.string());
	setEnv("BV2_DATA_DIR", "");
	setEnv("ProgramFiles", ""); // detectDataDir must not find a real install (Windows)
	setEnv("ProgramFiles(x86)", "");
	fs::create_directories(pkg / "config" / "local");
	std::ofstream(pkg / "config" / "local" / "test.cfg") << "test_local 5\n\n";

	fs::current_path(pkg);
	check(bv2::pathsInit(argv[0]), "pathsInit finds the package");
	check(bv2::packageRoot() == root(pkg) && bv2::dataRoot() == root(pkg), "without BV2_DATA_DIR the data root is the package");

	check(bv2::findDataIn((home / "Downloads" / "BaboViolent2-master").string()) == root(content), "upstream zip layout");
	check(bv2::findDataIn((content / "main").string()) == root(content), "main/ itself");
	check(bv2::findDataIn(pkg.string()).empty(), "no main/textures: not BV2 data");
	check(bv2::detectDataDir() == root(content), "auto-detect in ~/Downloads");

	check(bv2::savedDataDir().empty(), "nothing saved yet");
	bv2::saveDataDir(root(content));
	check(bv2::savedDataDir() == root(content), "saved folder read back");

	check(bv2::setDataRoot(root(content)) && bv2::dataRoot() == root(content), "setDataRoot");
	check(fs::equivalent(fs::current_path(), content), "setDataRoot changes into it");
	check(bv2::contentFile("main/LaunchScript/CTF.cfg") == root(pkg) + "main/LaunchScript/CTF.cfg", "launch script from the package");
	check(bv2::contentFile("main/textures/a.tga") == "main/textures/a.tga", "assets from the data root");
	check(bv2::contentFile("main/LaunchScript/TDM.cfg") == "main/LaunchScript/TDM.cfg", "a script only the data has");
	check(bv2::contentFile("main/languages/en.lang") == root(pkg) + "main/languages/en.lang", "saved languageFile resolves to the package");

	// config/local beside the package still applies when the data root is the player's
	Sink sink;
	dksvarInit(&sink);
	int local = 1;
	dksvarRegister(CString("test_local [int : (default 1)]"), &local, 0, 100, 0, true);
	const char * args[1] = { argv[0] };
	bv2::loadConfigLayers(1, args);
	check(local == 5, "config/local beside the package");

	setEnv("BV2_DATA_DIR", (content / "main").string());
	fs::current_path(pkg);
	check(bv2::pathsInit(argv[0]) && bv2::dataRoot() == root(content) && bv2::packageRoot() == root(pkg), "BV2_DATA_DIR keeps the package root");

	setEnv("BV2_DATA_DIR", (home / "Downloads" / "BaboViolent2-master").string());
	fs::current_path(pkg); // the package is found through the working directory
	check(bv2::pathsInit(argv[0]) && bv2::dataRoot() == root(content), "BV2_DATA_DIR searched as findDataIn");
	setEnv("BV2_DATA_DIR", (base / "nowhere").string());
	fs::current_path(pkg);
	check(bv2::pathsInit(argv[0]) && bv2::dataRoot() == root(pkg), "invalid BV2_DATA_DIR falls back to the package");

	fs::remove_all(content);
	check(bv2::savedDataDir().empty(), "a saved folder that no longer holds data is ignored");

	if (!failures) std::printf("ok: paths\n");
	return failures ? 1 : 0;
}
