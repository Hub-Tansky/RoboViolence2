Game data goes in main/. Tracked here: languages/, LaunchScript/, bots/, maps/ and base-pack.tsv.
base-pack.tsv is the base pack manifest (ADR 0065): one row per asset the game loads, with its id, file, author and licence.
maps/ holds the game's own maps (made by the owner, GPL-3.0 like the code); the build copies them into main/maps.
The original assets are not part of this repository; see README.md.
