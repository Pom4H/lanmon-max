#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
PORT="${MAX_E2E_PORT:-18081}"
BASE="http://127.0.0.1:${PORT}"
LOG="mock_lanmon_max_server.log"
CXX="${CXX:-g++}"

$CXX -std=gnu++98 -Wall -Wextra -Werror \
  ../maxcore.cpp ../maxclient.cpp ../maxusers.cpp ../maxsettings.cpp \
  ../lanmon_commands.cpp ../lanmon_bot.cpp \
  ../e2e/posix_http_transport.cpp lanmon_e2e_harness.cpp \
  -o lanmon_e2e_harness

python3 mock_lanmon_max_server.py "$PORT" >"$LOG" 2>&1 &
PID=$!
trap 'kill "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true' EXIT
for i in $(seq 1 50); do
  if python3 - <<PY >/dev/null 2>&1
import urllib.request
urllib.request.urlopen("$BASE/_state", timeout=.2).read()
PY
  then break; fi
  sleep .05
done

./lanmon_e2e_harness "$BASE"
printf '\n--- mock HTTP log ---\n'
cat "$LOG"
