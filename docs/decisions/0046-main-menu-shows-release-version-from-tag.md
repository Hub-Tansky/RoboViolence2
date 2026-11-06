# 0046: The main menu shows the release version from the tag, not the protocol number

- Status: Accepted
- Date: 2026-11-06
- Affects: ROBO-157, `CMakeLists.txt`, `game/CMakeLists.txt`, `game/src/Scene.cpp`, `game/src/GameVar.cpp`, `content/languages/en.lang`, `.github/workflows/build.yml`, `packaging/windows/bv2.rc.in`

## Context
- The menu printed the protocol number 22100 as "client version 2.21.00" (`game/src/Client.h:22`), so a player could not see the installed release.
- The number must match the server's to join (`game/src/ClientRecv.cpp:286-309`), so it cannot follow releases.
- The tag reached only the Package step of `.github/workflows/build.yml`, after build and test.
- An original BV2 `.lang` keeps the old keys' three-int format, so passing them a string is undefined behaviour.

## Decision
- The owner approved this **[GUI]** exception to ADR 0010 on 2026-11-06. The client and the server case show the same text at the same position and size.
- `BV2_VERSION` (CMake cache variable, seeded from the environment, default `0.0.0-dev`) carries the tag. A leading `v` is removed, as `packaging/make-package.py:116` does. Any tag shows in full, with no format check. The `build` job sets it, and the owner approved that `.github/` change on 2026-11-06.
- A generated `ReleaseVersion.h` hands it to `Scene.cpp`, which draws it through the new key `lang_version` (`version: %s`). The two old keys are removed.
- `GAME_VERSION_CL` and `GAME_VERSION_SV` stay the compatibility gate, unchanged.

## Consequences
- Players see the number drop from 2.21.00 to 0.0.x. The join console lines (`game/src/ClientRecv.cpp:294,307`) still print the protocol number, which is correct.
- A build from the source tarball shows `0.0.0-dev` unless given `-DBV2_VERSION=...`. The environment seeds the cache on the first configure only.
- Old `.lang` files still load, because unregistered keys are ignored. A `.po` carrying the old keys fails `tools/lang-check.py`.
- Proof that the tag reaches the binary is `ctest client_version`, because the screenshot tolerance is larger than the whole line.
- The five menu goldens were regenerated on Linux in this PR (ADR 0026); owner's choice.
- Rejected: reusing the old key with a string (undefined behaviour on original `.lang` files).
- Rejected: `git describe` or a hash (the source tarball has no `.git`, and an untagged build shows `0.0.0-dev`).
- Rejected: a target-wide compile definition (it recompiles every source of `bv2`, and string quoting differs per generator).
