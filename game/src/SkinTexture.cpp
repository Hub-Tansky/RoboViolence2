// ROBO-258: full-colour skins with a team mask beside the BV2 colour-mask skins. Images are decoded on the CPU
// (dkt_image), not read back from OpenGL, so the same code serves any GL flavour.
#include "SkinTexture.h"

#include "AssetRegistry.h"
#include "Skins.h"
#include "dkt.h"
#include "dkt_image.h"

#include <map>
#include <utility>
#include <string>

namespace
{
	struct FullSkin
	{
		bool ok = false;
		bv2::DecodedImage colour;
		bv2::DecodedImage mask;
	};

	// Decoded once per name: a ball repaints on every spawn and team change. Only skins that loaded are kept, so a
	// pack stacked later (the art pack, ADR ROBO-260) still shows.
	const FullSkin& fullSkin(const std::string& name)
	{
		static std::map<std::string, FullSkin> cache;
		static const FullSkin none;
		auto it = cache.find(name);
		if (it != cache.end())
			return it->second;
		const std::string colourId = bv2::skins::colourId(name), maskId = bv2::skins::maskId(name);
		// only ids a pack lists: an unknown id would print a warning per name a peer sends
		if (!bv2::assetKnown(colourId.c_str()) || !bv2::assetKnown(maskId.c_str()))
			return none;
		FullSkin skin;
		if (!bv2::decodeImageFile(bv2::assetPath(colourId.c_str()), skin.colour) ||
			!bv2::decodeImageFile(bv2::assetPath(maskId.c_str()), skin.mask) || skin.colour.width != skin.mask.width ||
			skin.colour.height != skin.mask.height || skin.colour.bytesPerPixel < 3)
			return none;
		skin.ok = true;
		return cache.emplace(name, std::move(skin)).first->second;
	}

	void toFloats(const CColor3f& c, float out[3])
	{
		out[0] = c[0];
		out[1] = c[1];
		out[2] = c[2];
	}
}

namespace bv2
{
	void paintSkinTexture(unsigned int* texture, const CString& name, const CColor3f& maskColour, const CColor3f& red,
						  const CColor3f& green, const CColor3f& blue)
	{
		const std::string wanted = name.s ? name.s : "";
		const std::string valid = skins::validName(wanted) ? wanted : skins::DEFAULT_SKIN;
		const FullSkin& full = fullSkin(valid);
		if (full.ok)
		{
			std::vector<unsigned char> pixels = full.colour.pixels;
			float colour[3];
			toFloats(maskColour, colour);
			skins::paintMask(pixels, full.colour.bytesPerPixel, full.mask.pixels, full.mask.bytesPerPixel, full.colour.width,
							 full.colour.height, colour);
			dktCreateTextureFromBuffer(texture, pixels.data(), full.colour.width, full.colour.height, full.colour.bytesPerPixel,
									   DKT_FILTER_BILINEAR);
			return;
		}
		DecodedImage legacy;
		std::string id = skins::legacyId(skins::legacyName(valid));
		if (!assetKnown(id.c_str())) // a valid name that isn't a BV2 skin either
			id = skins::legacyId(skins::DEFAULT_SKIN);
		if (!decodeImageFile(assetPath(id.c_str()), legacy) || legacy.width <= 0 || legacy.height <= 0 || legacy.bytesPerPixel < 3)
		{
			legacy.width = 64;
			legacy.height = 32;
			legacy.bytesPerPixel = 3;
			legacy.pixels.assign(size_t(64) * 32 * 3, 0);
		}
		float r[3], g[3], b[3];
		toFloats(red, r);
		toFloats(green, g);
		toFloats(blue, b);
		skins::paintLayers(legacy.pixels, legacy.bytesPerPixel, r, g, b);
		dktCreateTextureFromBuffer(texture, legacy.pixels.data(), legacy.width, legacy.height, legacy.bytesPerPixel,
								   DKT_FILTER_BILINEAR);
	}

	bool fullColourSkinsInstalled()
	{
		return fullSkin(skins::FACES[0]).ok;
	}
}
