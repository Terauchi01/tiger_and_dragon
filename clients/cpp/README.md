# C++ Client

## Usage

```bash
g++ -std=c++17 -O2 -I/opt/homebrew/opt/boost@1.85/include -I/opt/homebrew/include \
  clients/cpp/random_player.cpp -o random_player_cpp \
  -L/opt/homebrew/opt/boost@1.85/lib -lboost_system -pthread
./random_player_cpp ws://localhost:9002 room1 p1
```

Requires `websocketpp` and Boost (1.85 recommended for compatibility).

## How to Implement Your Own Client

- Connect to the WebSocket server and send a `join` message.
- Read `state` messages and act only when `legal` is non-empty.
- Send `action` with a `choice` from the `legal` list.

Message shapes are defined in `docs/protocol_ws_json.md`.

### Minimal Flow

```json
{"type":"join","room_id":"room1","player_id":"p1","role":"player"}
```

When you receive a `state` with `legal`, send:

```json
{"type":"action","room_id":"room1","player_id":"p1","choice":"7"}
```

`choice` can be `1`-`8`, `T`, `D`, or `pass`.
