# Tiger & Dragon

このリポジトリは、タイガー＆ドラゴンのルールドキュメントと学習用C++ゲームエンジンを含みます。

## 学習用エンジン

学習用エンジンは通信なしで高速に動作し、手札・攻防・1周ボーナスを扱います。
対戦サーバは同じエンジンを利用して状態遷移とルール検証を行い、学習と対戦のルール差異を防ぎます。
ルールの一次資料は `RULES.md` に集約しています。
対戦サーバ/クライアントの設計メモは `docs/architecture.md` を参照してください。

対戦サーバとAIクライアントは WebSocket + JSON を共通プロトコルとして利用します。

## Multiplayer WebSocket MVP

プロトコル: `docs/protocol_ws_json.md`

### サーバ (C++)

依存:
- `websocketpp`
- `boost@1.85`（互換性のため推奨）

例:
```bash
g++ -std=c++17 -O2 -I./src -I/opt/homebrew/include -I/opt/homebrew/opt/boost@1.85/include \
  src/engine.cpp server/ws_server.cpp -o ws_server \
  -L/opt/homebrew/opt/boost@1.85/lib -lboost_system -pthread
./ws_server 4 42 9002
```

### クライアント

Python:
```bash
python -m venv .venv
source .venv/bin/activate
pip install websockets
python clients/python/random_player.py ws://localhost:9002 room1 p1
```

Java (JDK 11+):
```bash
javac clients/java/RandomPlayer.java
java -cp clients/java RandomPlayer ws://localhost:9002 room1 p2
```

C++ (websocketpp + Boost):
```bash
g++ -std=c++17 -O2 -I/opt/homebrew/include -I/opt/homebrew/opt/boost@1.85/include \
  clients/cpp/random_player.cpp -o random_player_cpp \
  -L/opt/homebrew/opt/boost@1.85/lib -lboost_system -pthread
./random_player_cpp ws://localhost:9002 room1 p3
```

### Web観戦

※ Web観戦機能は **現在未実装** です。

## 付属スクリプト

100戦まとめて実行:
```bash
MATCHES=100 ./scripts/run_match.sh
```

掃除:
```bash
./scripts/cleanup_match.sh
```

## 学習用サンプル

```bash
g++ -std=c++17 -O2 -I./src src/engine.cpp src/learn_sim.cpp -o learn_sim
./learn_sim
```

## デバッグGUI (ターミナル)

```bash
g++ -std=c++17 -O2 -I./src src/engine.cpp src/random_player.cpp src/server_debug.cpp -o server_debug
./server_debug 4 42 0
```
第3引数が人間プレイヤの番号で、それ以外はランダムに行動します。
