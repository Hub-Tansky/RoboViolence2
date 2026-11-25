// The OpenGL texture of one ball's skin (ROBO-258, ADR ROBO-258), for the game (Player) and the Appearance preview
// (CUserLogin)
#ifndef BV2_SKIN_TEXTURE_H
#define BV2_SKIN_TEXTURE_H

#include "CString.h"
#include "CVector.h"

namespace bv2
{
	// Paints skin `name` into *texture (replacing it). A full-colour skin from the art pack takes maskColour (0..1) where
	// its mask is set: the team colour, or the player's layer 1 colour outside team games. Without one (no art pack,
	// --placeholders, a BV2 name) the BV2 skin main/skins/<legacy name>.tga mixes the three layer colours, as before.
	// A skin that loads from neither leaves a 64x32 black texture.
	void paintSkinTexture(unsigned int* texture, const CString& name, const CColor3f& maskColour, const CColor3f& red,
						  const CColor3f& green, const CColor3f& blue);

	// True when the art pack holds the faces' full-colour skins, so the Appearance tab lists them
	bool fullColourSkinsInstalled();
}

#endif
