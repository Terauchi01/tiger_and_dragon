# WebSocket + JSON Protocol (MVP)

This document defines a minimal protocol for AI clients and spectators.

## Transport
- WebSocket, text frames only
- UTF-8 JSON objects per message

## Client -> Server
### join
```
{"type":"join","room_id":"room1","player_id":"p1","role":"player"}
```
- role: "player" or "spectator"

### action
```
{"type":"action","room_id":"room1","player_id":"p1","choice":"7"}
```
- choice: "1"-"8", "T", "D", or "pass"
- The server determines action type from the current phase.

## Server -> Client
### join_ack
```
{"type":"join_ack","room_id":"room1","player_id":"p1","seat":0,"players":4}
```

### state
```
{
  "type":"state",
  "room_id":"room1",
  "turn":12,
  "phase":"Attack",
  "current_player":0,
  "attack_tile":"7",
  "hand":"8,6,4,7,T",
  "hand_sizes":"7,7,7,7",
  "bonus_discards":"0,1,0,0",
  "legal":"7,8,pass",
  "scores":"4,0,0,0"
}
```
- hand: only for the recipient player; spectators receive "".
- legal: only for the current player; others receive "".

### round_result
```
{
  "type":"round_result",
  "winner":2,
  "last_tile":"7",
  "bonus_discards":2,
  "round_points":6,
  "winner_hand":"",
  "winner_hand_size":0,
  "winner_discards":"7A,4D,TB",
  "scores":"0,0,6,0",
  "round":1
}
```

### error
```
{"type":"error","message":"not your turn"}
```

### game_over
```
{"type":"game_over","winner":2,"scores":"10,4,6,2"}
```
