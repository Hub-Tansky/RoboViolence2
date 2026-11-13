"""The shipped defaults (config/bv2.example.cfg) start fullscreen at the desktop size and name a new player as the game
does (GameVar.cpp), not as the upstream developer's own config did ("Daivuk")."""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
example = re.search(r'^cl_playerName "(.*)"$', (root / "config/bv2.example.cfg").read_text(), re.M).group(1)
builtin = re.search(r'cl_playerName = "(.*)";', (root / "game/src/GameVar.cpp").read_text()).group(1)
if example != builtin:
    sys.exit(f'FAIL bv2.example.cfg names new players "{example}", the game "{builtin}"')
print("ok: example config player name")

# A new player starts fullscreen at the desktop resolution (r_resolution 0 0, dkwInit)
text = (root / "config/bv2.example.cfg").read_text()
for line in ("r_fullScreen true", "r_resolution 0 0"):
    if not re.search(rf"^{line}$", text, re.M):
        sys.exit(f'FAIL bv2.example.cfg lacks "{line}"')
print("ok: example config starts fullscreen at the desktop resolution")

# The owner's defaults for every server and the Host tab (ROBO-212, ADR 0053): the example config and each launch script
defaults = ("sv_serverType 1", "sv_spawnType 0", "sv_winLimit 7", "sv_friendlyFire true", "sv_reflectedDamage false",
            "sv_forceRespawn false", "sv_enableNuclear false", "sv_enableMinibot false", "sv_ftMaxRange 5")
for line in defaults:
    if not re.search(rf"^{line}$", text, re.M):
        sys.exit(f'FAIL bv2.example.cfg lacks "{line}"')
servers = {"FFA": ('"Deathmatch ring"', 0, 3335, 12), "CTF": ('"Get that flag."', 2, 3333, 10),
           "TDM": ('"Team Deathmatch"', 1, 3334, 10)}
for script, (name, game_type, port, players) in servers.items():
    lines = (root / f"content/LaunchScript/{script}.cfg").read_text()
    for line in (*defaults, f"sv_gameName {name}", f"sv_gameType {game_type}", f"sv_port {port}", f"sv_maxPlayer {players}"):
        if not re.search(rf"^set {re.escape(line)}$", lines, re.M):
            sys.exit(f'FAIL {script}.cfg lacks "set {line}"')
print("ok: example config and launch scripts carry the server defaults")
