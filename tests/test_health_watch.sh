#!/bin/bash
# Test for tools/staging/health-watch.sh: which containers count as a problem, when a state notifies, and a whole run against a stub docker.
# Usage: test_health_watch.sh <scratch dir>
set -u
watcher=$(cd "$(dirname "$0")/../tools/staging" && pwd)/health-watch.sh
scratch=${1:?scratch dir}
rm -rf "$scratch" && mkdir -p "$scratch"
unset RV2_DEPLOY_LABEL
export RV2_HEALTH_SOURCE_ONLY=1 RV2_STAGING_ROOT="$scratch/root" RV2_STAGING_ENV="$scratch/env"
# shellcheck source=../tools/staging/health-watch.sh
. "$watcher"

failures=0
expect() { # expect <name> <expected> <actual>
	if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FAIL $1: expected '$2', got '$3'"; failures=$((failures + 1)); fi
}

ps_lines() { printf '%s\t%s\t%s\n' "$@"; } # ps_lines <service> <state> <status> ...: `docker ps -a` lines

expect "the stack's services come from GAME_SERVERS" "master admin-proxy server tdm" "$(services | tr '\n' ' ' | sed 's/ $//')"
expect "healthy, starting and healthcheck-less running containers are fine" "" "$(ps_lines \
	master running 'Up 2 hours (healthy)' server running 'Up 5 seconds (health: starting)' admin-proxy running 'Up 2 hours' |
	problems "master server admin-proxy")"
expect "unhealthy, exited and restarting containers are problems, sorted by service" \
	"admin-proxy restarting; server unhealthy; tdm exited" "$(ps_lines \
	tdm exited 'Exited (1) 3 minutes ago' master running 'Up 2 hours (healthy)' server running 'Up 2 hours (unhealthy)' \
	admin-proxy restarting 'Restarting (1) 4 seconds ago' | problems "master server tdm admin-proxy")"
expect "an expected service without a container is missing" "tdm missing" "$(ps_lines master running 'Up 2 hours (healthy)' | problems "master tdm")"
expect "an empty line is no container" "master missing" "$(printf '\n' | problems master)"

expect "a problem seen once doesn't notify" "" "$(health_step "tdm exited" "" "")"
expect "a problem seen twice notifies" alert "$(health_step "tdm exited" "tdm exited" "")"
expect "and only once" "" "$(health_step "tdm exited" "tdm exited" "tdm exited")"
expect "a changed problem notifies again once it holds" alert "$(health_step "server unhealthy" "server unhealthy" "tdm exited")"
expect "one healthy run after an alert doesn't recover" "" "$(health_step "" "tdm exited" "tdm exited")"
expect "two healthy runs after an alert recover" recover "$(health_step "" "" "tdm exited")"
expect "healthy without an alert stays quiet" "" "$(health_step "" "" "")"

# The whole run, against a stub docker: notifications appear in the log (no NTFY_TOPIC, nothing is sent)
mkdir -p "$scratch/bin"
printf '#!/bin/bash\n[ -f "%s" ] || exit 1\ncat "%s"\n' "$scratch/docker-up" "$scratch/ps" >"$scratch/bin/docker" && chmod +x "$scratch/bin/docker"
touch "$scratch/docker-up"
all_up() { ps_lines master running 'Up 1 minute (healthy)' server running 'Up 1 minute (healthy)' \
	tdm running 'Up 1 minute (healthy)' admin-proxy running 'Up 1 minute'; }
printf 'BV2_ADMIN_PASS=test-only-value\n' >"$RV2_STAGING_ENV" && chmod 600 "$RV2_STAGING_ENV"
watch() { (unset RV2_HEALTH_SOURCE_ONLY; PATH="$scratch/bin:$PATH" bash "$watcher") | sed -n 's/^[^ ]* notify: //p'; }
{ all_up | grep -v '^tdm'; ps_lines tdm exited 'Exited (1) 1 minute ago'; } >"$scratch/ps"
expect "first run with a problem is quiet (and makes the state folder)" "" "$(watch)"
expect "second run notifies with the label" "RV2 staging unhealthy - tdm exited" "$(watch)"
expect "third run stays quiet" "" "$(watch)"
: >"$scratch/ps"
watch >/dev/null
expect "no containers: every service is missing" \
	"RV2 staging unhealthy - admin-proxy missing; master missing; server missing; tdm missing" "$(watch)"
rm "$scratch/docker-up"
watch >/dev/null
expect "docker not answering is a problem" "RV2 staging unhealthy - docker does not answer" "$(watch)"
touch "$scratch/docker-up"
all_up >"$scratch/ps"
watch >/dev/null
expect "recovery notifies once it holds" "RV2 staging healthy again - Was: docker does not answer" "$(watch)"

# A deploy holds the lock: not checked. A lock naming any other process is stale: checked.
ps_lines tdm exited 'Exited (1) 1 minute ago' >"$scratch/ps"
printf 'sleep 30\n' >"$scratch/deploy.sh"
bash "$scratch/deploy.sh" >/dev/null 2>&1 & echo $! >"$ROOT/lock" # detached: its sleep would hold ctest's pipe
watch >/dev/null
expect "nothing is checked while a deploy runs" "" "$(watch)"
pkill -P $! 2>/dev/null; kill $!
sleep 30 >/dev/null 2>&1 & echo $! >"$ROOT/lock"
watch >/dev/null
expect "a lock left by another process doesn't silence the watcher" "RV2 staging unhealthy - admin-proxy missing; master missing; server missing; tdm exited" "$(watch)"
pkill -P $! 2>/dev/null; kill $!

[ $failures -eq 0 ] && echo "all passed"
exit $failures
