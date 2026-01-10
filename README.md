# Tiger & Dragon

This repository contains rules documentation and a learning-focused C++ game engine for Tiger & Dragon.

## Learning Engine

The learning engine is a fast, communication-free core that models hands, attack/defend flow, and the one-lap bonus.
It is intended for offline training and self-play.

The multiplayer server should reuse this same engine for state transitions and rule validation so that training and live play stay consistent.
Keep the rules document in `RULES.md` as the shared source of truth.

### Build sample simulation

```bash
g++ -std=c++17 -O2 -I./src src/engine.cpp src/learn_sim.cpp -o learn_sim
./learn_sim
```
