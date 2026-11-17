// ROBO-233: a PNG decodes to what dkt uploads for a TGA of the same picture (size, bytes per pixel, bottom-up rows),
// so dktCreateTextureFromFile builds the same texture from either. No GL context needed.
#include "dkt_image.h"

#include <cstdio>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static int g_fail = 0;

static void check(bool ok, const char* what)
{
	if (!ok)
	{
		std::printf("FAIL: %s\n", what);
		g_fail = 1;
	}
}

// Rows given top to bottom; a TGA with the default bottom-left origin stores them bottom to top, as BGR(A)
static void writeTga(const std::string& path, int w, int h, int bpp, const std::vector<unsigned char>& topDown)
{
	FILE* f = std::fopen(path.c_str(), "wb");
	unsigned char header[18] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								(unsigned char)(w & 255), (unsigned char)(w >> 8), (unsigned char)(h & 255), (unsigned char)(h >> 8),
								(unsigned char)(bpp * 8), (unsigned char)(bpp == 4 ? 8 : 0)};
	std::fwrite(header, 1, sizeof(header), f);
	for (int y = h - 1; y >= 0; --y)
		for (int x = 0; x < w; ++x)
		{
			const unsigned char* p = &topDown[size_t((y * w + x) * bpp)];
			unsigned char bgra[4] = {p[2], p[1], p[0], bpp == 4 ? p[3] : (unsigned char)0};
			std::fwrite(bgra, 1, size_t(bpp), f);
		}
	std::fclose(f);
}

static void sameAsTga(const std::string& dir, const char* name, int w, int h, int bpp, const std::vector<unsigned char>& topDown)
{
	const std::string tga = dir + "/" + name + ".tga", png = dir + "/" + name + ".PNG"; // upper case: any case works
	writeTga(tga, w, h, bpp, topDown);
	stbi_write_png(png.c_str(), w, h, bpp, topDown.data(), w * bpp);
	bv2::DecodedImage a, b;
	check(bv2::decodeImageFile(tga.c_str(), a), "the TGA didn't decode");
	check(bv2::decodeImageFile(png.c_str(), b), "the PNG didn't decode");
	check(a.width == w && a.height == h && a.bytesPerPixel == bpp, "TGA size or bytes per pixel");
	check(b.width == a.width && b.height == a.height && b.bytesPerPixel == a.bytesPerPixel, "PNG size or bytes per pixel differs from the TGA");
	check(b.pixels == a.pixels, "PNG pixels differ from the TGA (row order or channel order)");
	// Bottom-up: the first decoded row is the picture's bottom row
	check(a.pixels.size() >= size_t(bpp) && a.pixels[0] == topDown[size_t((h - 1) * w * bpp)], "TGA rows are not bottom-up");
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::printf("usage: test_dkt_image <scratch dir>\n");
		return 2;
	}
	const std::string dir = argv[1];

	// 3x2 RGB, every pixel different
	sameAsTga(dir, "rgb", 3, 2, 3, {255, 0, 0, 0, 255, 0, 0, 0, 255, 10, 20, 30, 40, 50, 60, 70, 80, 90});
	// 2x2 RGBA with partial alpha
	sameAsTga(dir, "rgba", 2, 2, 4, {255, 0, 0, 0, 0, 255, 0, 64, 0, 0, 255, 128, 1, 2, 3, 255});

	// Grey PNG: RGB with the grey in each channel; grey + alpha: RGBA
	const unsigned char grey[4] = {0, 85, 170, 255};
	const std::string greyPath = dir + "/grey.png";
	stbi_write_png(greyPath.c_str(), 2, 2, 1, grey, 2);
	bv2::DecodedImage g;
	check(bv2::decodePng(greyPath.c_str(), g) && g.bytesPerPixel == 3, "grey PNG is not RGB");
	check(g.pixels.size() == 12 && g.pixels[0] == 170 && g.pixels[1] == 170 && g.pixels[2] == 170, "grey PNG bottom-left pixel");
	const unsigned char greyAlpha[2] = {200, 100};
	const std::string gaPath = dir + "/grey-alpha.png";
	stbi_write_png(gaPath.c_str(), 1, 1, 2, greyAlpha, 2);
	bv2::DecodedImage ga;
	check(bv2::decodePng(gaPath.c_str(), ga) && ga.bytesPerPixel == 4 && ga.pixels.size() == 4 && ga.pixels[3] == 100,
		  "grey + alpha PNG is not RGBA");

	// Failures: missing file, not a PNG, unknown extension
	bv2::DecodedImage none;
	check(!bv2::decodeImageFile((dir + "/missing.png").c_str(), none), "a missing PNG decoded");
	check(!bv2::decodeImageFile((dir + "/missing.tga").c_str(), none), "a missing TGA decoded");
	const std::string junk = dir + "/junk.png";
	FILE* f = std::fopen(junk.c_str(), "wb");
	std::fputs("not a png at all", f);
	std::fclose(f);
	check(!bv2::decodeImageFile(junk.c_str(), none), "a file that isn't a PNG decoded");
	check(!bv2::decodeImageFile((dir + "/rgb.bmp").c_str(), none), "a .bmp decoded");
	const std::string shortTga = dir + "/short.tga";
	f = std::fopen(shortTga.c_str(), "wb");
	std::fputs("tiny", f);
	std::fclose(f);
	check(!bv2::decodeImageFile(shortTga.c_str(), none), "a TGA shorter than its header decoded");
	check(!bv2::decodeImageFile(nullptr, none), "a null path decoded");

	if (!g_fail)
		std::printf("ok: PNG decodes like TGA (RGB, RGBA, grey), failures rejected\n");
	return g_fail;
}
