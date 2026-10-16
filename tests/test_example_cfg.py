"""The shipped defaults (config/bv2.example.cfg) name a new player as the game does (GameVar.cpp), not as the
upstream developer's own config did ("Daivuk")."""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
example = re.search(r'^cl_playerName "(.*)"$', (root / "config/bv2.example.cfg").read_text(), re.M).group(1)
builtin = re.search(r'cl_playerName = "(.*)";', (root / "game/src/GameVar.cpp").read_text()).group(1)
if example != builtin:
    sys.exit(f'FAIL bv2.example.cfg names new players "{example}", the game "{builtin}"')
print("ok: example config player name")
