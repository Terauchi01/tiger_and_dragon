#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${PORT:-9002}"

echo "Cleaning up match processes..."

if command -v lsof >/dev/null 2>&1; then
  PIDS="$(lsof -ti tcp:"$PORT" || true)"
  if [ -n "$PIDS" ]; then
    echo "Stopping processes on port $PORT: $PIDS"
    kill $PIDS || true
  fi
fi

PATTERNS=(
  "$ROOT/ws_server"
  "$ROOT/random_player_cpp"
  "$ROOT/clients/python/random_player.py"
  "clients/java RandomPlayer"
)

for pattern in "${PATTERNS[@]}"; do
  PIDS="$(pgrep -f "$pattern" || true)"
  if [ -n "$PIDS" ]; then
    echo "Stopping: $pattern ($PIDS)"
    kill $PIDS || true
  fi
done

sleep 1

echo "Removing logs..."
rm -f "$ROOT/.ws_server.log" "$ROOT/.p"*.log

echo "Done."
