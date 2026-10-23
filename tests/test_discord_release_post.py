"""packaging/discord_release_post.py builds the #announcements post of the release job (ADR 0032): links to the files
the job uploads, the opt-in role ping, and no other mention."""
import importlib.util
import json
import pathlib
import re
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parent.parent
script = root / "packaging" / "discord_release_post.py"
spec = importlib.util.spec_from_file_location("discord_release_post", script)
post = importlib.util.module_from_spec(spec)
spec.loader.exec_module(post)
failures = []


def check(ok, message):
    if not ok:
        failures.append(message)


# Every linked file is one the release job uploads, so no link 404s
uploaded = set(re.findall(r"dist/(roboviolence2-[\w.-]+)", (root / ".github/workflows/build.yml").read_text()))
for _, name in post.DOWNLOADS:
    check(name in uploaded, f"{name} is not uploaded by build.yml's release job ({sorted(uploaded)})")

# With a role id: the content pings that role only, the embed names the tag and links the release page and the files
with_role = post.payload("v0.0.2", prerelease=True, role_id="123456789012345678")
check(with_role["content"] == "<@&123456789012345678>", f"role ping: {with_role.get('content')!r}")
check(with_role["allowed_mentions"] == {"parse": [], "roles": ["123456789012345678"]},
      f"allowed_mentions: {with_role['allowed_mentions']}")
embed = with_role["embeds"][0]
check(embed["title"] == "Robo Violence 2 v0.0.2 (alpha) is out", f"pre-release title: {embed['title']!r}")
check(embed["url"] == f"{post.REPO}/releases/tag/v0.0.2", f"release page: {embed['url']}")
for _, name in post.DOWNLOADS:
    check(f"{post.REPO}/releases/download/v0.0.2/{name})" in embed["description"], f"no link to {name}")
check(len(embed["description"]) <= 4096, "embed description over Discord's 4096 characters")

# Without a role id: no ping at all, and no other mention type is allowed
no_role = post.payload("v1.0.0")
check("content" not in no_role, f"content without a role: {no_role.get('content')!r}")
check(no_role["allowed_mentions"] == {"parse": [], "roles": []}, f"allowed_mentions: {no_role['allowed_mentions']}")
check(no_role["embeds"][0]["title"] == "Robo Violence 2 v1.0.0 is out", f"release title: {no_role['embeds'][0]['title']!r}")

# The command lines the workflow runs print the same JSON; an empty --role means no role
def cli(*args):
    run = subprocess.run([sys.executable, "-I", str(script), *args], capture_output=True, text=True, check=True)
    return json.loads(run.stdout)


check(cli("v1.0.0", "--role", "") == post.payload("v1.0.0"), "CLI without pre-release or role differs from payload()")
check(cli("v0.0.2", "--prerelease", "--role", "123456789012345678") == with_role, "CLI with both differs from payload()")

if failures:
    sys.exit("FAIL " + "\nFAIL ".join(failures))
print("ok: discord release post")
