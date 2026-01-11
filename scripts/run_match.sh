#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PLAYERS="${PLAYERS:-4}"
SEED="${SEED:-42}"
PORT="${PORT:-9002}"
ROOM="${ROOM:-room1}"

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

echo "Starting server..."
rm -f "$ROOT/.ws_server.log" "$ROOT/.p1.log" "$ROOT/.p2.log" "$ROOT/.p3.log" "$ROOT/.p4.log"
"$SERVER_BIN" "$PLAYERS" "$SEED" "$PORT" > "$ROOT/.ws_server.log" 2>&1 &
SERVER_PID=$!
trap 'kill "$SERVER_PID" 2>/dev/null || true' EXIT
sleep 1

echo "Starting clients..."
python "$ROOT/clients/python/random_player.py" "ws://localhost:${PORT}" "$ROOM" p1 > "$ROOT/.p1.log" 2>&1 &
P1=$!
java -cp "$ROOT/clients/java" RandomPlayer "ws://localhost:${PORT}" "$ROOM" p2 > "$ROOT/.p2.log" 2>&1 &
P2=$!
"$CPP_CLIENT_BIN" "ws://localhost:${PORT}" "$ROOM" p3 > "$ROOT/.p3.log" 2>&1 &
P3=$!
python "$ROOT/clients/python/random_player.py" "ws://localhost:${PORT}" "$ROOM" p4 > "$ROOT/.p4.log" 2>&1 &
P4=$!

wait "$P1" "$P2" "$P3" "$P4"
echo "Match finished."
