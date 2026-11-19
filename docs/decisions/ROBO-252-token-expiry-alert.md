# ROBO-252: A weekly check pings the owner 14 days before SYMBOLS_TOKEN expires

- Status: Accepted
- Date: 2026-11-19
- Affects: `.github/workflows/token-expiry.yml`, `tools/token-expiry.py`, `docs/build/README.md`; ROBO-252 (workflow audit WP-15 part 3, approved 2026-11-11)

## Context
- `SYMBOLS_TOKEN` is a fine-grained token (Contents write on rv2-docs only) with an expiry date. Release symbols (`build.yml`) and backlog moves after each merge (`backlog-move.yml`, ADR ROBO-249) need it; on expiry the symbols step fails on the next tag and every backlog-move run fails at its clone.
- GitHub answers an API call made with an expiring token with the header `github-authentication-token-expiration`; a token without expiry gets no such header.

## Decision
Every Monday (and by hand) `token-expiry.yml` calls `GET /repos/Hub-Tansky/rv2-docs` with the token and passes the status and headers to `tools/token-expiry.py`. 14 days or fewer left, a status other than 200 or a missing secret send one ntfy message to `NTFY_TOPIC` and fail the run; otherwise the run is green and its summary shows the date. Token and topic reach curl as config on stdin.

## Consequences
- The owner hears of an expiry two weeks ahead, twice at most before the date, and a red weekly run stays visible until the secret is renewed.
- A token without expiry passes without an alert.
- Other tokens are not checked; a second token gets a second job calling the same tool.
- Rejected: checking in every build (noise on each PR); a calendar reminder (lost when the token is renewed with a new date).
