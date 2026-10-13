# Step 12 review

Reviewer: fresh-context subagent, /thermo-nuclear-code-quality-review, 2026-10-12
Diff: ac296ba...e4d6921
Skill evidence: `SKILL: Launching skill: thermo-nuclear-code-quality-review` (raw report: [step12-report.md](step12-report.md); its example IPs are replaced with documentation IPs for the secret scan)

## review.sh

`tools/review.sh --full macos-arm64` and `--full macos-arm64-asan` at 00a40d2: see the step commit that closes this record. CI `containers` checks the remote `status` and a command over TLS through Caddy.

## Findings

| # | Finding | Outcome |
|---|---|---|
| 1 | `restart`/`disconnect` over the API freed the server under the request, unaudited | fixed in 130ef76: refused (also `host`, `connect`), audited before running; test |
| 2 | Unbounded connections, 5 s each | fixed in 130ef76 (8, 1 s), 00a40d2 (Caddy body cap and read timeouts) |
| 3 | `X-Forwarded-For` trusted from any peer, leftmost | fixed in 130ef76 (`zsv_adminApiProxy` only, rightmost); test |
| 4 | Budget map grows forever | fixed in 130ef76 (full entries expire) |
| 5 | Audit log forgeable with a bare newline | fixed in 130ef76 (line cut at `\r` or `\n`; control characters cleaned); tests |
| 6 | Runbook audit path wrong | fixed in 00a40d2 |
| 7 | Legacy `RA_*` code left dead | PNS-46 |
| 8 | `close()` every frame when off | fixed in 130ef76 (wanted port and address) |
| 9 | `SO_REUSEADDR` on Windows | fixed in 130ef76 (`SO_EXCLUSIVEADDRUSE`) |
| 10 | Request framing | fixed in 130ef76 (head-only headers, 411 for `Transfer-Encoding`, bounded `Content-Length`, full send, timeouts audited), 00a40d2 (CI command through Caddy) |
| 11 | Docs drift | fixed in 130ef76 (`Served` struct), 00a40d2 (ADR variable name); env lists in AGENTS.md and config/README.md → PNS-46 |
| 12 | `long long` socket casts | fixed in 130ef76 (`std::uintptr_t`, one cast helper) |
| 13 | Test gaps | fixed in 130ef76 (split, 413, 404, timeout, XFF, refused commands); Windows socket branch has no test (POSIX test) |
| 14 | 46c865e combines 12.2 and 12.3; Caddy as root, tag pin; CI readiness | commits rejected (pushed history); Caddy noted; CI retry fixed in 00a40d2 |

## Fix proof

With each mechanism removed, `admin_api` fails:

- token check: `:62-63`
- budget: `:94`
- masking: `:103`
- `X-Forwarded-For` peer check: `:145`
- control-character cleaning: `:146`
- timeout audit: `:143`
- refusal: ASan heap-use-after-free on `restart`

## Not done / unsure

- Windows: the socket code is compiled by CI, but no test exercises it there.
- Caddy's certificate and `X-Forwarded-For` behaviour are checked only through CI's compose run.
