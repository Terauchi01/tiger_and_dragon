# Architecture

## Module Breakdown

### Server Modules

| モジュール | 主要責務 |
| --- | --- |
| API Gateway | HTTPリクエストの受付とルーティングを担う。 |
| Authentication | ユーザー認証とセッション管理を行う。 |
| Authorization | 権限チェックとアクセス制御を提供する。 |
| User Service | ユーザープロフィールとアカウント情報を管理する。 |
| Game Service | ゲーム進行ロジックと対局状態を管理する。 |
| Matchmaking | 対戦相手の検索とマッチングキューを管理する。 |
| Realtime Gateway | WebSocketなどのリアルタイム通信を中継する。 |
| Persistence | データベース読み書きとデータ整合性を担う。 |
| Cache | 頻繁に参照されるデータの高速取得を提供する。 |
| Job Scheduler | 非同期ジョブと定期タスクの実行を管理する。 |
| Notification | メールやプッシュ通知の送信を担当する。 |
| Observability | ログ・メトリクス・トレースの収集を行う。 |

### Client Modules

| モジュール | 主要責務 |
| --- | --- |
| App Shell | 画面全体の初期化と共通レイアウトを提供する。 |
| Routing | 画面遷移とURL状態の同期を管理する。 |
| Auth UI | ログイン/登録など認証関連の画面を提供する。 |
| Lobby | ロビー画面と対局開始の導線を管理する。 |
| Matchmaking UI | マッチング操作と待機状態の表示を担当する。 |
| Game Board | 盤面描画と入力受付を行う。 |
| Game HUD | スコアやターンなど対局情報の表示を提供する。 |
| Realtime Client | サーバとのリアルタイム通信を維持する。 |
| State Store | 共有状態管理とデータフローを整理する。 |
| API Client | サーバAPIへのリクエストとレスポンス処理を担う。 |
| Settings | 設定画面とユーザー設定の変更を管理する。 |
| Notifications UI | 通知バナーやアラートの表示を担当する。 |
| Error Handling | 例外時のフォールバックUIとログ送信を提供する。 |
