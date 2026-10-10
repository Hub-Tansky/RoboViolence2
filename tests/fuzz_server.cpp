// Random-mutation fuzzer for one client message type against the in-process server (portable: GCC, Clang, MSVC).
//   fuzz_server <corpus dir> <type> <seconds> <seed> <out dir>
// Mutates the type's corpus files at their wire size and delivers them from a joined client. Before each delivery
// the input is written to <out dir>/<type>/last.bin; on a crash (ASan abort) that file is the reproducer, and CI
// uploads it. A clean run deletes it.
#include "corpus_types.h"
#include "server_harness.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>

namespace fs = std::filesystem;

static void join(UINT4 & client)
{
	client = harness::connect();
	std::vector<char> accept = corpus::valid(NET_CLSV_GAMEVERSION_ACCEPTED);
	std::vector<char> info = corpus::valid(NET_CLSV_SVCL_PLAYER_INFO);
	harness::deliver(client, NET_CLSV_GAMEVERSION_ACCEPTED, accept.data(), (int)accept.size());
	harness::deliver(client, NET_CLSV_SVCL_PLAYER_INFO, info.data(), (int)info.size());
}

int main(int argc, char ** argv)
{
	if (argc != 6)
	{
		std::printf("usage: fuzz_server <corpus dir> <type> <seconds> <seed> <out dir>\n");
		return 1;
	}
	const corpus::Type * t = corpus::find(argv[2]);
	if (!t)
	{
		std::printf("unknown type %s\n", argv[2]);
		return 1;
	}
	std::vector<std::vector<char>> seeds;
	for (const fs::directory_entry & e : fs::directory_iterator(fs::path(argv[1]) / t->name))
		if (e.path().filename().string().rfind("crash-", 0) != 0)
		{
			std::ifstream in(e.path(), std::ios::binary);
			seeds.emplace_back(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
		}
	fs::path out = fs::path(argv[5]) / t->name;
	fs::create_directories(out);
	fs::path last = out / "last.bin";

	if (!harness::start(argv[0])) return 1;
	harness::sent().clear();
	UINT4 client;
	join(client);

	std::mt19937 rng((unsigned)std::strtoul(argv[4], 0, 10));
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(std::atoi(argv[3]));
	long runs = 0;
	while (std::chrono::steady_clock::now() < end)
	{
		std::vector<char> input = seeds[rng() % seeds.size()];
		if (t->size == 0) input.resize(rng() % 300); // text message: any length
		int flips = 1 + (int)(rng() % 8);
		for (int i = 0; i < flips && !input.empty(); ++i)
			input[rng() % input.size()] = (char)(rng() % 3 == 0 ? (rng() % 256) : input[rng() % input.size()] ^ (1 << (rng() % 8)));
		// Known defects stay out of the way until their fix step removes the mask: Q-S2 (playerID out of range),
		// R21/R22 (client text used as a format string, console text without '\0').
		if (t->playerID >= 0) input[t->playerID] &= 31;
		if (t->id == NET_CLSV_SVCL_CHAT || t->id == NET_SVCL_CONSOLE)
			for (char & c : input)
				if (c == '%') c = '_';
		if (t->id == NET_SVCL_CONSOLE) input.push_back('\0');

		std::ofstream(last, std::ios::binary).write(input.data(), (std::streamsize)input.size());
		harness::deliver(client, t->id, input.data(), (int)input.size());
		harness::sent().clear();
		if (++runs % 200 == 0) join(client); // kicks and disconnects leave no player to fuzz
	}
	harness::stop();
	fs::remove(last);
	std::printf("ok: %s, %ld inputs\n", t->name, runs);
	return 0;
}
