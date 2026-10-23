# 0032: The release job posts each release to Discord through a webhook

- Status: Accepted
- Date: 2026-10-23
- Affects: `.github/workflows/build.yml` (release job), `packaging/discord_release_post.py`, `tests/test_discord_release_post.py`

## Context
- The owner set up the Discord server on 2026-10-23 (plan decided 2026-10-20, map "First public release delivery"): `#announcements` is an announcement channel, and players opt in to the `release-ping` role during onboarding.
- The release is created with `GITHUB_TOKEN`, and events from that token start no other workflow, so an `on: release` workflow would never run.

## Decision
- A last step of the release job runs `packaging/discord_release_post.py <tag> [--prerelease] --role <id>` and posts its JSON once (no retry: a webhook post isn't idempotent) to the webhook in the `DISCORD_RELEASE_WEBHOOK` repository secret.
- The post is one embed: the release title (with "(alpha)" when the job's own pre-release rule passed `--prerelease`, so the rule lives only in the job), the release page, direct links to the four player downloads and the website's install section.
- The repository variable `DISCORD_RELEASE_ROLE_ID` (not secret) adds a ping of that role. `allowed_mentions` allows that role and nothing else, so only that role can be pinged even if free text such as release notes is added to the post later.
- Without the secret the step prints a line and succeeds (forks, and before setup). A failed post fails the job after the release exists; re-running the job would fail on the existing release, so the owner posts by hand.

## Consequences
- Webhooks can't publish (crosspost) in an announcement channel: the owner clicks Publish so following servers get the post.
- The webhook URL is a credential: anyone with it can post as the server. It lives only in the secret; a leak means deleting the webhook in Discord and saving a new one.
- Rejected: the repo webhook's fixed `/github` format (no download links, no role ping); a third-party release-to-Discord action (supply-chain risk for ten lines of code); a bot (hosting, and only needed for auto-publish or slash commands).
