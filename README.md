# Tiger & Dragon

This repository contains rules documentation and a learning-focused C++ game engine for Tiger & Dragon.

## Learning Engine

The learning engine is a fast, communication-free core that models hands, attack/defend flow, and the one-lap bonus.
It is intended for offline training and self-play.

The multiplayer server should reuse this same engine for state transitions and rule validation so that training and live play stay consistent.
Keep the rules document in `RULES.md` as the shared source of truth.
See `docs/architecture.md` for the planned responsibilities and module layout of the multiplayer server and client.

Communication for the multiplayer server and AI clients uses WebSocket + JSON, so Python/C++/Java clients and a browser spectator can share the same protocol.

## Multiplayer WebSocket MVP

Protocol: `docs/protocol_ws_json.md`

### Server (C++)

Dependencies:
- `websocketpp` + standalone Asio (or Boost.Asio)

Example build (paths depend on your install):
```bash
g++ -std=c++17 -O2 -I./src -I/path/to/websocketpp -I/path/to/asio/include \\
  src/engine.cpp server/ws_server.cpp -o ws_server
./ws_server 4 42 9002
```

### Clients

Python:
```bash
pip install websockets
python clients/python/random_player.py ws://localhost:9002 room1 p1
```

Java (JDK 11+):
```bash
javac clients/java/RandomPlayer.java
java -cp clients/java RandomPlayer ws://localhost:9002 room1 p2
```

C++ (websocketpp + Asio):
```bash
g++ -std=c++17 -O2 -I/path/to/websocketpp -I/path/to/asio/include \\
  clients/cpp/random_player.cpp -o random_player_cpp
./random_player_cpp ws://localhost:9002 room1 p3
```

### Spectator (browser)

Open `clients/web/spectator.html?ws=ws://localhost:9002&room=room1` in a browser.

### Build sample simulation

```bash
g++ -std=c++17 -O2 -I./src src/engine.cpp src/learn_sim.cpp -o learn_sim
./learn_sim
```

### Build debug GUI (terminal)

```bash
g++ -std=c++17 -O2 -I./src src/engine.cpp src/random_player.cpp src/server_debug.cpp -o server_debug
./server_debug 4 42 0
```
The third argument selects the human player index; other players act randomly.
