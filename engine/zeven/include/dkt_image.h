// Image files decoded to the pixel rows that dkt hands to OpenGL (ROBO-233, ADR 0010: new formats beside the old).
// Rows run bottom to top and channels are RGB or RGBA, the layout the BV2 TGA loader always produced.
#pragma once

#include <vector>

namespace bv2
{
	struct DecodedImage
	{
		int width = 0;
		int height = 0;
		int bytesPerPixel = 0; // 3 (RGB) or 4 (RGBA)
		std::vector<unsigned char> pixels;
	};

	// The BV2 Targa reader, unchanged: 24 or 32 bit, red and blue swapped to RGB(A), rows as stored in the file
	bool decodeTga(const char* path, DecodedImage& out);

	// PNG through stb_image: RGBA when the file has alpha, otherwise RGB, flipped to bottom-up rows like a TGA
	bool decodePng(const char* path, DecodedImage& out);

	// Picks the decoder from the last three letters (tga or png, any case); false for anything else or an unreadable file
	bool decodeImageFile(const char* path, DecodedImage& out);
}
