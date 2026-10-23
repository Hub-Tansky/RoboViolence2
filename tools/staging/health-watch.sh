#!/bin/bash
# Container health watcher: ntfy when a container of the compose stack is unhealthy, stopped, restarting or missing, and again when all are well.
# Runs every minute from cron beside deploy.sh, with the same RV2_DEPLOY_LABEL (production: docs/ops/runbook.md#production).
# Usage: health-watch.sh   no arguments; logs only changes
# A state must be seen on two runs in a row before it notifies, so a container recreated by a deploy or a slow start doesn't alert.
# Nothing is checked while deploy.sh runs: it notifies its own failures.
set -u

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# LABEL, ROOT, ENV_FILE, PROJECT, GAME_SERVERS, load_env, deploy_running, say, notify
# shellcheck source=deploy.sh
RV2_DEPLOY_SOURCE_ONLY=1 . "$here/deploy.sh"

# --- Helpers (tests/test_health_watch.sh) ---

services() { # the compose services of the stack: master, the proxy and every game server deploy.sh knows
	local s
	echo master admin-proxy
	for s in $GAME_SERVERS; do echo "${s%%:*}"; done
}

problems() { # problems <service...>: `docker ps -a` lines "<service>\t<state>\t<status>" on stdin -> "<service> <problem>" per bad or missing one, joined by "; "
	awk -F'\t' -v want="$*" 'NF { seen[$1] = 1; if ($2 != "running") print $1, $2; else if ($3 ~ /\(unhealthy\)/) print $1, "unhealthy" }
		END { n = split(want, w, " "); for (i = 1; i <= n; i++) if (!(w[i] in seen)) print w[i], "missing" }' |
		sort | paste -sd ';' - | sed 's/;/; /g'
}

health_step() { # health_step <now> <previous run> <last notified>: alert, recover or nothing, once the state has held for two runs
	[ "$1" = "$2" ] && [ "$1" != "$3" ] || return 0
	if [ -n "$1" ]; then echo alert; else echo recover; fi
}

[ -n "${RV2_HEALTH_SOURCE_ONLY:-}" ] && return 0

load_env || exit 1 # the reason goes to the log; without the env file there is no ntfy topic
mkdir -p -m 700 "$ROOT" || exit 1 # the state folder; deploy.sh makes it too, but cron may start this first
deploy_running && exit 0

if lines=$(docker ps -a -f "label=com.docker.compose.project=$PROJECT" \
	--format '{{.Label "com.docker.compose.service"}}{{"\t"}}{{.State}}{{"\t"}}{{.Status}}'); then
	now=$(printf '%s\n' "$lines" | problems "$(services)")
else
	now="docker does not answer"
fi

prev="" told=""
read -r prev 2>/dev/null <"$ROOT/health-seen"
read -r told 2>/dev/null <"$ROOT/health-notified"
echo "$now" >"$ROOT/health-seen"
case $(health_step "$now" "$prev" "$told") in
	alert) notify "RV2 $LABEL unhealthy" "$now"; echo "$now" >"$ROOT/health-notified" ;;
	recover) notify "RV2 $LABEL healthy again" "Was: $told"; : >"$ROOT/health-notified" ;;
esac
