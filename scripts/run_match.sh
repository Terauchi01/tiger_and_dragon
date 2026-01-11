#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PLAYERS="${PLAYERS:-4}"
SEED="${SEED:-42}"
PORT="${PORT:-9002}"
ROOM="${ROOM:-room1}"
SERVER_WAIT="${SERVER_WAIT:-1.5}"
START_DELAY="${START_DELAY:-0.5}"
MATCHES="${MATCHES:-1}"
MATCH_PAUSE="${MATCH_PAUSE:-0.2}"
PORT_WAIT_RETRIES="${PORT_WAIT_RETRIES:-30}"

CXX="${CXX:-g++}"
WS_INCLUDE="${WS_INCLUDE:-/opt/homebrew/include}"
BOOST_PREFIX="${BOOST_PREFIX:-/opt/homebrew/opt/boost@1.85}"
BOOST_ASIO_OLD="${BOOST_ASIO_OLD:-1}"

SERVER_BIN="$ROOT/ws_server"
CPP_CLIENT_BIN="$ROOT/random_player_cpp"

if [ ! -d "$BOOST_PREFIX/include" ]; then
  echo "Boost include not found at $BOOST_PREFIX/include"
  echo "Set BOOST_PREFIX to your boost@1.85 path."
  exit 1
fi

EXTRA_DEFS=()
if [ "$BOOST_ASIO_OLD" = "1" ]; then
  EXTRA_DEFS+=("-DBOOST_ASIO_ENABLE_OLD_SERVICES")
fi

echo "Building server and C++ client..."
"$CXX" -std=c++17 -O2 "${EXTRA_DEFS[@]}" -I"$ROOT/src" -I"$BOOST_PREFIX/include" -I"$WS_INCLUDE" \
  "$ROOT/src/engine.cpp" "$ROOT/server/ws_server.cpp" -o "$SERVER_BIN" \
  -L"$BOOST_PREFIX/lib" -lboost_system -pthread

"$CXX" -std=c++17 -O2 "${EXTRA_DEFS[@]}" -I"$BOOST_PREFIX/include" -I"$WS_INCLUDE" \
  "$ROOT/clients/cpp/random_player.cpp" -o "$CPP_CLIENT_BIN" \
  -L"$BOOST_PREFIX/lib" -lboost_system -pthread

echo "Compiling Java client..."
javac "$ROOT/clients/java/RandomPlayer.java"

if [ ! -d "$ROOT/.venv" ]; then
  echo "Creating Python venv..."
  python3 -m venv "$ROOT/.venv"
fi

source "$ROOT/.venv/bin/activate"
python - <<'PY'
try:
    import websockets  # noqa: F401
except Exception:
    raise SystemExit("Python 'websockets' is missing. Run: pip install websockets")
PY

for ((i=1; i<=MATCHES; i++)); do
  MATCH_PORT="$PORT"
  if [ "$MATCHES" -gt 1 ]; then
    MATCH_PORT=$((PORT + i - 1))
  fi

  if command -v lsof >/dev/null 2>&1; then
    retries=$PORT_WAIT_RETRIES
    while lsof -ti tcp:"$MATCH_PORT" >/dev/null 2>&1; do
      if [ "$retries" -le 0 ]; then
        echo "Port $MATCH_PORT still busy. Forcing cleanup."
        kill $(lsof -ti tcp:"$MATCH_PORT") 2>/dev/null || true
        break
      fi
      sleep 0.2
      retries=$((retries - 1))
    done
  fi
  if [ "$MATCHES" -gt 1 ]; then
    SERVER_LOG="$ROOT/.match_${i}.ws_server.log"
    P1_LOG="$ROOT/.match_${i}.p1.log"
    P2_LOG="$ROOT/.match_${i}.p2.log"
    P3_LOG="$ROOT/.match_${i}.p3.log"
    P4_LOG="$ROOT/.match_${i}.p4.log"
  else
    SERVER_LOG="$ROOT/.ws_server.log"
    P1_LOG="$ROOT/.p1.log"
    P2_LOG="$ROOT/.p2.log"
    P3_LOG="$ROOT/.p3.log"
    P4_LOG="$ROOT/.p4.log"
  fi

  echo "Starting server (match $i/$MATCHES)..."
  rm -f "$SERVER_LOG" "$P1_LOG" "$P2_LOG" "$P3_LOG" "$P4_LOG"
  "$SERVER_BIN" "$PLAYERS" "$SEED" "$MATCH_PORT" > "$SERVER_LOG" 2>&1 &
  SERVER_PID=$!
  trap 'kill "$SERVER_PID" 2>/dev/null || true' EXIT
  sleep "$SERVER_WAIT"

  echo "Starting clients..."
  python "$ROOT/clients/python/random_player.py" "ws://localhost:${MATCH_PORT}" "$ROOM" p1 > "$P1_LOG" 2>&1 &
  P1=$!
  sleep "$START_DELAY"
  java -cp "$ROOT/clients/java" RandomPlayer "ws://localhost:${MATCH_PORT}" "$ROOM" p2 > "$P2_LOG" 2>&1 &
  P2=$!
  sleep "$START_DELAY"
  "$CPP_CLIENT_BIN" "ws://localhost:${MATCH_PORT}" "$ROOM" p3 > "$P3_LOG" 2>&1 &
  P3=$!
  sleep "$START_DELAY"
  python "$ROOT/clients/python/random_player.py" "ws://localhost:${MATCH_PORT}" "$ROOM" p4 > "$P4_LOG" 2>&1 &
  P4=$!

  wait "$P1" "$P2" "$P3" "$P4"
  kill "$SERVER_PID" 2>/dev/null || true
  wait "$SERVER_PID" 2>/dev/null || true
  if kill -0 "$SERVER_PID" 2>/dev/null; then
    kill -9 "$SERVER_PID" 2>/dev/null || true
  fi
  echo "Match $i finished."
  sleep "$MATCH_PAUSE"
done
