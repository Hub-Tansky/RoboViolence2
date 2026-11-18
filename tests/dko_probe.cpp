// ROBO-239: loads a .DKO through engine/dko (the game's loader) and prints what tests/test_gltf_to_dko.py checks:
// the bounding box, the radius and the position of one dummy. Exit 1 when the file does not load.
// Usage: dko_probe <file.DKO> <dummy name>
#include "dko.h"

#include <cstdio>

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::printf("usage: dko_probe <file.DKO> <dummy name>\n");
		return 2;
	}
	dkoInit();
	unsigned int model = dkoLoadFile(argv[1]);
	if (!model)
	{
		std::printf("FAIL: dkoLoadFile: %s\n", dkoGetLastError() ? dkoGetLastError() : "?");
		return 1;
	}
	float box[6] = {0, 0, 0, 0, 0, 0};
	dkoGetOABB(model, box);
	std::printf("probe oabb %g %g %g %g %g %g\n", box[0], box[1], box[2], box[3], box[4], box[5]);
	std::printf("probe radius %g\n", dkoGetRadius(model));
	if (!dkoGetDummy(argv[2], model))
	{
		std::printf("FAIL: no dummy %s\n", argv[2]);
		return 1;
	}
	float pos[3] = {0, 0, 0};
	dkoGetDummyPosition(argv[2], model, pos);
	std::printf("probe dummy %g %g %g\n", pos[0], pos[1], pos[2]);
	dkoDeleteModel(&model);
	dkoShutDown();
	return 0;
}
