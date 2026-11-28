# Robo Violence 2 game

The language of the game server and the players in it.

## Language

### Players

**Player**:
Anyone in a player slot (0-31) of a game server, human or bot.
_Avoid_: Client, connection

**Human player**:
A player behind a network connection: a person running the game client.
_Avoid_: Real player, user

**Bot**:
A player the server runs itself, with no connection; its name ends in ` [bot]`, the tag drawn in cyan.
_Avoid_: AI, NPC, fake player

**Target population**:
How many players on blue and red the server aims for (`sv_bots`); bots fill what humans on a team leave free.
_Avoid_: Bot count, minimum players

**Bot roster**:
The list of bot names (`content/bots/names.txt`); a new bot takes a random name no bot has. A bot keeps its roster index, which picks its main weapon and skin.
_Avoid_: Bot list

### Bot behaviour

**Tactical state**:
What a bot is doing now: wander, attack, attack objective, capture, escort, recover or retreat.
_Avoid_: Mode, behaviour

**CTF role**:
A bot's job in its team's capture-the-flag plan: attacker, defender, escorter, recoverer, interceptor or carrier.
_Avoid_: Class, position

**Brain**:
A bot's memory between frames: state, role, goal, enemy memory, aim and stuck recovery.
_Avoid_: AI, controller

### Assets, modes and fronts

**Asset id**:
The stable name code uses to ask for an asset; the pack manifest maps it to a file (ADR 0062).
_Avoid_: Asset path, file name

**Asset pack**:
A folder of assets with one manifest listing each asset's id, file, type, author and licence. Packs stack: a later pack overrides or adds ids.
_Avoid_: Data dir, mod

**Base pack**:
The asset pack the game ships with and always loads first.
_Avoid_: Default assets, original data

**Default art pack**:
The Robo Violence 2 art pack the client downloads and stacks over the base pack (`robo_art/`, ADR ROBO-260); what players see unless they choose a theme pack.
_Avoid_: Original data, BV2 data

**Theme pack**:
A player-chosen asset pack stacked over the default art pack, client-side only, that replaces some of its ids: a theme, an event (for example Christmas skins), a test or a music pack (ROBO-280).
_Avoid_: Mod, skin pack, data dir

**BV2 data**:
The original BaboViolent 2 assets, maps and config. Unsupported (ADR ROBO-278): no code, test or doc is added for it; its files stay blocked by hash.
_Avoid_: Original data (as something the game uses), game assets

**Mode**:
A rule set for a match (`GAME_TYPE_*` in `game/src/Game.h`), selected by `sv_gameType`.
_Avoid_: Game type (in prose); ruleset means `sv_serverType` (Normal, Pro)

**Mini game**:
A package of manifest, asset pack, rules and maps that runs on the game server as its own mode.
_Avoid_: Mod, plugin

**Front**:
Any program that speaks the protocol to a game server and shows or drives a match: the game client, a test client, a web client.
_Avoid_: Client (when not the game client)

**Protocol version**:
The number a server and a front compare on connect (`GAME_VERSION_SV/CL`) to know they can talk.
_Avoid_: Game version, build version

### Maps

**Map look**:
One of the visual sets a map is drawn with: Wasteland, Foundry, Winter Coming or Scrap Deck. It picks the floor, wall pieces, decals and wall-top props.
_Avoid_: Theme (that is a theme pack), map type, biome, Cryo Yard

**Wall piece**:
The art picked for one wall face from its neighbours and its place in a wall run: corner, end, T-join, plain run, pillar or panel.
_Avoid_: Wall tile, wall texture

**Decal**:
A flat mark laid on the floor or on a wall: a crack, an oil stain, a grate. Visual only.
_Avoid_: Sticker, splat (the editor's splat tool paints dirt)

**Spray**:
A decal of text painted on a wall face, a line from the spray list. Rare by design.
_Avoid_: Graffiti, tag

**Wall-top prop**:
A visual-only object on top of a wall (pipes, vents, cables, sandbags, snow). It never blocks shots or movement and never hides a player or a flag.
_Avoid_: Decoration, prop (alone), doodad

**Decoration override**:
An editor choice stored in the map that replaces what the game would pick on its own for one cell (wall piece, decal, spray or prop). Without one, the game decides from the cell pattern and the map.
_Avoid_: Manual tile, paint layer
