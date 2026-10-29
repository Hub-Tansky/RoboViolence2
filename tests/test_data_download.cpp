// ADR 0040: the first-run download's hash check, its install at originalDataDir() (where auto-detect finds it), and
// the commands and hash paths agreeing on where main/ lands.
#include "DataDownload.h"
#include "Paths.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static void setEnv(const char * name, const std::string & value)
{
#ifdef _WIN32
	_putenv_s(name, value.c_str());
#else
	setenv(name, value.c_str(), 1);
#endif
}

static void write(const fs::path & file, const char * text)
{
	fs::create_directories(file.parent_path());
	std::ofstream(file, std::ios::binary) << text;
}

int main(int argc, char ** argv)
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };
	if (argc < 2) { std::printf("usage: test_data_download <scratch dir>\n"); return 2; }
	const fs::path base = fs::absolute(fs::path(argv[1]) / "data_download");
	fs::remove_all(base);
	const fs::path home = base / "home", original = base / "BaboViolent2" / "original";
	fs::create_directories(home);
	setEnv("HOME", home.string());
	setEnv("BV2_DATA_DIR", (base / "nowhere").string());
	setEnv("ProgramFiles", (base / "nowhere").string()); // detectDataDir must not find a real install (Windows)
	setEnv("ProgramFiles(x86)", (base / "nowhere").string());
	setEnv("BV2_PREF_DIR", (base / "BaboViolent2" / "bv2").string());
	bv2::pathsInit(argv[0]);
	check(fs::path(bv2::originalDataDir()) == original, "originalDataDir is beside the pref dir");

	const char * X = "73cb3858a687a8494ca3323053016282f3dad39d42cf62ca4e79dda2aac7d9ac"; // sha256 of "x\n"
	const fs::path got = base / "partial" / "Content";
	write(got / "main" / "textures" / "a.tga", "x\n");
	write(got / "main" / "sounds" / "b.wav", "y\n");
	check(bv2::sha256File((got / "main" / "textures" / "a.tga").string()) == X, "sha256File");

	const std::vector<bv2::AssetHash> wrong = { { X, "main/textures/a.tga" }, { X, "main/sounds/b.wav" } };
	check(bv2::firstMismatch(got.string(), wrong) == "main/sounds/b.wav", "a wrong hash names the file");
	const std::vector<bv2::AssetHash> missing = { { X, "main/textures/none.tga" } };
	check(bv2::firstMismatch(got.string(), missing) == "main/textures/none.tga", "a missing file fails");
	const std::vector<bv2::AssetHash> right = { { X, "main/textures/a.tga" } };
	check(bv2::firstMismatch(got.string(), right).empty(), "matching hashes pass");
	check(bv2::detectDataDir().empty(), "no data before the install");

	write(original / "main" / "stale.txt", "old\n");
	check(!bv2::replaceDir((base / "nowhere").string(), original.string()).empty(), "a failed move is reported");
	check(fs::exists(original / "main" / "stale.txt") && !fs::exists(original.string() + ".old"),
		"a failed move keeps the old install");
	check(bv2::replaceDir(got.string(), bv2::originalDataDir()).empty(), "the install moves into place");
	check(fs::exists(original / "main" / "textures" / "a.tga") && !fs::exists(got), "data moved to originalDataDir");
	check(!fs::exists(original / "main" / "stale.txt") && !fs::exists(original.string() + ".old"), "an old install is replaced");
	check(bv2::detectDataDir() == fs::weakly_canonical(original).generic_string() + "/", "auto-detect finds the install");

	// The extract command and the hash paths agree: main/ lands in extractedDataDir, the hashes are relative to it
	const std::string partial = (base / "partial2").string();
	const std::vector<std::string> extract = bv2::extractArgs("bv2.zip", partial);
	const fs::path dataDir(bv2::extractedDataDir(partial));
	check(dataDir.filename() == "Content" && dataDir.parent_path().filename() == "BaboViolent2", "extracted data is Content/");
	bool namesMain = false;
	for (size_t i = 0; i < extract.size(); ++i)
		namesMain |= extract[i].rfind(fs::relative(dataDir / "main", partial).generic_string(), 0) == 0;
	check(namesMain, "extract names the zip's main/ folder");
	check(bv2::downloadArgs("bv2.zip").back() == bv2::ORIGINAL_ZIP_URL, "curl fetches the pinned zip");
	check(dataDir.parent_path().parent_path().filename() == std::string("BaboViolent2-") + bv2::ORIGINAL_COMMIT,
		"the zip's top folder names the pinned commit");
	check(fs::path(bv2::BROWSER_ZIP_URL).stem() == "master", "the browser zip unzips to BaboViolent2-master, which auto-detect finds");

	const std::vector<bv2::AssetHash> & hashes = bv2::originalMainHashes();
	check(hashes.size() == 324 && std::strncmp(hashes[0].path, "main/", 5) == 0 && std::strlen(hashes[0].sha256) == 64,
		"the generated list holds the 324 Content/main hashes");

	if (!failures) std::printf("ok: data download\n");
	return failures ? 1 : 0;
}
