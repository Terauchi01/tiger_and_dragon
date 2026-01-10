# タイガー＆ドラゴン

このリポジトリは、タイガー＆ドラゴンのルール文書と学習向けのC++ゲームエンジンを含みます。

## 学習エンジン

学習エンジンは、手牌や攻め/受けの進行、1周ボーナスを通信なしで高速に扱うコアです。
オフライン学習と自己対戦を目的としています。

対戦サーバは学習と本番のルール差異を防ぐため、同じエンジンを用いて状態遷移と検証を行う想定です。
ルールの一次資料は `RULES.md` に集約します。
対戦サーバ/クライアントの責務と構成は `docs/architecture.md` を参照してください。

### 学習シミュレーションのビルド

```bash
g++ -std=c++17 -O2 -I./src src/engine.cpp src/learn_sim.cpp -o learn_sim
./learn_sim
```

### デバッグGUI（ターミナル）のビルド

```bash
g++ -std=c++17 -O2 -I./src src/engine.cpp src/server_debug.cpp -o server_debug
./server_debug 4 42
```
