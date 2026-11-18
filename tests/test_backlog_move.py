#!/usr/bin/env python3
"""tools/backlog-move.py (ROBO-249): a merged PR's item moves to Done or Testing in a throwaway vault; no id, a missing
file and an item already in place change nothing and never fail; Done is final (ADR ROBO-249)."""
import importlib.util
import os
import shutil
import stat
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOL = os.path.join(ROOT, "tools", "backlog-move.py")
spec = importlib.util.spec_from_file_location("backlog_move", TOOL)
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)

ITEM = """---
id: {id}
title: "T"
status: {status}
created: 2026-11-01
updated: 2026-11-01
---
# T

## Acceptance criteria
{criteria}

## Notes
- [ ] (post-deploy) outside the criteria section does not count
"""

failures = 0


def check(name, cond, detail=""):
    global failures
    if not cond:
        print(f"FAIL {name} {detail}")
        failures += 1


def rmtree(path):
    """Windows: git object files are read-only, and shutil.rmtree can't delete them until made writable."""
    def writable(func, p, _):
        os.chmod(p, stat.S_IWRITE)
        func(p)
    shutil.rmtree(path, **({"onexc": writable} if sys.version_info >= (3, 12) else {"onerror": writable}))


def git(vault, *args):
    return subprocess.run(["git", *args], cwd=vault, check=True, capture_output=True, text=True).stdout


def vault_with(items, parent=None):
    """items: {(folder, id): (status, criteria)}"""
    vault = tempfile.mkdtemp(prefix="vault-", dir=parent)
    git(vault, "init", "-q")
    git(vault, "config", "user.name", "t")
    git(vault, "config", "user.email", "t@example.invalid")
    os.makedirs(os.path.join(vault, "40-backlog", "_templates"))
    with open(os.path.join(vault, "40-backlog", "_templates", "ROBO-999 template.md"), "w") as f:
        f.write(ITEM.format(id="ROBO-999", status="Proposed", criteria="- [ ] x"))
    for (folder, rid), (status, crit) in items.items():
        os.makedirs(os.path.join(vault, "40-backlog", folder), exist_ok=True)
        with open(os.path.join(vault, "40-backlog", folder, f"{rid} T.md"), "w") as f:
            f.write(ITEM.format(id=rid, status=status, criteria=crit))
    git(vault, "add", "-A")
    git(vault, "commit", "-qm", "init")
    return vault


def body(item):
    return f"## Summary\nx\n\n| Field | Value |\n| --- | --- |\n| Item | {item} |\n| Spec | y |\n"


def read(vault, folder, rid):
    path = os.path.join(vault, "40-backlog", folder, f"{rid} T.md")
    return open(path).read() if os.path.exists(path) else None


# 1. Plain criteria: Done in 90-done, updated today, staged as a rename. The vault sits under `_temp/` as on a runner.
runner_temp = os.path.join(tempfile.mkdtemp(prefix="runner-"), "_temp")
os.makedirs(runner_temp)
v = vault_with({("30-inprogress", "ROBO-300"): ("In progress", "- [x] a\n- [ ] b")}, runner_temp)
msg, note = tool.move(v, body("ROBO-300 (rv2-docs `40-backlog/30-inprogress/ROBO-300 T.md`)"), "7", "2026-11-18")
check("done message", msg == "backlog: ROBO-300 -> Done (PR #7)", msg)
text = read(v, "90-done", "ROBO-300")
check("done moved", text is not None and read(v, "30-inprogress", "ROBO-300") is None)
check("done status", text and "\nstatus: Done\n" in text and "\nupdated: 2026-11-18\n" in text and "\ncreated: 2026-11-01\n" in text, text)
check("done staged rename", git(v, "status", "--porcelain").startswith("R "), git(v, "status", "--porcelain"))
rmtree(os.path.dirname(runner_temp))

# 2. Unticked (post-deploy): Testing, stays in 30-inprogress.
v = vault_with({("30-inprogress", "ROBO-301"): ("In progress", "- [x] a\n- [ ] (post-deploy) players see it")})
msg, _ = tool.move(v, body("ROBO-301"), "8", "2026-11-18")
check("testing message", msg == "backlog: ROBO-301 -> Testing (PR #8)", msg)
text = read(v, "30-inprogress", "ROBO-301")
check("testing status", text and "\nstatus: Testing\n" in text, text)

# 3. Same item merged again by a second PR: already Testing, nothing to commit.
git(v, "commit", "-qm", "move")
msg, note = tool.move(v, body("ROBO-301"), "9", "2026-11-19")
check("already testing", msg is None and "already" in note, note)
rmtree(v)

# 4. Ticked (post-deploy): Done.
v = vault_with({("10-next", "ROBO-302"): ("Next", "- [x] (post-deploy) seen on prod")})
msg, _ = tool.move(v, body("ROBO-302"), "10", "2026-11-18")
check("ticked post-deploy done", msg == "backlog: ROBO-302 -> Done (PR #10)" and read(v, "90-done", "ROBO-302"), msg)
rmtree(v)

# 5. Already Done: no change.
v = vault_with({("90-done", "ROBO-303"): ("Done", "- [x] a")})
msg, note = tool.move(v, body("ROBO-303"), "11", "2026-11-18")
check("already done", msg is None and "already" in note and git(v, "status", "--porcelain") == "", note)
rmtree(v)

# 5b. Done is final: an item closed by hand with an unticked (post-deploy) box does not go back to Testing.
v = vault_with({("90-done", "ROBO-307"): ("Done", "- [ ] (post-deploy) owner waived it")})
msg, note = tool.move(v, body("ROBO-307"), "15", "2026-11-18")
check("done is final", msg is None and "already Done" in note and git(v, "status", "--porcelain") == "", note)
rmtree(v)

# 5c. No frontmatter: warning, nothing changed.
v = vault_with({})
os.makedirs(os.path.join(v, "40-backlog", "30-inprogress"))
with open(os.path.join(v, "40-backlog", "30-inprogress", "ROBO-308 T.md"), "w") as f:
    f.write("# T\nstatus: In progress\n")
msg, note = tool.move(v, body("ROBO-308"), "16", "2026-11-18")
check("no frontmatter", msg is None and note.startswith("warning") and "frontmatter" in note, note)
rmtree(v)

# 6. No id in the Item row (an id elsewhere in the body does not count), and no Item row at all.
v = vault_with({("30-inprogress", "ROBO-304"): ("In progress", "- [ ] a")})
for case, b in [("no id", body("none (chore)") + "\nSee ROBO-304.\n"), ("no row", "## Summary\nROBO-304\n"), ("empty", "")]:
    msg, note = tool.move(v, b, "12", "2026-11-18")
    check(case, msg is None and not note.startswith("warning") and git(v, "status", "--porcelain") == "", note)

# 7. Missing item file, and a template with the same id: warning, nothing changed.
for case, rid in [("missing", "ROBO-305"), ("template only", "ROBO-999")]:
    msg, note = tool.move(v, body(rid), "13", "2026-11-18")
    check(case, msg is None and note.startswith("warning") and git(v, "status", "--porcelain") == "", note)
rmtree(v)

# 8. CLI: exit 0, ::warning:: line and an empty message= output on a missing item.
v = vault_with({})
out = os.path.join(v, "gh-output")
r = subprocess.run([sys.executable, "-B", TOOL, v, "14", "2026-11-18"], capture_output=True, text=True,
                   env={**os.environ, "PR_BODY": body("ROBO-306"), "GITHUB_OUTPUT": out})
check("cli exit 0", r.returncode == 0, r.stderr)
check("cli warning", r.stdout.startswith("::warning::"), r.stdout)
check("cli output", open(out).read() == "message=\n", open(out).read())
rmtree(v)

print("FAILED" if failures else "OK")
sys.exit(1 if failures else 0)
