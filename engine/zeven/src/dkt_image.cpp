// ROBO-233: image decoding for dkt, apart from OpenGL so it can be tested headless.
#include "dkt_image.h"

#include "platform.h"

#include <cstdio>
#include <cstring>

// stb_image only for PNG; static so it can't clash with another copy in a linked library
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

namespace bv2
{
	bool decodeTga(const char* path, DecodedImage& out)
	{
		FILE* file = fopen(path, "rb");
		if (!file)
			return false;
		// 12 bytes of Targa header we skip, then width, height and bits per pixel. A file too short for it is not a
		// picture (the old reader went on with whatever was there).
		unsigned char header[18];
		if (!bv2ReadBytes(file, header, sizeof(header)))
		{
			fclose(file);
			return false;
		}
		out.width = header[13] * 256 + header[12];
		out.height = header[15] * 256 + header[14];
		out.bytesPerPixel = header[16] / 8;
		const size_t imageSize = size_t(out.width) * size_t(out.height) * size_t(out.bytesPerPixel);
		out.pixels.assign(imageSize, 0);
		bv2ReadBytes(file, out.pixels.data(), imageSize);
		fclose(file);
		// Targa stores BGR(A)
		if (out.bytesPerPixel >= 3)
			for (size_t i = 0; i + 2 < imageSize; i += size_t(out.bytesPerPixel))
			{
				unsigned char t = out.pixels[i];
				out.pixels[i] = out.pixels[i + 2];
				out.pixels[i + 2] = t;
			}
		return true;
	}

	bool decodePng(const char* path, DecodedImage& out)
	{
		FILE* file = fopen(path, "rb");
		if (!file)
			return false;
		int w = 0, h = 0, channels = 0;
		if (!stbi_info_from_file(file, &w, &h, &channels))
		{
			fclose(file);
			return false;
		}
		// Grey + alpha and RGBA keep their alpha
		const int wanted = (channels == 2 || channels == 4) ? 4 : 3;
		unsigned char* data = stbi_load_from_file(file, &w, &h, &channels, wanted);
		fclose(file);
		if (!data)
			return false;
		out.width = w;
		out.height = h;
		out.bytesPerPixel = wanted;
		const size_t row = size_t(w) * size_t(wanted);
		out.pixels.resize(row * size_t(h));
		// PNG rows run top to bottom; OpenGL and the TGA loader start at the bottom
		for (int y = 0; y < h; ++y)
			std::memcpy(out.pixels.data() + size_t(y) * row, data + size_t(h - 1 - y) * row, row);
		stbi_image_free(data);
		return true;
	}

	bool decodeImageFile(const char* path, DecodedImage& out)
	{
		// The last three letters, as dktCreateTextureFromFile always checked them
		const size_t n = path ? strlen(path) : 0;
		if (n >= 3 && strnicmp(path + n - 3, "tga", 3) == 0)
			return decodeTga(path, out);
		if (n >= 3 && strnicmp(path + n - 3, "png", 3) == 0)
			return decodePng(path, out);
		return false;
	}
}
