# Architecture

## Client-Server Message Set

The client and server exchange JSON messages grouped into the following categories. Each message lists the minimum required fields.

### Connection
- `connect_request`: `{ "player_id" }`
- `connect_ack`: `{ "player_id", "session_id" }`

### Room Join
- `room_join_request`: `{ "room_id", "player_id" }`
- `room_join_ack`: `{ "room_id", "player_id", "seat_id" }`

### Hand Submission
- `hand_submit`: `{ "room_id", "player_id", "turn_id", "hand" }`
- `hand_submit_ack`: `{ "room_id", "player_id", "turn_id", "accepted" }`

### State Update
- `state_update`: `{ "room_id", "turn_id", "state" }`
- `turn_result`: `{ "room_id", "turn_id", "results" }`

### Termination Notice
- `room_close`: `{ "room_id", "reason" }`
- `disconnect_notice`: `{ "player_id", "reason" }`
