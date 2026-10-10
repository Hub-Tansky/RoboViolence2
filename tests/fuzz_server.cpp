// Random-mutation fuzzer for one client message type against the in-process server (portable: GCC, Clang, MSVC).
//   fuzz_server <corpus dir> <type> <seconds> <seed> <out dir> [max inputs]
// Mutates the type's corpus files at their wire size and delivers them from a joined, spawned client. Before each
// delivery the input is written to <out dir>/<type>/last.bin and its number to last.txt; on a crash (ASan abort)
// last.bin is the reproducer and CI uploads both. A crash that needs earlier inputs replays with the same seed and
// max inputs = the number in last.txt. A clean run deletes them.
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

// A fresh client in slot 0 that has joined and spawned (the old one, if any, disconnects first).
static void join(UINT4 & client)
{
	if (client) harness::disconnect(client);
	client = harness::connect();
	for (int id : {NET_CLSV_GAMEVERSION_ACCEPTED, NET_CLSV_SVCL_PLAYER_INFO, NET_CLSV_SVCL_TEAM_REQUEST, NET_CLSV_SPAWN_REQUEST})
	{
		std::vector<char> m = corpus::valid(id);
		harness::deliver(client, id, m.data(), (int)m.size());
	}
}

int main(int argc, char ** argv)
{
	if (argc != 6 && argc != 7)
	{
		std::printf("usage: fuzz_server <corpus dir> <type> <seconds> <seed> <out dir> [max inputs]\n");
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
	fs::path lastCount = out / "last.txt";
	long maxInputs = argc == 7 ? std::atol(argv[6]) : -1;

	if (!harness::start(argv[0])) return 1;
	harness::sent().clear();
	UINT4 client = 0;
	join(client);

	std::mt19937 rng((unsigned)std::strtoul(argv[4], 0, 10));
	auto end = std::chrono::steady_clock::now() + std::chrono::seconds(std::atoi(argv[3]));
	long runs = 0;
	while (std::chrono::steady_clock::now() < end && runs != maxInputs)
	{
		std::vector<char> input = seeds[rng() % seeds.size()];
		if (t->size == 0) input.resize(rng() % 300); // text message: any length
		int flips = 1 + (int)(rng() % 8);
		for (int i = 0; i < flips && !input.empty(); ++i)
			input[rng() % input.size()] = (char)(rng() % 3 == 0 ? (rng() % 256) : input[rng() % input.size()] ^ (1 << (rng() % 8)));
		std::ofstream(last, std::ios::binary).write(input.data(), (std::streamsize)input.size());
		std::ofstream(lastCount) << "seed " << argv[4] << ", input " << runs + 1 << "\n";
		harness::deliver(client, t->id, input.data(), (int)input.size());
		harness::sent().clear();
		if (++runs % 200 == 0) join(client); // a kick or a death leaves no spawned player to fuzz
	}
	harness::stop();
	fs::remove(last);
	fs::remove(lastCount);
	std::printf("ok: %s, %ld inputs\n", t->name, runs);
	return 0;
}
