#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
PORT="${MAX_E2E_PORT:-18080}"
BIN="./max_e2e_harness"
LOG="./mock_max_server.log"

g++ -std=gnu++98 -Wall -Wextra -Werror \
  ../maxcore.cpp ../maxclient.cpp posix_http_transport.cpp e2e_harness.cpp \
  -o "$BIN"

python3 mock_max_server.py "$PORT" >"$LOG" 2>&1 &
PID=$!
trap 'kill "$PID" 2>/dev/null || true; wait "$PID" 2>/dev/null || true' EXIT

for _ in 1 2 3 4 5 6 7 8 9 10; do
  if python3 - <<PY >/dev/null 2>&1
import socket
s=socket.create_connection(("127.0.0.1",$PORT),0.2); s.close()
PY
  then break; fi
  sleep 0.1
done

"$BIN" "http://127.0.0.1:$PORT"

echo "--- mock server log ---"
cat "$LOG"
