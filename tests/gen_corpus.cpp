// Writes tests/corpus/<type>/valid.bin for every type in corpus_types.h, and the known crash inputs. Run it after changing netPacket.h:
//   gen_corpus <repo>/tests/corpus
#include "corpus_types.h"

#include <cstdio>
#include <filesystem>

int main(int argc, char ** argv)
{
	if (argc != 2)
	{
		std::printf("usage: gen_corpus <corpus dir>\n");
		return 1;
	}
	std::vector<corpus::Crash> files = corpus::knownCrashes();
	for (const corpus::Type & t : corpus::types)
	{
		files.push_back({t.name, "valid", corpus::valid(t.id)});
		if (files.back().data.empty())
		{
			std::printf("no valid sample for %s: add it to corpus::valid()\n", t.name);
			return 1;
		}
	}
	for (const corpus::Crash & c : files)
	{
		std::filesystem::path dir = std::filesystem::path(argv[1]) / c.type;
		std::filesystem::create_directories(dir);
		FILE * f = std::fopen((dir / (std::string(c.name) + ".bin")).string().c_str(), "wb");
		std::fwrite(c.data.data(), 1, c.data.size(), f);
		std::fclose(f);
	}
	return 0;
}
