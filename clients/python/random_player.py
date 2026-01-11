import asyncio
import json
import random
import sys

import websockets


def parse_csv(value: str):
    value = value.strip()
    if not value:
        return []
    return [v.strip() for v in value.split(",") if v.strip()]


async def run(uri: str, player_id: str, room_id: str):
    async with websockets.connect(uri) as ws:
        print(f"Python client joining room={room_id} player_id={player_id}", flush=True)
        join_msg = {"type": "join", "room_id": room_id, "player_id": player_id, "role": "player"}
        await ws.send(json.dumps(join_msg))

        while True:
            raw = await ws.recv()
            msg = json.loads(raw)
            mtype = msg.get("type")
            if mtype == "state":
                legal = parse_csv(msg.get("legal", ""))
                if legal:
                    choice = random.choice(legal)
                    action = {
                        "type": "action",
                        "room_id": room_id,
                        "player_id": player_id,
                        "choice": choice,
                    }
                    await ws.send(json.dumps(action))
            elif mtype == "game_over":
                winner = msg.get("winner")
                print(f"game_over winner={winner}")
                return
            elif mtype == "error":
                print(f"error: {msg.get('message')}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: python random_player.py ws://localhost:9002 room1 p1")
        sys.exit(1)
    uri = sys.argv[1]
    room_id = sys.argv[2] if len(sys.argv) > 2 else "room1"
    player_id = sys.argv[3] if len(sys.argv) > 3 else "p1"
    asyncio.run(run(uri, player_id, room_id))
