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
- role: "player" or "spectator" (current server treats any non-"spectator" as a player)
- room_id: accepted but not validated; echoed in join_ack only

### action
```
{"type":"action","room_id":"room1","player_id":"p1","choice":"7"}
```
- choice: "1"-"8", "T", "D", or "pass"
- The server determines action type from the current phase.
- room_id and player_id are accepted but ignored by the current server
- choice is case-insensitive for "pass" and for "T"/"D"

### discards_request
```
{"type":"discards_request","room_id":"room1"}
```
- Request the current round's discards for all players.
- room_id is accepted but not validated by the current server.

## Server -> Client
### join_ack
```
{"type":"join_ack","room_id":"room1","player_id":"p1","seat":0,"players":4}
```
- seat: -1 for spectators
- room_id is echoed from the join payload and may differ from state.room_id

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
- turn starts at 0 and increments after each accepted action
- phase is one of "Attack", "Defend", "BonusReceive", or "Finished"
- attack_tile is "" when there is no active attack

### discards
```
{"type":"discards","room_id":"room1","player0_discards":"4A,7D,6A,B","player1_discards":"","player2_discards":"","player3_discards":""}
```
- playerN_discards: per-player discard list using "," between tokens.
- Each token is encoded as:
  - "<tile>A" for Attack
  - "<tile>D" for Defend
  - "B" for BonusReceive (tile is masked)
- Empty string means no discards for that player.
- player0_discards maps to seat 0, player1_discards to seat 1, etc.

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
- winner_discards uses "<tile><suffix>" with the tile shown for all actions, including BonusReceive.

### error
```
{"type":"error","message":"not your turn"}
```

### game_over
```
{"type":"game_over","winner":2,"scores":"10,4,6,2"}
```
