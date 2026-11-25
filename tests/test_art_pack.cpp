// ROBO-260: the art pack's install checks, without SDL or network: installed version, the off switch, the archive
// hash, the install with VERSION, and the curl and tar command lines; the pack stack with the build's packs (ROBO-261)
#include "ArtPack.h"
#include "DataDownload.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
static int g_fail = 0;

static void check(bool ok, const char* what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_fail = 1;
	}
}

static void write(const fs::path& file, const std::string& text)
{
	fs::create_directories(file.parent_path());
	std::ofstream(file, std::ios::binary) << text;
}

int main(int argc, char** argv)
{
	if (argc < 2)
		return std::printf("FAIL: usage: test_art_pack <scratch dir>\n"), 1;
	const fs::path dir = fs::path(argv[1]) / "art-pack";
	fs::remove_all(dir);

	// A build without a pinned pack downloads nothing
	check(std::string(bv2::ART_PACK_VERSION).empty() || !std::string(bv2::ART_PACK_SHA256).empty(),
		  "a pinned version has a pinned SHA-256");

	// installed: main/pack.tsv and the right VERSION
	const fs::path target = dir / "robo_art";
	check(!bv2::artPackInstalled(target.string(), "1"), "a missing folder counts as installed");
	const fs::path unpacked = dir / "partial" / "pack";
	write(unpacked / "main" / "pack.tsv", "id\tfile\n");
	check(bv2::installArtPack((dir / "partial" / "nopack").string(), "1", target.string()) != "",
		  "a folder without main/pack.tsv installs");
	write(unpacked / "main" / "pack.tsv", "id\tfile\ntextures/a\ttextures/a.png\ttexture\n");
	check(bv2::installArtPack(unpacked.string(), "1", target.string()) != "" && !fs::exists(target),
		  "a pack missing a listed file installs");
	write(unpacked / "main" / "textures" / "a.png", "png");
	check(bv2::installArtPack(unpacked.string(), "1", target.string()) == "", "a valid pack does not install");
	check(bv2::artPackInstalled(target.string(), "1"), "an installed pack is not found");
	check(!bv2::artPackInstalled(target.string(), "2"), "an older pack counts as this build's");
	check(!bv2::artPackInstalled(target.string(), ""), "a build without a pin stacks a pack");

	// An update replaces the old pack
	write(unpacked / "main" / "pack.tsv", "id\tfile\n");
	check(bv2::installArtPack(unpacked.string(), "2", target.string()) == "", "an update does not install");
	check(bv2::artPackInstalled(target.string(), "2"), "an update did not replace VERSION");

	// The archive must have the pinned hash; an empty pin never matches
	const fs::path archive = dir / "a.tar.gz";
	write(archive, "abc");
	const std::string abc = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
	check(bv2::artArchiveValid(archive.string(), abc), "the right hash is refused");
	check(!bv2::artArchiveValid(archive.string(), abc.substr(1) + "0"), "a wrong hash is accepted");
	check(!bv2::artArchiveValid(archive.string(), ""), "an empty pin is accepted");

	// The off switch: the flag is remembered in the pref dir
	const fs::path pref = dir / "pref";
	fs::create_directories(pref);
	const char* none[] = {"bv2"};
	const char* flag[] = {"bv2", "--no-art-download"};
	check(!bv2::artDownloadOff(1, none, pref.string()), "the download is off by default");
	const char* placeholders[] = {"bv2", "--placeholders"};
	check(bv2::artDownloadOff(2, placeholders, pref.string()), "--placeholders does not turn it off");
	check(!bv2::artDownloadOff(1, none, pref.string()), "--placeholders is remembered");
	check(bv2::artDownloadOff(2, flag, pref.string()), "--no-art-download does not turn it off");
	check(bv2::artDownloadOff(1, none, pref.string()), "--no-art-download is not remembered");

	// Command lines: curl to the file, tar unpacks a .tar.gz into the folder
	const auto curl = bv2::artDownloadArgs("https://example.invalid/a.tar.gz", "x.tar.gz");
	check(curl.size() >= 3 && curl.back() == "https://example.invalid/a.tar.gz" && curl[curl.size() - 2] == "x.tar.gz",
		  "curl's argv ends with -o <file> <url>");
	const auto tar = bv2::artExtractArgs("x.tar.gz", "out");
	check(tar.size() == 5 && tar[1] == "-xzf" && tar[2] == "x.tar.gz" && tar[4] == "out", "tar's argv");

	// The download sequence with fake commands: curl copies the "url" file, tar writes main/pack.tsv
	const fs::path src = dir / "src.tar.gz", fetched = dir / "fetched", partial = dir / "fetched.partial";
	write(src, "abc"); // its SHA-256 is abc below
	bool tarOk = true, stopAfterTar = false, tarRan = false;
	const bv2::ArtRunner fake = [&](const std::vector<std::string>& args)
	{
		if (args[1] == "-xzf")
		{
			write(fs::path(args.back()) / "main" / "pack.tsv", "id\n");
			tarRan = true;
			return tarOk;
		}
		std::error_code ec;
		fs::copy_file(args.back(), args[args.size() - 2], fs::copy_options::overwrite_existing, ec);
		return !ec;
	};
	const auto stopped = [&]
	{ return stopAfterTar && tarRan; };
	check(!bv2::fetchArtPack(partial.string(), src.string(), abc.substr(1) + "0", "7", fetched.string(), fake, stopped).empty() &&
			  !fs::exists(fetched) && !tarRan,
		  "a wrong hash is unpacked or installed");
	tarOk = false;
	check(!bv2::fetchArtPack(partial.string(), src.string(), abc, "7", fetched.string(), fake, stopped).empty() &&
			  !fs::exists(fetched),
		  "a failed unpack is installed");
	tarOk = true;
	stopAfterTar = true;
	tarRan = false;
	check(bv2::fetchArtPack(partial.string(), src.string(), abc, "7", fetched.string(), fake, stopped) == "stopped" &&
			  !fs::exists(fetched),
		  "a pack stopped while unpacking is installed");
	stopAfterTar = false;
	check(bv2::fetchArtPack(partial.string(), src.string(), abc, "7", fetched.string(), fake, stopped).empty() &&
			  bv2::artPackInstalled(fetched.string(), "7") && !fs::exists(partial),
		  "a good download is not installed, or its scratch folder stays");

	// ROBO-261: the stack with the build's packs (url on top, its art pack over the player's) and without them
	{
		const fs::path build = dir / "packs", pref = dir / "pref-art";
		write(pref / "main" / "pack.tsv", "id\n");
		write(pref / "VERSION", "7\n");
		const std::string b = build.generic_string() + "/", p = pref.generic_string();
		bv2::PackStack s = bv2::assetPackStack(b, true, p, "7", "data/", "pkg/");
		check(s.roots.size() == 3 && s.roots[0] == p && s.cwdPack == 1 && !s.buildArt, "no build packs: the player's art pack");
		write(build / "robo_art" / "main" / "pack.tsv", "id\n");
		write(build / "robo_art" / "VERSION", "6\n");
		s = bv2::assetPackStack(b, true, p, "7", "data/", "pkg/");
		check(s.roots[0] == p && !s.buildArt, "the build's art pack of another version is stacked");
		write(build / "robo_art" / "VERSION", "7\n");
		write(build / "url" / "main" / "pack.tsv", "id\n");
		s = bv2::assetPackStack(b, true, p, "7", "data/", "pkg/");
		check(s.roots.size() == 4 && s.roots[0] == (build / "url").generic_string() &&
				  s.roots[1] == (build / "robo_art").generic_string() && s.roots[2] == "data/" && s.roots[3] == "pkg/" &&
				  s.cwdPack == 2 && s.buildArt,
			  "url over the build's art pack over the data root over the base pack");
		s = bv2::assetPackStack(b, false, p, "7", "data/", "pkg/");
		check(s.roots.size() == 3 && s.roots[0] == p && s.cwdPack == 1 && !s.buildArt, "--placeholders leaves the build's packs out");
		s = bv2::assetPackStack("", true, (dir / "none").generic_string(), "7", "data/", "pkg/");
		check(s.roots.size() == 2 && s.cwdPack == 0, "no art pack at all: data root over base pack");
	}

	if (!g_fail)
		std::printf("art pack: all checks passed\n");
	return g_fail;
}
