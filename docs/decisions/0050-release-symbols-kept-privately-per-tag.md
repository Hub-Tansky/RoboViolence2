# 0050: Keep release debug symbols privately, one release per tag in a private repo

- Status: Proposed
- Date: 2026-11-10
- Affects: ROBO-188, `CMakeLists.txt`, `packaging/split-symbols.py`, `.github/workflows/build.yml`, `game/src/CrashReport.cpp`, `docs/ops/runbook.md`

## Context
- Crash reports (ADR 0048) give module + offset per frame. Without the symbols of that exact build they can't be resolved, and release packages carry none.
- Owner (2026-11-07): the release pipeline keeps symbols privately: Windows PDBs, macOS dSYMs, Linux debug files with build IDs.
- Workflow artifacts of a public repo can be downloaded by any signed-in GitHub user, so they are not private. The public release is not either.

## Decision
- Release builds compile with debug info: `-g` (GCC, Clang); MSVC `/Z7` objects and `/DEBUG /OPT:REF /OPT:ICF`, which writes `<name>.pdb` next to each binary. Linux links with `--build-id`.
- `packaging/split-symbols.py` runs before `make-package.py` and moves the symbols of `bv2`, `bv2dedicated` and `bv2master` into `roboviolence2-<preset>-<version>-symbols.zip`: `.pdb`; `.dSYM` (`dsymutil`, then `strip -S`); Linux `.build-id/<xx>/<rest>.debug` (`objcopy --only-keep-debug`, `--strip-debug --add-gnu-debuglink`). Packages stay stripped.
- On a `v*` tag, each `build` job uploads its zip to a release named after the tag in the private repo `Hub-Tansky/rv2-symbols`, with the secret `SYMBOLS_TOKEN` (fine-grained, Contents write on that repo only). Other runs split the symbols and drop them.
- macOS crash reports add `load address` (`_dyld_get_image_header(0)`), the main image's ASLR slide for `atos -l`.
- Resolving a report: `docs/ops/runbook.md` "Crash reports".

## Consequences
- The owner creates `Hub-Tansky/rv2-symbols` and the `SYMBOLS_TOKEN` repo secret once; a tag build fails at "Store symbols privately" without them.
- Release builds take longer to link, and `dsymutil` adds about a minute on macOS.
- The `.exe` keeps the path of its `.pdb` from the runner (`D:\a\...`), which is not private.
- Rejected: Actions artifacts (public to signed-in users), a cloud bucket (an account and keys for one zip per tag), and Sentry debug files (ROBO-187, not adopted; it can upload the same zip later).
