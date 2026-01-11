# Java Client

## Usage

```bash
javac clients/java/RandomPlayer.java
java -cp clients/java RandomPlayer ws://localhost:9002 room1 p1
```

Requires JDK 11+ (uses `java.net.http.WebSocket`).

## How to Implement Your Own Client

- Connect to the WebSocket server and send a `join` message.
- Wait for `state` messages and act only when `legal` is non-empty.
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
