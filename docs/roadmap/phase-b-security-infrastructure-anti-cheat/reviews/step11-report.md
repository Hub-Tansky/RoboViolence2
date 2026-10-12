SKILL: Launching skill: thermo-nuclear-code-quality-review

Review of `git diff bbe93ad...b839254` (Phase B step 11, PR #41). This was a read-only review. Docker was not run; the findings come from the files and the code they execute.

**Verdict: not ready.** Two findings break the step's goal or affect live play (1 and 2). Findings 3 to 5 make the runbook wrong. The Docker build itself looks sound: the package list, vcpkg at the baseline, the preset, `-DBV2_BUILD_CLIENT=OFF` (it turns off the default `client` feature), placeholder data and DB seeding (`add_dependencies` pulls in both), and `cp -rL` of the `main/` symlinks. The changed files stay inside the Allowed paths.

## Findings

**1. HIGH: the launch script wipes the secrets passed in the environment.**
- Where: `packaging/docker/Dockerfile:47` (`CMD ["CTF"]`), `compose.example.yml:18-19`, `docs/ops/runbook.md:18-26`.
- Problem: `main.cpp` applies the environment layer in `loadConfigLayers` (`Paths.cpp:265-268`) and then runs `execute CTF`. `content/LaunchScript/CTF.cfg` contains `set sv_password ""`, `set zsv_adminUser ""` and `set zsv_adminPass ""`, which go through the console `set`. "Transient" (`CSystemVariable.cpp:198`) only means "not saved"; it does not protect a variable from a later `set`.
- Effect: `BV2_SV_PASSWORD` and `BV2_ADMIN_PASS` from the env file are reset to empty, so the server runs open and admin login is impossible. This is the step's stated goal ("run with secrets injected from the environment"), and the runbook says it works. CI never sets a password, so it cannot catch this.
- Fix options:
  - With owner approval: remove those three lines from `content/LaunchScript/CTF.cfg`.
  - Inside the Allowed paths: ship `packaging/docker/CTF.cfg` without them and `COPY` it over `/opt/bv2/main/LaunchScript/CTF.cfg`.
  - Either way, have CI set `BV2_SV_PASSWORD` and assert `Password = 1` in the master's `web.db` row (the heartbeat sends `*`, `CMaster.cpp:893`).

**2. HIGH: both healthchecks, and the CI port probe, open real game connections.**
- Where: `Dockerfile:44,56`; `containers.yml:54`.
- Problem: `bash -c '</dev/tcp/127.0.0.1/3333'` makes a full TCP connection. babonet accepts it and promotes it to a client straight away, because UDP is off: `cIncConnection.cpp` State 0 returns 1. `Server::updateNet` (`Server.cpp:366-405`) then calls `createNewPlayerSV`, which takes a player slot and broadcasts `NET_SVCL_NEWPLAYER` to every client, followed by a disconnect broadcast and console lines.
- Effect: every 10 s, live players see a phantom 127.0.0.1 player join and leave. The heartbeat's player count can include it. Connected clients get "A client has connected" / "Player disconnected" traffic. The master's check also logs "new client" every 10 s.
- Fix: test for a listening socket without connecting:
  - server: `grep -q ':0D05 00000000:0000 0A' /proc/net/tcp` (port 3333; babonet is IPv4 only)
  - master: `:27DF` (port 10207)
  - In CI, drop the `/dev/tcp` probe, or accept it since CI has no players.

**3. MEDIUM: neither process handles SIGTERM or SIGINT as PID 1.**
- Where: `Dockerfile:47,60`; `compose.example.yml`; `runbook.md:37,39`.
- Problem: the codebase installs no signal handlers (only `SIGPIPE` in `baboNet.cpp:125`). The kernel ignores those signals for PID 1 when no handler is set. The master entrypoint `exec`s, so `bv2master` is also PID 1.
- Effect: `docker compose stop`, upgrades and restarts wait 10 s, then SIGKILL. The runbook line "Ctrl-C stops the server" is false.
- Fix: add `init: true` to both services and `--init` to any `docker run` in the runbook. Then correct the runbook's stop/Ctrl-C text (the `quit` console command does exit cleanly).

**4. MEDIUM: `docker attach` detach keys need a TTY.**
- Where: `compose.example.yml:22`; `runbook.md:37`.
- Problem: `stdin_open: true` without `tty: true` means Ctrl-P Ctrl-Q (Docker docs: "if the container was run with -i and -t") does not detach, and Ctrl-C does nothing (see 3). An operator who attaches has no documented way out short of killing their terminal.
- Fix: add `tty: true`, or document `docker attach --sig-proxy=false` and closing the terminal.

**5. MEDIUM: the runbook gives the wrong config-file syntax.**
- Where: `runbook.md:26`.
- Problem: it says "`set` lines in `bv2.cfg` … or a file passed with `--config`". Both are read by `CSystemVariable::loadConfig`, which expects `name value` lines (see `config/bv2.example.cfg:46,70`). A `set …` line is silently ignored. Separately, `zsv_adminUser` is overwritten by the launch script anyway (finding 1).
- Fix: show `zsv_adminUser "boss"` with no `set`, and fix finding 1.

**6. LOW/MEDIUM: the acceptance check `docker run --rm <image> id -u` cannot work.**
- Where: step11 doc, Acceptance checks.
- Problem: with an `ENTRYPOINT`, the arguments go to the server (`execute id`, which runs forever) or to `sh -c` (which runs the master forever). CI correctly uses `--entrypoint id`.
- Fix: update the acceptance line to `docker run --rm --entrypoint id <image> -u`.

**7. LOW: `.dockerignore` lets local secrets and untracked files into the build context.**
- Where: `.dockerignore:1-7`.
- Problem: `COPY . .` (`Dockerfile:22`) copies whatever `.gitignore` hides locally into the build stage and the local build cache: `config/local/`, `.env*`, `secrets/`, `*.key`/`*.pem`, `*.db`, `bv2.cfg`, `/original-assets/`, `/content/{maps,…}`, `CMakeUserPresets.json`. They are not in the final image, but they do sit in cached build-stage layers.
- Also: `*.md` matches only root-level files (no implicit `**`), so the "sources only" comment is inaccurate.
- Fix: mirror the `.gitignore` secrets and original-asset blocks. Also exclude `.github/`, `tests/` and `packaging/docker/` so CI-only edits don't invalidate the C++ build layer.

**8. LOW: build layering and caching (a structural simplification).**
- Where: `Dockerfile:22-29`; `containers.yml:24-41`.
- Problem: the vcpkg dependency build (sqlite3, libsodium) runs in the same layer as `COPY . .`. The `--mount=type=cache` is not exported to `type=gha`, so every source change rebuilds the dependencies in CI. `mode=max` also exports the large build-stage layers (`/src/build`, the vcpkg clone) to the 10 GB GHA cache.
- Fix: `COPY vcpkg.json` and run `vcpkg install --x-install-root=/opt/vcpkg_installed` as its own layer, then configure with `-DVCPKG_INSTALLED_DIR=/opt/vcpkg_installed`. The dependency layer then changes only with `vcpkg.json`.
- Optional simplification: build with `docker compose -f … build`, since the compose file already declares both targets. That removes the two duplicated build-push blocks and actually tests the compose `build:` stanzas. The trade-off is less control over the GHA cache.
- Separately: the vcpkg clone fetches the whole history; `git fetch --depth 1 origin <baseline>` is enough.

**9. LOW: CI is not bisectable across the step's commits.**
- Where: commit `d75c364`.
- Problem: its workflow uses `packaging/docker/compose.example.yml`, which only arrives in `79fc8a8`, so the `containers` job fails at `d75c364`.
- Fix: order or squash so 11.3 lands before 11.2, or note it.

**10. LOW: CI keeps no logs when `up --wait` fails.**
- Where: `containers.yml:52`.
- Problem: under `set -e`, a failed or timed-out `up --wait` exits before `$compose logs` runs.
- Fix: add a separate `if: failure()` step that runs `docker compose … logs` and `ps`.

**11. LOW: GHCR push hygiene.**
- Where: `containers.yml:16,66-73`.
- `packages: write` is granted to every push and PR run. Put the push in its own job, `needs: images`, `if: main`, holding the permission.
- The token is templated into shell (`echo "${{ secrets.GITHUB_TOKEN }}"`). Use `docker/login-action` or an `env:` variable instead.
- Add `LABEL org.opencontainers.image.source=https://github.com/Hub-Tansky/<repo>` so the packages link to the repo. They are private until the owner changes their visibility, which the runbook does say.

**12. LOW: base image and patching.**
- `FROM ubuntu:24.04` is unpinned, and there is no Dependabot `docker` ecosystem entry for `/packaging/docker` (`.github/dependabot.yml` is inside the Allowed paths).
- Images are rebuilt only on pushes to `main`, so OS security fixes are picked up only when someone pushes. Consider a weekly `schedule:` trigger.

**13. LOW: the master image carries GL/GLU.**
- Where: `Dockerfile:32-34`.
- Problem: the shared runtime stage installs `libgl1 libglu1-mesa`, but `bv2master` links neither (`setup-dev.sh:8-9`).
- Fix: move that `apt-get` to the `server` stage.

**14. LOW: the master entrypoint is an escaped shell one-liner inside JSON.**
- Where: `Dockerfile:60`.
- Problem: it is hard to read and review.
- Fix: move it to `packaging/docker/master-entrypoint.sh` and `COPY` it, or use a heredoc `RUN` to create it.

**15. LOW: the "at EOF it idles" note is inaccurate.**
- Where: step11 Notes.
- Problem: when stdin is at EOF (plain `docker run` without `-i`), `std::cin.getline` fails at once on every pass. The loop (`main.cpp` CONSOLE `main`) then takes the game mutex about 1000 times a second; it spins with a 1 ms sleep rather than idling.
- Fix: say "use `-i` / compose `stdin_open`" in the runbook, and correct the note.

**16. LOW: docs and inventory gaps.**
- `ARCHITECTURE.md` files `docs/ops/runbook.md` under `### docs`, where the convention is a `### docs/ops` section.
- Task 11.1 differs from the plan without a note: the `linux-x64-release` preset instead of `linux-x64`, no declared `BV2_DATA_DIR` volume, and one Dockerfile where the deliverable says "Two Dockerfiles". Record whether the owner approved this.
- The runbook doesn't say the images are linux/amd64 only (the triplet `x64-linux` is hard-coded). Apple Silicon and ARM hosts run them under emulation or not at all.
- The `AGENTS.md` CI list doesn't mention `containers` or whether it is required. That file is out of path, so record it in `possible-new-scope.md`.

**17. LOW: Docker logs grow without limit.**
- Problem: compose sets no rotation, and the master writes a log line on every connection, including the 10 s healthcheck (finding 2).
- Fix: add `logging: { driver: json-file, options: { max-size: "10m", max-file: "3" } }` to both services.

## Not checked / unsure
- Whether vcpkg resolves the baseline cleanly from a `--filter=blob:none` clone. Lazy blob fetches should work, but this needs the CI run to confirm.
- Whether `docker/setup-buildx-action@f87e599… # v4.4.1` and `docker/build-push-action@c3c9e26… # v7.4.0` exist under those SHAs; not checked online. Their style matches the repo convention (SHA pins for third-party actions, tags for `actions/*`).
- How `docker attach` detach keys behave without a TTY in current Docker versions. The documentation says they need `-t`; not tested.
- Whether `libgl1` with `--no-install-recommends` pulls in the large Mesa DRI/LLVM packages.
- Whether a GHCR package created with `GITHUB_TOKEN` links to the repo automatically, without the OCI source label.
- Whether the launch script's `set zsv_adminPass ""` (a non-`sv_` variable) reaches dksvar through the console `set`. I read the `sv_` branch only; the rest of the `set` handler at `Console.cpp:822ff` was not traced. `sv_password` is confirmed to be overwritten.
- How the client reacts to the phantom `NEWPLAYER`/`PLAYER_DISCONNECT` pair (what players see on screen) was inferred, not traced in `ClientRecv.cpp`.
- I did not run `tools/review.sh`, `check-architecture.sh` or gitleaks. No real hosts, IPs or secrets appear in the diff on reading.
