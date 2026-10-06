# Step 0: Supply-chain security

**Status:** TODO

**Depends on:** Phase A. **Next:** [step1-playtest-builds-and-build-guides.md](step1-playtest-builds-and-build-guides.md). **Index:** [README.md](README.md)

## Scope

| Field | Value |
|---|---|
| Goal | GitHub tracks the vcpkg dependencies, alerts on vulnerable Actions, and scans the code with CodeQL and OSV-Scanner on every PR |
| In scope | Tasks 0.1–0.5 below |
| Out of scope | Fixing CodeQL findings (steps 4–7, or `possible-new-scope.md`); making CodeQL a required check before its alerts are triaged; vcpkg baseline bumps beyond what a finding needs |
| Allowed paths | `.github/workflows/**`, `.github/dependabot.yml`, `AGENTS.md`, `ARCHITECTURE.md`, `docs/roadmap/**` |
| Inputs | `AGENTS.md`, `ARCHITECTURE.md`, `vcpkg.json`, `.github/workflows/build.yml`, [Phase A step 3](../phase-a-modern-portable-build/step3-dependency-upgrades.md) |
| Deliverables | Workflows `dependency-graph.yml`, `codeql.yml`, `osv-scanner.yml`; `.github/dependabot.yml`; repository settings on; `ARCHITECTURE.md` facts current |
| Definition of done | See "Acceptance checks". All must pass. `ARCHITECTURE.md` is updated |

## Context

- GitHub can't parse `vcpkg.json`. vcpkg can submit its resolved ports through the dependency submission API (`VCPKG_FEATURE_FLAGS=dependencygraph` with `GITHUB_TOKEN`).
- The GitHub Advisory Database has no C/C++ ecosystem, so Dependabot alerts cover GitHub Actions only. vcpkg ports are covered by OSV-Scanner (partial C/C++ coverage) and a manual monthly baseline bump.
- `ARCHITECTURE.md` still says "State after Step 3", protocol 21100 (the code has `GAME_VERSION_SV 22000`, `game/src/Server.h:32`) and "Linux and Windows are untested" (PNS-11).

## Tasks

### 0.1 Repository settings (owner confirms first)

- As `Hub-Tansky`, under Settings → Advanced Security: turn on the dependency graph, Dependabot alerts and Dependabot security updates.
- Record the settings in the PR description. No secrets are involved.

### 0.2 Dependency submission

- `.github/workflows/dependency-graph.yml` runs on push to `main`: configure `linux-x64` with `VCPKG_FEATURE_FLAGS=dependencygraph` and `permissions: contents: write`.

### 0.3 Dependabot and OSV-Scanner

- `.github/dependabot.yml`: `github-actions` ecosystem, weekly.
- `.github/workflows/osv-scanner.yml`: `google/osv-scanner-action` reusable workflow on PRs and weekly, uploading SARIF to code scanning. List the vcpkg ports it resolves in `ARCHITECTURE.md`.

### 0.4 CodeQL

- `.github/workflows/codeql.yml` (advanced setup): `c-cpp` with a manual build of `linux-x64` (`bv2`, `bv2dedicated`, `bv2master`), plus `actions`. Runs on push, PR and weekly. Not a required check.
- Triage the first run: each alert is fixed later by step 4–7 or listed in `possible-new-scope.md`.

### 0.5 Docs

- `AGENTS.md` CI list; `ARCHITECTURE.md` summary (PNS-11 facts, new workflows, the monthly baseline-bump rule).

## Critical files

- `.github/workflows/build.yml` (vcpkg setup to reuse)
- `vcpkg.json`, `CMakePresets.json`

## Acceptance checks

- Insights → Dependency graph lists the vcpkg ports (sqlite3, sdl3, miniaudio, stb).
- The Security tab shows CodeQL (`c-cpp`, `actions`) and OSV-Scanner results for `main`.
- `.github/dependabot.yml` is valid: Insights → Dependency graph → Dependabot shows no errors.
- `ARCHITECTURE.md` names protocol 22000 and no longer says "State after Step 3".
- `ARCHITECTURE.md` lists every file added, moved or removed in this step.
- The secret scan (`gitleaks git --staged` / CI) is clean.
