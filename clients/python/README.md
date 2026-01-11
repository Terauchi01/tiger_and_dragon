# Python Client

## Usage

```bash
python clients/python/random_player.py ws://localhost:9002 room1 p1
```

Requires `websockets` (install in a venv):
```bash
python -m venv .venv
source .venv/bin/activate
pip install websockets
```

## How to Implement Your Own Client

- Connect to the WebSocket server and send a `join` message.
- Listen for `state` messages and act only when `legal` is non-empty.
- Send `action` with `choice` set to one of `legal` values.

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
