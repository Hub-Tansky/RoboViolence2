// Replays tests/corpus through the in-process server: no crash, and the expected reply to each valid sample.
//   test_replay <corpus dir>          every file except crash-*.bin (known defects, see KEY_QUESTIONS.md)
//   test_replay <corpus dir> <file>   one file (ctest runs each crash-*.bin this way, expected to fail until fixed)
#include "corpus_types.h"
#include "server_harness.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

static std::vector<char> readFile(const fs::path & p)
{
	std::ifstream in(p, std::ios::binary);
	return std::vector<char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

static bool replies(int typeID, size_t from)
{
	std::vector<harness::SentPacket> & sent = harness::sent();
	for (size_t i = from; i < sent.size(); ++i)
		if (sent[i].typeID == typeID) return true;
	return false;
}

int main(int argc, char ** argv)
{
	if (argc < 2)
	{
		std::printf("usage: test_replay <corpus dir> [file]\n");
		return 1;
	}
	if (!harness::start(argv[0]))
	{
		std::printf("FAIL: server did not start\n");
		return 1;
	}
	UINT4 client = harness::connect();
	int failures = 0;
	for (const corpus::Type & t : corpus::types)
	{
		std::vector<fs::path> files;
		if (argc == 3)
		{
			if (fs::path(argv[2]).parent_path().filename() == t.name) files.push_back(argv[2]);
		}
		else
			for (const fs::directory_entry & e : fs::directory_iterator(fs::path(argv[1]) / t.name))
				if (e.path().filename().string().rfind("crash-", 0) != 0) files.push_back(e.path());
		std::sort(files.begin(), files.end());
		for (const fs::path & f : files)
		{
			std::vector<char> data = readFile(f);
			size_t before = harness::sent().size();
			harness::deliver(client, t.id, data.data(), (int)data.size());
			harness::tick(2);
			if (t.reply && f.filename() == "valid.bin" && !replies(t.reply, before))
			{
				std::printf("FAIL %s: no reply of type %d\n", f.string().c_str(), t.reply);
				++failures;
			}
		}
	}
	harness::stop();
	if (failures == 0) std::printf("ok: corpus replayed\n");
	return failures ? 1 : 0;
}
