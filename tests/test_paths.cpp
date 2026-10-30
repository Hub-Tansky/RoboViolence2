// Phase C step 1 (ADR 0016): package root vs data root, contentFile, findDataIn, the saved folder and auto-detect;
// the Options field and the start order with the placeholders choice (ADR 0042).
#include "Paths.h"
#include "dksvar.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
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
	check(bv2::findDataIn(content.parent_path().string()) == root(content), "upstream BaboViolent2/");
	check(bv2::findDataIn(content.string()) == root(content), "upstream Content/");
	check(bv2::findDataIn((content / "main").string()) == root(content), "main/ itself");
	check(bv2::findDataIn(pkg.string()).empty(), "no main/textures: not BV2 data");
	check(bv2::detectDataDir() == root(content), "auto-detect in ~/Downloads");
	{
		// Windows Extract All of BaboViolent2-master.zip with its default target: the zip's own folder inside a folder of the same name
		const fs::path nested = home / "Desktop" / "BaboViolent2-master" / "BaboViolent2-master" / "BaboViolent2" / "Content";
		touch(nested / "main" / "textures" / "a.tga");
		check(bv2::findDataIn((home / "Desktop" / "BaboViolent2-master").string()) == root(nested), "Extract All nesting");
		fs::remove_all(home / "Desktop");
	}

	check(bv2::savedDataDir().empty() && !bv2::placeholdersSaved(), "nothing saved yet");
	check(bv2::startDataDir().dir == root(content) && !bv2::startDataDir().ask, "nothing saved: start on detected data");
	check(bv2::playerDataDir().empty(), "on the package's placeholders there is no player data folder");

	// Options > Game assets (ADR 0042)
	check(bv2::saveDataDirField("") == bv2::DataDirField::Placeholders && bv2::placeholdersSaved(), "empty field saves placeholders");
	check(bv2::startDataDir().dir.empty() && !bv2::startDataDir().ask, "placeholders win over detected data, without the box");
	check(bv2::saveDataDirField((base / "nowhere").string()) == bv2::DataDirField::NoData && bv2::placeholdersSaved(),
		"a folder without data is not saved");
	const std::string copied = " \"" + (home / "Downloads" / "BaboViolent2-master").string() + "\" ";
	check(bv2::saveDataDirField(copied) == bv2::DataDirField::Saved && !bv2::placeholdersSaved(), "a quoted, padded path is saved");
	check(bv2::savedDataDir() == root(content) && bv2::startDataDir().dir == root(content), "saved folder read back");

	check(bv2::setDataRoot(root(content)) && bv2::dataRoot() == root(content), "setDataRoot");
	check(fs::equivalent(fs::current_path(), content), "setDataRoot changes into it");
	check(bv2::playerDataDir() == root(content), "the player's data folder");
	check(bv2::contentFile("main/LaunchScript/CTF.cfg") == root(pkg) + "main/LaunchScript/CTF.cfg", "launch script from the package");
	check(bv2::contentFile("main/textures/a.tga") == "main/textures/a.tga", "assets from the data root");
	check(bv2::contentFile("main/LaunchScript/TDM.cfg") == "main/LaunchScript/TDM.cfg", "a script only the data has");
	check(bv2::contentFile("main/languages/en.lang") == root(pkg) + "main/languages/en.lang", "saved languageFile resolves to the package");

	// config/local beside the package still applies when the data root is the player's
	Sink sink;
	dksvarInit(&sink);
	int local = 1;
	dksvarRegister(CString("test_local [int : (default 1)]"), &local, 0, 100, 0, true);
	// ADR 0028: the built-in master wins over an old bv2.cfg that saved cl_masterServers "", and is never saved
	CString masters("master.example.invalid:10207");
	dksvarRegister(CString("cl_masterServers [string : (default master.example.invalid:10207)]"), &masters, true);
	setEnv("BV2_MASTER_SERVERS", "");
	fs::create_directories(fs::path(bv2::configFile()).parent_path());
	std::ofstream(bv2::configFile()) << "cl_masterServers \"\"\n\n";
	const char * args[1] = { argv[0] };
	bv2::loadConfigLayers(1, args);
	check(local == 5, "config/local beside the package");
	check(masters == CString("master.example.invalid:10207"), "bv2.cfg cannot pin the master list");
	dksvarSaveConfig((char *)bv2::configFile().c_str());
	{
		std::ifstream in(bv2::configFile());
		std::string saved((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		check(saved.find("cl_masterServers") == std::string::npos, "the master list is not saved");
	}
	setEnv("BV2_MASTER_SERVERS", "127.0.0.1:10207");
	bv2::loadConfigLayers(1, args);
	check(masters == CString("127.0.0.1:10207"), "BV2_MASTER_SERVERS overrides the built-in master");
	setEnv("BV2_MASTER_SERVERS", "none");
	bv2::loadConfigLayers(1, args);
	check(bv2::parseHostPorts(masters.s).empty(), "BV2_MASTER_SERVERS=none turns the master list off");
	setEnv("BV2_MASTER_SERVERS", "");
	check(bv2::parseHostPorts("none,127.0.0.1:10207").size() == 1, "a none entry beside a master is skipped");
	std::vector<bv2::HostPort> hp = bv2::parseHostPorts("master.roboviolence.com:10207, other.invalid");
	check(hp.size() == 2 && hp[0].host == "master.roboviolence.com" && hp[0].port == 10207 && hp[1].port == 11112,
		"host:port list");

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
	check(bv2::startDataDir().dir.empty() && bv2::startDataDir().ask, "no data anywhere: the first-run box");

	if (!failures) std::printf("ok: paths\n");
	return failures ? 1 : 0;
}
