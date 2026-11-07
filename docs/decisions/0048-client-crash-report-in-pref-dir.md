# 0048: Write client crash reports as a text file in the pref dir

- Status: Accepted
- Date: 2026-11-07
- Affects: ROBO-186, `game/src/CrashReport.cpp`, `game/src/main.cpp`, `.github/ISSUE_TEMPLATE/bug_report.yml`

## Context
- Releases v0.0.1 to v0.0.3 are public. A client crash on a player's machine leaves no trace; the only channel is Discord. The server side already has `tools/staging/health-watch.sh` with ntfy alerts (`tools/staging/deploy.sh`, `docs/ops/runbook.md`).
- The client has no crash handler: `grep -rniE 'sentry|crashpad|breakpad|minidump' game/src engine vcpkg.json CMakeLists.txt` found nothing at da1e6f5.
- The pref dir already holds `console.log` and the bug report form already points players there (ADR 0007).
- Sending anything off the player's machine needs a privacy policy update first (ADR 0034), and a DSN or upload key would be a secret kept out of the repo.

## Decision
The client installs a crash handler at start (`crashreport::install` in `main.cpp`) that writes `crash-<start time>.txt` to the pref dir: version, signal or exception code, and a stack trace as module + offset. Nothing is uploaded; the bug report form has an optional "Crash dump" field for it, pointing to private vulnerability reporting when the player prefers not to post it publicly. Opt-in Sentry upload is ROBO-187; keeping release symbols privately is ROBO-188. The Debug build has a console command `crash` that crashes the client on purpose.

## Consequences
- Players and testers get a file to attach; we resolve its offsets against the same build (`addr2line -Cfe bv2 <offset>` on Linux, `atos` on macOS, the `.pdb` on Windows). Release builds have no symbols in the package; ROBO-188 keeps them privately per tag.
- The report holds file paths of the executable, which can include the user's folder name. The form (owner's wording) warns that a dump can hold private data and offers private vulnerability reporting instead.
- The handler uses only async-signal-safe calls on Linux and macOS, then restores the previous handler (the default action, or ASan's report in the `-asan` presets) and re-raises, so the process dies as before. On Windows the filter then calls the filter it replaced (the MSVC ASan runtime's in the `-asan` preset) or returns `EXCEPTION_CONTINUE_SEARCH`, so Windows error reporting still runs.
- Not covered: crashes before the pref dir exists, stack overflows (no alternate signal stack), MSVC `abort()`, and the dedicated server (health-watch covers it).
- Rejected: `sentry-native` from vcpkg. It uploads automatically, but needs a Sentry account, a DSN secret, player consent and a privacy policy update, and adds a port to three platform builds. It can sit on top of this handler later if crash volume justifies it.
- Rejected: a Windows minidump (`MiniDumpWriteDump`). Its stack memory can hold chat text or passwords, and players attach reports to public issues.
