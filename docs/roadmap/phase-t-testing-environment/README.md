# Phase T: testing environment

**Status:** IN PROGRESS

**Depends on:** Phase B packages and CI. Runs alongside Phase C, which owns game, packaging and release work. **Plan:** [20261014_testing_environment_plan.md](../20261014_testing_environment_plan.md). **Rules:** [roadmap README](../README.md).

## Goal

Every successful `main` build is installed and tested on the owner's Mac without manual work, with a pass or failure report per build. Later steps add a staging server and automated feature tests.

## Steps

| # | Step | Plan item | Status |
|---|---|---|---|
| 1 | [Mac pull agent](step1-mac-pull-agent.md) | T1 | IN PROGRESS |
| 2 | [Staging host and deploy agent](step2-staging-deploy.md) | T8 | IN PROGRESS |
| – | CI client smoke, autotest mode, E2E bots, Windows VM, workstation, screenshots | T2–T7 | candidate |

Candidates get a scope file only after step 1 is proven.

## Interaction with Phase C

| Phase C step | Effect here |
|---|---|
| 1: player data outside the package | A first-run folder picker would block the unattended client; the agent then sets `BV2_DATA_DIR` |
| 3: installers (`.dmg`) | If the CI artifact name or layout changes, the agent's `ARTIFACT` and install step follow |

## Private facts

Host, Mac and network details: `docs/prv_wiki/docs_restricted/testing-environment/` (git-ignored). Secrets: env files only.
