"""Prints the Discord webhook payload that build.yml's release job posts to #announcements (ADR 0032).

One embed: the release title and page, the player downloads and the website. With a role id the message pings that
role (release-ping, opt-in), and allowed_mentions lets through that role and nothing else, so text in release notes
can't ping @everyone. Usage: discord_release_post.py <tag> [<role id>]
"""
import json
import sys

REPO = "https://github.com/Hub-Tansky/RoboViolence2"
WEBSITE = "https://roboviolence.com"
# Player downloads in the order the website lists them; names come from build.yml's `gh release create` line
DOWNLOADS = [
    ("Windows installer", "roboviolence2-win-x64-msvc-setup.exe"),
    ("macOS (Apple Silicon)", "roboviolence2-macos-arm64.dmg"),
    ("Linux .tar.gz", "roboviolence2-linux-x64.tar.gz"),
    ("Linux .deb", "roboviolence2-linux-x64.deb"),
]


def payload(tag, role_id=""):
    prerelease = tag.startswith("v0.") or "-" in tag  # same rule as the release job
    links = "\n".join(f"[{label}]({REPO}/releases/download/{tag}/{name})" for label, name in DOWNLOADS)
    message = {
        "username": "Robo Violence 2",
        "embeds": [{
            "title": f"Robo Violence 2 {tag}{' (alpha)' if prerelease else ''} is out",
            "url": f"{REPO}/releases/tag/{tag}",
            "description": f"{links}\n\nInstall guide and all files: {WEBSITE}/#install",
            "color": 0xE5383B,  # website --red
        }],
        "allowed_mentions": {"parse": [], "roles": [role_id] if role_id else []},
    }
    if role_id:
        message["content"] = f"<@&{role_id}>"
    return message


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3):
        sys.exit("usage: discord_release_post.py <tag> [<role id>]")
    print(json.dumps(payload(*sys.argv[1:])))
