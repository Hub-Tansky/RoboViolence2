# 0017: Test main builds on the owner's Mac with a pull agent

- Status: Accepted
- Date: 2026-10-14
- Affects: [phase T step 1](https://github.com/Hub-Tansky/RoboViolence2/blob/58c91b2a660db2b290b0b5f504381980a6aedba2/docs/roadmap/phase-t-testing-environment/step1-mac-pull-agent.md)

## Context
- The repo is public. A self-hosted GitHub runner on the owner's Mac would let a fork pull request run code there.
- CI uploads a macOS package per build with a SHA-256 digest in the Actions API (`.github/workflows/build.yml`, `archive: false`); it holds client, server, master and databases.
- The admin API is meant for the compose network behind Caddy TLS ([ADR 0015](0015-admin-http-api-behind-tls-proxy.md)); the binary has no TLS.

## Decision
A `launchd` agent (`tools/playtest/agent.sh`) pulls only successful `push` builds of `main` from this repo, checks the artifact's SHA-256 digest (transfer integrity; the trust anchor is the main-only run filter), and tests the packaged client against a disposable local master and server from the same package. Staging is a separate, on-demand target that the agent only reads (`GET /status`).

## Consequences
- No inbound access to the Mac and no PR code on it. A PR build is never tested there; that would need its own isolated, approved flow.
- The local admin API runs in plain HTTP on 127.0.0.1 during a test: no network exposure, so no TLS proxy. The packaged master and server still listen on all interfaces while a test runs.
- Fixed ports: a manual `--play` session defers scheduled runs.
- Rejected: self-hosted runner (fork PR code on the Mac); Compose with GHCR images per build (separate workflow, image may lag the package, needs a checkout); `gh run download` (extracts the zip, so the digest can't be checked).
