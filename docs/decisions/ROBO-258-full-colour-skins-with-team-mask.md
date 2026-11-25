# ROBO-258: Full-colour ball skins with a team mask, beside the BV2 skins

- Status: Accepted
- Date: 2026-11-25
- Affects: `game/src/Skins.h`, `game/src/SkinTexture.cpp`, `game/src/Player.cpp` (`updateSkin`), `game/src/CUserLogin.cpp`, `game/src/Bot.cpp`, `game/src/Server.cpp`, `game/src/ArtPack.cpp` (pin), `tests/art_pack_fixture.py`; ROBO-258 (owner decisions 2026-11-24)

## Context
- BV2 skins are 64x32 colour masks (`main/skins/skinNN.tga`). The player's three layer colours (or the team's) replace the whole ball, so no painted detail survives.
- The new skins (art pack 2) have faces, chrome armour and a red team band. The owner kept the faces in their own colours on both teams.
- Ground rule 1 (ADR 0010): BV2 skins keep loading and look as before. Packets carry a skin name in 7 bytes (`netPacket.h`).

## Decision
- **Format:** two art pack ids per skin. `skins/<name>` is a 512x256 RGB equirectangular PNG, the same unwrap `gluSphere` uses for the BV2 skins. `skins/<name>_mask` is a same-size greyscale PNG, white where team colour goes (band and armour panels, never the face). Both are rows in the art pack's `pack.tsv`, not files in the base pack.
- **Names:** `faceNN` for the 17 approved faces. Face 08 is `skin10`, so the default `cl_skin` and older clients keep working. Names fit the 6-character packet field.
- **Colour:** a masked pixel keeps its brightness relative to the paint red (232, 52, 58) and takes the colour, as the art export made the blue previews (`design/tools/export_assets.py` `recolour`). Team games with `cl_teamIndicatorType 0` use team red (232, 52, 58) or blue (47, 123, 255). Otherwise the mask takes the player's layer 1 colour, so the colour sliders still do something. Unmasked pixels are drawn as painted.
- **Fallback:** without both files (no art pack, `--placeholders`), the BV2 skin is drawn with the old layer recolour: `faceNN` shows `skinNN` (wrapping past 23), a BV2 name shows itself. A name that isn't 1 to 6 lower-case letters or digits gets `skin10`, so wire text never builds a path.
- **Decoding:** skins are decoded on the CPU (`dkt_image`) and uploaded once per paint. There is no `glGetTexImage` readback, which OpenGL ES and WebGL lack. That keeps a future web build on the same code; there is no web build yet.
- **Picker:** with the faces installed, the Appearance slider lists the 17 faces, then the BV2 skins, with `skin10` once. Without them, it lists `skin01` to `skin23` as before. The preview has no team, so the mask shows layer 1.
- **Bots:** a new server bot gets a random face no other bot wears (any face once all 17 are worn).

## Consequences
- The art pack pin moves to `rv2-art-2`. Installed packs show the splash once while it downloads (ADR ROBO-260).
- Full-colour skins live only in the art pack, so the GPL game ships no new art. The fixture art pack has one full-colour `skin10` with a band and an unmasked stripe; the `art-pack-appearance` golden checks that layer 1 lands on the mask only.
- Players without the art pack see bots in BV2 skins. Players with it see faces, also for players who pick BV2 skins.
- Rejected: a team-colour alpha channel (an RGBA skin can't be checked by eye as easily as a separate mask); recolouring the whole ball as BV2 does (the faces would be lost).
