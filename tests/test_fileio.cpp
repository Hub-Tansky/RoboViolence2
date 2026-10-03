// .bvm maps are read and written with FileIO; every width is explicit and independent of the platform's
// `int`/`long`. Also pins the widths the .DKO loader relies on.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "FileIO.h"

static_assert(sizeof(INT4) == 4, "INT4 is 4 bytes (3DS/DKO chunk fields)");
static_assert(sizeof(short) == 2 && sizeof(float) == 4, "DKO fields use short and float");

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

static std::vector<unsigned char> slurp(const std::string & path)
{
	std::vector<unsigned char> out;
	FILE * f = std::fopen(path.c_str(), "rb");
	if (!f)
		return out;
	int c;
	while ((c = std::fgetc(f)) != EOF)
		out.push_back((unsigned char)c);
	std::fclose(f);
	return out;
}

int main(int argc, char ** argv)
{
	if (argc < 2)
	{
		std::printf("usage: test_fileio <scratch dir>\n");
		return 2;
	}
	std::string path = std::string(argv[1]) + "/fileio.bin";
	{
		FileIO out(CString("%s", path.c_str()), "wb");
		CHECK(out.isValid());
		out.put((int)-2);                       // 2 bytes (the .bvm "int" is a short)
		out.put((unsigned int)65535u);          // 2 bytes
		out.put((long)-3);                      // 4 bytes, even where long is 64-bit
		out.put((unsigned long)4000000000ul);   // 4 bytes
		out.put(1.5f);                          // 4 bytes
		out.put((unsigned char)200);            // 1 byte
		out.put(true);                          // 1 byte
	}
	std::vector<unsigned char> b = slurp(path);
	CHECK(b.size() == 2 + 2 + 4 + 4 + 4 + 1 + 1);
	if (b.size() == 18)
	{
		CHECK(b[0] == 0xFE && b[1] == 0xFF);                // -2, little endian
		CHECK(b[2] == 0xFF && b[3] == 0xFF);                // 65535
		CHECK(b[4] == 0xFD && b[5] == 0xFF && b[6] == 0xFF && b[7] == 0xFF); // -3 as int32
		CHECK(b[8] == 0x00 && b[9] == 0x28 && b[10] == 0x6B && b[11] == 0xEE); // 4000000000 = 0xEE6B2800
		CHECK(b[12] == 0x00 && b[13] == 0x00 && b[14] == 0xC0 && b[15] == 0x3F); // 1.5f
		CHECK(b[16] == 200 && b[17] == 1);
	}
	{
		FileIO in(CString("%s", path.c_str()), "rb");
		CHECK(in.getInt() == -2);
		CHECK(in.getUInt() == 65535u);
		CHECK(in.getLong() == -3);
		CHECK(in.getULong() == 4000000000u);
		CHECK(in.getFloat() == 1.5f);
		CHECK(in.getUByte() == 200);
		CHECK(in.getBool() == true);
	}
	if (failures == 0)
		std::printf("ok: FileIO widths\n");
	return failures ? 1 : 0;
}
