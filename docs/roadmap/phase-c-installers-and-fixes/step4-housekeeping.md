# Step 4: Build, CI and docs housekeeping

**Status:** DONE (2026-10-14)

**Depends on:** Phase B steps. **Next:** step 5 (after PNS-31). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | vcpkg ports are checked against OSV, the build no longer makes an unused `bv2.db` or compiles the server twice, and the docs match the code |
| In scope | Tasks 4.1–4.4 below |
| Out of scope | Pinning the Windows runner (PNS-27, step 3); making `containers` a required check (owner, branch rules); the master's other stale README lines beyond PNS-36 |
| Allowed paths | `tools/osv-vcpkg.py` (new), `.github/workflows/osv-vcpkg.yml` (new), `tools/CMakeLists.txt`, `tools/seed_db.cpp` and `CMakeLists.txt` (comments naming `bv2.db`), `game/CMakeLists.txt`, `tests/**`, `AGENTS.md`, `ARCHITECTURE.md`, `docs/analysis/**`, `masterserver/README.md`, `config/README.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, PNS-22, PNS-24, PNS-33, PNS-36, PNS-45, [ADR 0013](../../decisions/0013-r3-restore-normal-and-pro-rulesets.md) |
| Deliverables | OSV check script and weekly workflow; build changes; doc fixes |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- Dependabot and OSV can't read vcpkg purls (PNS-22). Each port's `vcpkg_installed/<triplet>/share/<port>/vcpkg.spdx.json` names the upstream repository and tag (e.g. `git+https://github.com/libsdl-org/SDL@release-3.4.16`); OSV matches a commit against its `GIT` ranges.
- `tools/CMakeLists.txt:10` had a rule for `runtime/bv2.db` (built only through the `bv2_seed_db` target, which nothing depended on); nothing reads it: `openClientDb` creates the pref-dir copy itself (`game/src/Paths.cpp`, PNS-24).
- `bv2server_lib` (tests only) and `bv2dedicated` compile the same `BV2_SERVER_SOURCES` (`game/CMakeLists.txt`, PNS-33).
- `AGENTS.md`, `docs/analysis/README.md` and `ALGORITHM_03` W1 describe R3 as current; `masterserver/README.md` says the master builds SQL with `sprintf` (PNS-36). `containers` is missing from the CI list in `AGENTS.md` (PNS-45).

## Tasks

### 4.1 OSV check for vcpkg ports (PNS-22)

- `tools/osv-vcpkg.py <vcpkg_installed dir>`: for each port's SPDX upstream `github.com/<owner>/<repo>@<ref>`, resolve the ref to a commit (`git ls-remote`, or the ref itself when it is a hash) and query `api.osv.dev` by commit. sqlite3 (a sqlite.org tarball) maps to `github.com/sqlite/sqlite` tag `version-<x.y.z>`. Prints each port and its result; exits 1 on any vulnerability, 2 when a port can't be checked.
- `osv-vcpkg.yml`: weekly and on demand, on `main`; configures `linux-x64` and runs the script. Not required.

### 4.2 Remove the runtime `bv2.db` (PNS-24)

- Drop the `bv2.db` rule and the `bv2_seed_db` target. `content-seed/bv2.sql` stays as the schema reference for `openClientDb`.

### 4.3 `bv2dedicated` from `bv2server_lib` (PNS-33)

- Build `bv2server_lib` whenever the server is built; `bv2dedicated` is `main.cpp` linked to it.

### 4.4 Docs (PNS-36, PNS-45)

- Point the R3 lines at ADR 0013; fix the master README's SQL line; add `containers` (not required) and the OSV workflow to the CI list in `AGENTS.md`; update the vcpkg alert line in `ARCHITECTURE.md`.

## Critical files

- `tools/osv-vcpkg.py`, `.github/workflows/osv-vcpkg.yml`, `tools/CMakeLists.txt`, `game/CMakeLists.txt`

## Acceptance checks

- `python3 tools/osv-vcpkg.py build/<preset>/vcpkg_installed` lists every port with a commit and an OSV result.
- `ctest --test-dir build/<preset> --output-on-failure` passes, including `smoke_server` and `server_harness`; no build rule for `runtime/bv2.db` remains.
- `tools/review.sh --full <preset>` passes, and the fresh-context review record `reviews/step4.md` and the raw reviewer report `reviews/step4-report.md` (with the skill's load line) exist (`REVIEW.md` section 5).
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
