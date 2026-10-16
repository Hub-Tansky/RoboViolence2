SKILL: Launching skill: thermo-nuclear-code-quality-review

Scope: `git diff f70b068..HEAD`, which covers 14 files and +220/-18 lines. I read all of it: `packaging/make-package.py`, `packaging/windows/roboviolence2.iss`, `.github/workflows/build.yml`, `triplets/arm64-osx.cmake`, `CMakePresets.json`, ADR 0018 and the docs. I modified nothing.

The GitHub Actions setup is correct:
- **Tags:** `on: push:` has no filter, so tags trigger the workflow.
- **Release job:** `release` is gated on `ref_type == 'tag' && startsWith(ref_name,'v')`. Its job-level `permissions: contents: write` overrides the workflow-level `contents: read`.
- **Uploads:** each `upload-artifact@v7` with `archive:false` uploads exactly one file, so the artifact names are unique.
- **Download:** `download-artifact@v8` with `pattern` + `merge-multiple` puts the bare files in `dist/`, which matches the `gh release create` paths.
- **Shell variables:** `$GITHUB_REF_NAME` is quoted in the release step.
- **Inno Setup:** `AppId` has the escaped `{{` it needs. `{autopf}` + `PrivilegesRequired=lowest` installs to `%LOCALAPPDATA%\Programs`. `MinVersion=10.0.19045` is Windows 10 22H2, which matches ADR 0012.
- **Triplet:** the overlay triplet matches vcpkg's built-in arm64-osx plus the deployment target.
- **Files:** no file nears 1k lines.

## Findings

1. **Medium (structure)** `packaging/make-package.py:38-85`: `main()` now has five scattered `if macos` / `if windows` / `else` branches: bundle, archive, ISCC, dmg, and the rmtree moved to the end because the dmg needs the stage. The OS-specific work is interleaved with the common staging flow.
   - Fix: keep `main()` as stage → verify → archive → `extra()`, and move the per-OS steps into `package_macos(stage, out, name)` / `package_windows(stage, out, name, version)` (or a `{platform: fn}` table). Each OS's archive and installer then sit together, and the common path has no branches.

2. **Medium (REVIEW §2, "new behaviour has a test")**: nothing exercises the installer and nothing checks `licenses/` outside `make-package.py`'s own `required` list. The author's evidence covers macOS only; the Windows installer has not run.
   - Fix: add one CI step on Windows after Package: run `setup.exe /VERYSILENT /CURRENTUSER /DIR=<tmp>`, check that `bv2.exe` and `licenses\` exist, then run `unins000.exe /VERYSILENT`.
   - Optionally, have `package-check` assert that `LICENSE.txt` and `licenses/` exist in the Linux tarball.

3. **Medium (data loss on upgrade)** `packaging/windows/roboviolence2.iss:31`: `Source: "{#SourceDir}\*" ... Flags: recursesubdirs ignoreversion` overwrites `master.db` and `web.db` on every upgrade. Anyone running `run-master.cmd` from the install folder loses the master database when they install a new version.
   - Fix: give the two databases their own `[Files]` line with `onlyifdoesntexist uninsneveruninstall`, or document that the installed servers are for testing only.

4. **Low-Medium (Inno)** `roboviolence2.iss:14`: `PrivilegesRequiredOverridesAllowed=dialog` offers an all-users install into Program Files. There `bv2master` cannot write `master.db`, because the folder is read-only for users.
   - Fix: delete the line. The ADR and docs say "per-user, no admin", and the extra mode is unneeded complexity.

5. **Low (Actions script injection)** `.github/workflows/build.yml:46`: `${{ github.ref_name }}` is expanded straight into a `run:` line. Git tag names may contain `;`, `$`, `` ` `` and `|`. Only writers can push tags, but this is the pattern CodeQL `actions` flags.
   - Fix: pass `VERSION: ${{ github.ref_type == 'tag' && github.ref_name || '0.0.0-dev' }}` via `env:` and use `"$VERSION"`, as the release step already does.

6. **Low (duplication)** `build.yml:46` and `make-package.py:28` both hard-code `0.0.0-dev`. Fix: pick one owner, either by making the argument required or by not passing it on non-tag runs.

7. **Low (cosmetic)** A tag `v1.2.0` becomes `AppVersion=v1.2.0`, and Inno's default `AppVerName` then reads "Robo Violence 2 version v1.2.0". Fix: strip the leading `v` in `make-package.py` before calling ISCC.

8. **Low (licence UX)** `roboviolence2.iss:19`: `LicenseFile=` turns GPLv3 into an "I accept the agreement" gate. GPLv3 §9 says acceptance is not required to run the program. Fix: use `InfoBeforeFile=` (shown without accept) or drop the page. `LICENSE.txt` is installed either way.

9. **Low (licence completeness)** `make-package.py:56-59` copies only the vcpkg `copyright` files. The vendored glad loader (`engine/zeven/third_party/glad/`, including Khronos `khrplatform.h`) is compiled into the client and gets no notice in `licenses/`.
   - Fix: also copy glad's licence text, e.g. a `licenses/glad.txt` source next to `third_party/glad/README.md`.

10. **Low (CI cache, out of Allowed paths)** `.github/actions/setup-vcpkg/action.yml` keys `actions/cache` on `hashFiles('vcpkg.json')` only. The new overlay triplet changes every macOS port ABI, so CI restores the old cache under an unchanged key and rebuilds all ports. The rebuilt binaries are never saved, because an exact key hit skips the save. macOS CI will rebuild the ports on every run until `vcpkg.json` changes.
    - Fix: add `triplets/**` to `hashFiles`. That file is outside step 3's Allowed paths, so record it as a PNS entry or widen the scope.

11. **Low (docs)** `docs/build/macos.md` (new section, step 3) says the servers are "in the disk image beside `bv2.app`". The mounted `.dmg` is read-only, so `bv2master` started there cannot write `master.db`. Fix: say to copy the folder out of the image before running the servers.

12. **Low (residual risk, unverified)** `make-package.py:23` hard-codes `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`. Whether the `windows-2025` image ships Inno Setup 6 has not been verified (`docs/roadmap/phase-c-installers-and-fixes/step3-unsigned-installers.md` asserts it under "Context"). The first CI run will settle it; if it's missing, add `choco install innosetup` to the workflow.

13. **Info** `make-package.py:62` uses `licenses/sqlite3.txt` as a sentinel that the licence glob ran. That is fine, but a comment saying why sqlite3 (always a dependency) would help readers.

14. **Info (process)** Commit a82dbe4 bundles tasks 3.1–3.3 into one commit, while AGENTS.md asks for one commit per task item. There is no mechanical-rewrite mixing, so this is not a REVIEW §2 violation. Note it in the record, as steps 1 and 4 did.

## Other REVIEW §2 checks
- The ground rules hold: no assets added, no **[GUI]** change, no rename.
- `ARCHITECTURE.md`, `AGENTS.md`, ADR 0018, the ADR index and the build docs are updated.
- The step **Status** is still IN PROGRESS, as expected before the review record exists.

## Verdict
Not yet approvable. There are no blockers in workflow semantics, and the diff is small and well documented.
- **Act on before closing:** #1 (decompose `main()` per OS), #2 (no executed test of the Windows installer, which has never run), #3 (upgrade overwrites `master.db`).
- **Cheap fixes:** #4 and #5.
- **Fix or defer as PNS entries:** #6 to #12, with #10 being outside the step's Allowed paths.

## Round 2

SKILL: Launching skill: thermo-nuclear-code-quality-review

Scope: `git diff c308425..HEAD` (round-1 fixes in 5 files, +100/-45). I read the whole of `packaging/make-package.py`, `packaging/windows/roboviolence2.iss` and `.github/workflows/build.yml`, plus ADR 0018 and `docs/build/windows.md`. I modified nothing.

### Round-1 findings

**Resolved:**
- #1: `main()` now has no OS branches; it uses a `PLATFORMS` table with `client_*` and `ship_*` functions.
- #2: CI now installs and uninstalls the installer, and `package-check` checks the licences. Both checks are weaker than intended, see new finding 1.
- #3: the databases have their own `[Files]` lines.
- #4: the override line is gone.
- #5: the version comes in through `env`.
- #6: the version argument is required.
- #7: `removeprefix('v')`.
- #8: `InfoBeforeFile`.
- #9: `licenses/glad.txt` is written.
- #10: recorded as PNS-57.
- #11: the macOS doc is fixed.
- #12: accepted on the author's evidence.
- #13: comment added at `make-package.py:106`.
- #14: will be noted in the record.

**Semantics I checked:**
- **Inno Setup:**
  - `Excludes: "master.db,web.db"`: a pattern without a backslash matches that file name in any directory. Only the root has these files, so it is correct.
  - `onlyifdoesntexist` + `uninsneveruninstall` together give "keep on upgrade, keep on uninstall". Correct.
  - `InfoBeforeFile` shows the text without an accept gate and is skipped under `/VERYSILENT`.
  - `/DIR` works with `PrivilegesRequired=lowest`. `/CURRENTUSER` is no longer needed now that the override is removed.
- **Git Bash (MSYS2) on Windows:**
  - An argument starting with `//` reaches the program with the leading `/` removed and every `\` turned into `/`. So `"//DIR=$(cygpath -w …)"` arrives as `/DIR=D:/a/_temp/rv2-install`. Windows and Inno accept forward slashes, so this works.
  - `cygpath -u "$RUNNER_TEMP"` is correct.
  - bash waits for `setup.exe` (the setup stub waits for `setup.tmp`). `unins000.exe` re-launches itself from temp and returns at once, so the polling loop is needed and is correct.

### Findings

1. **Medium (the tests are weaker than they look)**: `.github/workflows/build.yml:54` and `:115`.
   - **Problem:** `shell: bash` runs with `-e`, and `-e` ignores a failing command anywhere in an `&&` list except the last one.
     - In `test -f "$dir/bv2.exe" && test -f "$dir/licenses/sqlite3.txt" && test -f "$dir/master.db"`, only `master.db` is enforced. A missing `bv2.exe` or `licenses/` does not fail the step.
     - The later `test ! -f "$dir/bv2.exe"` would even pass if `bv2.exe` was never installed.
     - Likewise, `package-check` enforces only `licenses/glad.txt`, not `LICENSE.txt` or `sqlite3.txt`.
   - **Fix:** put one `test -f` per line, or use a single `ls file1 file2 file3 > /dev/null`, which fails if any file is missing.

2. **Low-Medium (new behaviour untested)**: `roboviolence2.iss:31-34` and `build.yml:55-57`.
   - **Problem:** the new database rules are not tested. Nothing checks that `master.db` survives uninstall (`uninsneveruninstall`) or an upgrade (`onlyifdoesntexist`), which was the point of round-1 #3.
   - **Fix:** after the uninstall loop, add `test -f "$dir/master.db"`. For the upgrade, write a marker into `master.db` (or record its hash), run setup a second time before uninstalling, and check the database is unchanged.

3. **Low-Medium (ADR and docs missing the new decisions)**: `docs/decisions/0018-package-and-release-format.md:12-20` and `docs/build/windows.md:29`.
   - **Problem:** neither records the decisions this round made:
     - The master databases are kept on upgrade and left behind on uninstall. As a consequence, a later release that changes the schema in `content-seed/*.sql` never reaches an existing install.
     - The GPL is shown but not accepted.
     - `licenses/` also holds `glad.txt`; ADR line 12 says only `<port>.txt`.
   - **Fix:** add these to the ADR's decision and consequences. Add one line to the Windows guide saying the uninstaller leaves `master.db` and `web.db` in the install folder.

4. **Low (bespoke, drift-prone licence generation)**: `packaging/make-package.py:79-81`.
   - **Problem:** `glad.txt` is built from Python with a hard-coded string `"glad 2.0.8 …: WTFPL OR CC0-1.0"`, which repeats `engine/zeven/third_party/glad/README.md:3`. It also slices `khrplatform.h` between the first `/*` and `*/`. This is "magic": regenerating glad, or a change to the header's comment layout, silently drifts or corrupts the notice.
   - **Fix (code-judo):** commit `engine/zeven/third_party/glad/LICENSE.txt` with the glad SPDX line and the Khronos notice, and copy it with one `shutil.copy2`. This removes the `GLAD` path math, the slicing and the duplicated version string. That path may be outside the Allowed paths; if so, record it as a PNS entry.

5. **Low (unnecessary indirection)**: `make-package.py:39-43, 64-68`.
   - **Problem:** `client_binary(exe)` is a closure factory used only so that each table slot holds a function. It also repeats the exe suffix that the same tuple already carries.
   - **Fix:** make `client_binary(runtime, stage, exe)` a plain function and call it as `stage_client(runtime, stage, exe)`. `client_macos` would simply ignore `exe`. Alternatively, let the table hold `None` for "copy `bv2`+exe". Either way one nesting level goes away.
   - **Related:** the 4-tuple with a positional comment (`# sys.platform: (…)`) would read better as a `NamedTuple`. This one is minor.

6. **Low (output may list stale files)**: `make-package.py:115-116`.
   - **Problem:** `out.glob(name + "*")` prints whatever matches in `out/`, including files left over from an earlier run (for example an old `-setup.exe` or `.dmg` in a local `out/`). The printed list no longer proves what this run wrote.
   - **Fix:** have each `ship_*` return the paths it wrote, and print those.

7. **Low (unused parameter)**: `ship_macos` and `ship_linux` take `version` and never use it. This is acceptable for a uniform table signature. Note it only if you take finding 5's `NamedTuple` route; then pass `version` only to Windows (e.g. through a `functools.partial`, or let `ship_windows` read it from the arguments).

8. **Info (`//` argument conversion)**: `build.yml:53`. The `//DIR=` form depends on MSYS rewriting `\` to `/`. It works, but the step is easier to read and less fragile if it uses `cygpath -m` explicitly with a single-`/` switch, or uses `shell: pwsh` for the two installer calls. Optional.

9. **Info (the install test never ran)**: by the author's own evidence, the Windows install and uninstall step has not executed yet. The step cannot close until a CI run shows it passing, together with the fix for finding 1.

### Other REVIEW §2 checks

- **Ground rules:** they hold. No assets, no **[GUI]** change, no rename.
- **Mechanical rewrites:** none mixed with functional changes.
- **ARCHITECTURE.md:** the rows for `make-package.py` and the `.iss` are still accurate.
- **Size:** no file nears 1k lines.
- **Structure:** better than round 1. `main()` is now one straight flow.

### Verdict

Not yet approvable.
- **Blocks closing:** #1. As written, the new CI checks silently enforce only their last file.
- **Before closing:** #2, so the database-preservation fix has a test, and #3, the ADR and doc gaps.
- **Cheap, fix or defer as PNS entries:** #4 to #7.
- **Close only after:** the Windows CI step has actually passed (#9).
