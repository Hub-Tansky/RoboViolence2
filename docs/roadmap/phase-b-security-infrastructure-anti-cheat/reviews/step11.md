# Step 11 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-12
Diff: bbe93ad...b839254
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step11-report.md](step11-report.md))

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at ec58e4a: all checks PASS. CI `containers` at dacf304: both images build, uid 10001, the compose example is healthy, and the master lists the passworded server at port 3333.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | The launch script wiped `BV2_SV_PASSWORD` and `BV2_ADMIN_PASS` | fixed in 25af7a1 (owner: `CTF.cfg`); CI sets a password and checks the listing |
| 2 | Healthchecks connected, joining a phantom player every 10 s | fixed in 14cdc3f (listening socket in `/proc/net/tcp`) |
| 3 | No signal handling as PID 1 | fixed in d240f69 (`init: true`; runbook `--init`) |
| 4 | Detach keys need a TTY | fixed in d240f69 (`tty: true`) |
| 5 | Runbook config syntax wrong | fixed in ec58e4a |
| 6 | Acceptance `id -u` line cannot work | fixed in 14cdc3f (`--entrypoint id`) |
| 7 | `.dockerignore` let local secrets into the build context | fixed in 14cdc3f |
| 8 | Dependencies rebuilt on every change | fixed in 14cdc3f (vcpkg install layer, shallow baseline fetch) |
| 9 | `d75c364` fails CI alone | rejected: pushed history not rewritten |
| 10 | No logs when `up --wait` fails | fixed in c0ba0c0, 2850944 (an always-run logs step) |
| 11 | GHCR push hygiene | fixed in c0ba0c0 (own job, login-action), 14cdc3f (OCI label) |
| 12 | Base image patching | fixed in c0ba0c0 (weekly run), 14cdc3f (Dependabot docker; owner) |
| 13 | GL in the master image | fixed in 14cdc3f |
| 14 | Entrypoint one-liner | fixed in 14cdc3f (`master-entrypoint.sh`) |
| 15 | "Idles at EOF" note wrong | fixed in 14cdc3f |
| 16 | Docs gaps | fixed in ec58e4a; AGENTS CI list → PNS-45 |
| 17 | Unbounded logs | fixed in d240f69 |

CI then found a pre-existing bug: the master's struct copy wasn't packed, so it read every server's port one byte off (3333 became 13). It is fixed in dacf304 (owner), with asserts in both copies.

## Fix proof

- Before 25af7a1 and dacf304, the CI listing check found no game at port 3333. In run 38242719558 the row had the server container's address and port 13. After them it passes.

## Not done / unsure

- Acceptance: the owner runs the compose example on Linux and joins from a client (owner decision: CI plus the Linux machine). The step stays IN PROGRESS until then.
- Not tried: arm64 images; GHCR push (main only).
