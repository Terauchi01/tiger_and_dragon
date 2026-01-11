# Browser Spectator

## Usage

Open the HTML file in a browser:

```
clients/web/spectator.html?ws=ws://localhost:9002&room=room1
```

You can change the WebSocket URL and room in the query string:
- `ws`: WebSocket URL (default: `ws://localhost:9002`)
- `room`: room ID (default: `room1`)

## How to Implement Your Own Viewer

- Connect to the WebSocket server and send `join` with role `spectator`.
- Listen for `state` messages and render the fields you need.

Message shapes are defined in `docs/protocol_ws_json.md`.
