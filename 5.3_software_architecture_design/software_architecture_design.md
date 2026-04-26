# ソフトウェアアーキテクチャ設計書(SAD)

**ドキュメント ID:** SAD-TH25-001
**バージョン:** 0.1
**作成日:** 2026-04-26
**対象製品:** 仮想 Therac-25(Virtual Therac-25) / TH25-SIM-001
**対象ソフトウェアバージョン:** 0.2.0(Inc.1 完了で初回機能リリース予定)
**安全クラス:** C(IEC 62304)
**変更要求:** CR-0003(GitHub Issue #3)

| 役割 | 氏名 | 所属 | 日付 | 署名 |
|------|------|------|------|------|
| 作成者 | k-abe | — | 2026-04-26 | |
| レビュー者 | k-abe(セルフ、CCB §5.4 1 分インターバル後) | — | 2026-04-26 | |
| 承認者 | k-abe(セルフ承認、CCB §3 単独開発兼任) | — | 2026-04-26 | |

> **本プロジェクトの位置づけ(注記)**
>
> 本ドキュメントは IEC 62304 に基づく医療機器ソフトウェア開発プロセスの学習・参考実装を目的とした **仮想プロジェクト** の成果物である。題材は 1985-1987 年に放射線過剰照射事故を起こした実在の医療機器 **Therac-25**(AECL)である。本書の記述は、当時の AECL 社内文書ではなく、公開された事故報告書・学術文献(Leveson & Turner 1993 ほか)に基づく **学習用の再構成** である。
>
> **本 v0.1 は Inc.1(ビーム生成・モード制御コア)範囲のアーキテクチャを中心に確定する初版**である。Inc.2(インターロック/安全監視)・Inc.3(タスク同期・タイミング保証)・Inc.4(操作者 UI・エラー報告・ロギング)に該当するアーキテクチャ要素も、Inc.1 の **構造的前提**(分離設計・メッセージパッシング)を成立させる範囲で本 v0.1 に含める。詳細(各要素内のソフトウェアユニット)は SDD-TH25-001 v0.1(Step 16)で展開する。
>
> **本書の核心:** SRS-RCM-018(UI 層と安全コアの分離、IEC 62304 §5.3.5)と SRS-RCM-019(タスク間メッセージパッシング)を、本 SAD で **OS プロセス分離 + lock-free キューによるメッセージパッシング** として構造的に確定する。これは Therac-25 race condition(Tyler 第 1 例)の根本構造(UI と制御の同一プロセス・共有メモリ動作)を **アーキテクチャ時点で構造的に不可能化** することを目的とする。

---

## 1. 目的と適用範囲

### 1.1 目的

本書は、SRS-TH25-001 v0.1 で定義された要求事項を実現するための仮想 Therac-25(TH25-SIM-001)制御ソフトウェア(TH25-CTRL)のアーキテクチャを IEC 62304 箇条 5.3 に基づいて定義する。

具体的には以下を成立させる:

- **箇条 5.3.1**: SRS の全要求事項をアーキテクチャ要素(ARCH-NNN)に割り付ける(§3 / §11.1)
- **箇条 5.3.2**: ソフトウェア項目間のインタフェース(IF-U-NNN)および外部インタフェース(IF-E-NNN)を定義する(§5)
- **箇条 5.3.3 / 5.3.4**(クラス B/C 必須): SOUP 13 件の機能要求・HW/SW 要求を SRS への割付として明示する(§7 / §8)
- **箇条 5.3.5**(クラス C 必須): リスクコントロール手段のためのソフトウェア項目の分離(SEP-NNN)を定義し、Therac-25 race condition を構造的に不可能化する設計安全を確立する(§9)
- **箇条 5.3.6**: アーキテクチャの検証可能性を確認し、SDD(Step 16)以降への前提を確立する(§10)

### 1.2 適用範囲

- 対象ソフトウェア: TH25-CTRL(C++20 で実装される仮想シミュレータ制御ソフトウェア)
- 対象バージョン: 0.2.0 以降(本 v0.1 SAD は Inc.1 範囲を中心、Inc.2〜4 のアーキテクチャ要素は構造的前提範囲のみ確定)
- 対象 SRS: SRS-TH25-001 v0.1 全要求事項(SRS-001〜012、SRS-I-001〜008、SRS-O-001〜005、SRS-IF-001〜005、SRS-ALM-001〜005、SRS-SEC-001/002、SRS-UX-001〜003、SRS-D-001〜010、SRS-REG-001〜004、SRS-RCM-001/005/008/017/018/019、SRS-101〜104)
- 対象 RMF: RMF-TH25-001 v0.1 の HZ-001〜010 / RCM-001〜019
- スコープ外: 詳細設計(SDD で扱う)、実装コード、試験(本書ではトレーサビリティ枠組みのみ提供)

### 1.3 用語と略語

本書で使用する略語は CLAUDE.md「英語略語の統一」表に従う。本書固有の用語は以下のとおり。

| 用語 | 定義 |
|------|------|
| 安全コア(Safety Core) | TH25-CTRL の中核プロセス。SRS の機能要求(モード制御・ビーム制御・ドーズ管理・インターロック等)を実現する主要処理。本書で OS プロセスとして UI 層・シミュレータ層から分離する |
| 操作者 UI 層(Operator UI) | 操作者との対話を担当する別プロセス。SRS-UX-001〜003 / SRS-SEC-001/002 を実現する。Inc.4 で本格化 |
| ハードウェアシミュレータ層(Hardware Simulator) | 仮想ハードウェア(電子銃・ベンディング磁石・ターンテーブル・イオンチェンバ)を模擬する別プロセス。SRS-IF-001〜004 を実装 |
| メッセージバス(MessageBus) | プロセス内のタスク間通信を担う lock-free queue 抽象。`folly::ProducerConsumerQueue` 等の SPSC/MPSC 実装を採用予定(SDD で確定) |
| プロセス境界(Process Boundary) | OS のプロセス分離機能(別 PID、別仮想アドレス空間、別ファイル記述子テーブル)による隔離。本書 §9 の SEP-001/SEP-002 の中核 |
| メッセージパッシング(Message Passing) | 共有変数を介さず、複製済み確定データを queue または IPC channel で送受信する通信方式。SRS-RCM-019 / SEP-003 |
| イベントループ(Event Loop) | 単一スレッドが queue からメッセージを順次取得・処理する制御フロー。順序保証と race condition 回避を両立する |

## 2. 参照文書

| ID | 文書名 | バージョン |
|----|--------|----------|
| [1] | IEC 62304:2006+A1:2015 / JIS T 2304:2017 ヘルスソフトウェア — ソフトウェアライフサイクルプロセス | — |
| [2] | ISO 14971:2019 医療機器 — リスクマネジメントの医療機器への適用 | — |
| [3] | IEC 60601-1-8 アラームシステム | — |
| [4] | IEC 62366-1 ユーザビリティエンジニアリングの医療機器への適用 | — |
| [5] | ISO/IEC 25010:2011 システム及びソフトウェア品質モデル | — |
| [6] | ソフトウェア要求仕様書(SRS-TH25-001) | 0.1 |
| [7] | リスクマネジメントファイル(RMF-TH25-001) | 0.1 |
| [8] | ソフトウェア開発計画書(SDP-TH25-001) | 0.1 |
| [9] | ソフトウェアリスクマネジメント計画書(SRMP-TH25-001) | 0.1 |
| [10] | ソフトウェア構成管理計画書(SCMP-TH25-001) | 0.1 |
| [11] | ソフトウェア安全クラス決定記録(SSC-TH25-001) | 0.1 |
| [12] | ソフトウェア問題解決手順書(SPRP-TH25-001) | 0.1 |
| [13] | ソフトウェア保守計画書(SMP-TH25-001) | 0.1 |
| [14] | 構成アイテム一覧(CIL-TH25-001) | 0.6 |
| [15] | CCB 運用規程(CCB-TH25-001) | 0.1 |
| [16] | 変更要求台帳(CRR-TH25-001) | 0.4 |
| [17] | N. G. Leveson, C. S. Turner. "An Investigation of the Therac-25 Accidents." IEEE Computer, 26(7):18-41, 1993. | — |
| [18] | N. G. Leveson. *Safeware: System Safety and Computers*. Addison-Wesley, 1995(Appendix A). | — |
| [19] | ソフトウェア詳細設計書(SDD-TH25-001、Step 16 で作成) | — |

## 3. ソフトウェア要求事項のソフトウェアアーキテクチャへの変換(箇条 5.3.1)

### 3.1 変換の方針

本書では SRS-TH25-001 v0.1 の全要求事項を以下の優先順位でアーキテクチャ要素(ARCH-NNN)に割り付ける。

1. **構造的 RCM の優先確定**: SRS-RCM-018 / SRS-RCM-019 は、他のすべての機能 RCM(SRS-RCM-001/005/008/017)が動作する **前提条件** であるため、最初に分離設計(§9)で確定する
2. **ハザード直接対応の機能 RCM**: HZ-001(電子モード+ターゲット非挿入)に直接対応する SRS-001〜SRS-007 を中核アーキテクチャ要素(モード制御・ビーム制御・ターンテーブル管理)に割付け
3. **ハザード関連の状態管理・監視**: HZ-005(ドーズ計算誤り)・HZ-009(電源喪失再起動)に対応する SRS-008〜012 を独立アーキテクチャ要素(ドーズ管理・起動自己診断)に割付け
4. **入出力インタフェース**: SRS-I-001〜008 / SRS-O-001〜005 / SRS-IF-001〜005 を §5 内部 IF / 外部 IF として定義
5. **後続 Inc 範囲の構造的前提**: SRS-ALM-001〜005(Inc.2 主)/ SRS-SEC-001/002(Inc.4 主)/ SRS-UX-001〜003(Inc.4 主)/ SRS-O-005(Inc.4 主)について、本 v0.1 で対応するアーキテクチャ要素の **空殻**(プレースホルダ)を確定。詳細は後続 Inc の SAD 改訂で追加

### 3.2 割付完全性の保証

§11.1 トレーサビリティマトリクスで全 SRS 要求事項に対して 1 つ以上の ARCH-NNN を割付ける。割付なしの SRS 要求がある場合は本 SAD の不備とみなし、CR を起票して改訂する。

## 4. ソフトウェアアーキテクチャの概要

### 4.1 アーキテクチャ方針

| 観点 | 採用方針 | 採用理由(Therac-25 教訓との対応含む) |
|------|---------|-----------------------------------|
| プロセス構成 | **マルチプロセス**(操作者 UI / 安全コア / ハードウェアシミュレータの 3 プロセス) | OS プロセス分離による強い隔離(SRS-RCM-018 / IEC 62304 §5.3.5)。Therac-25 は UI と制御を同一プロセス・共有メモリで動作させたことが Tyler 第 1 例 race condition の構造的要因(Leveson & Turner 1993 §IV.B)。本プロジェクトは構造的にこれを不可能化する |
| プロセス内構成 | **イベントループ + 独立監視タスク**(各プロセス内は単一イベントループスレッド + 独立した監視/IO スレッド群、共有状態はすべて lock-free queue 経由でメッセージパッシング) | SRS-RCM-019 のメッセージパッシングを設計時点で強制。共有変数を「設計上禁止」とすることで、Therac-25 当時の race condition 類型(共有変数アクセスのメモリバリアなし競合)を構造的に排除 |
| インタフェーススタイル | **抽象 IF + DI(依存性注入)**(`IElectronGun` / `IBendingMagnet` / `ITurntable` / `IIonChamber` / `IAuthenticator` 等の純粋仮想クラス、SRS-IF-001〜005) | テスト容易性(モック差替)、シミュレータと将来の実 HW ドライバの差替容易性、SDD §5.4.3(クラス C 必須 IF 詳細設計)で IF を独立検証可能 |
| エラー処理 | **全エラーを構造化型(`enum class ErrorCode` + 詳細メッセージ)で扱い、ビームオン経路は明示的な `Result<T, ErrorCode>` 型で結果を返す(`std::expected` 相当)** | Therac-25 の暗号的エラーコード単独表示(MALFUNCTION 54、HZ-006)への構造的対応。SRS-UX-001 のメッセージ構造化要求を型システムで強制 |
| ロギング | **append-only 構造化ログ(JSON Lines、UTC ISO 8601 時刻)**、安全コアの専用タスクが書込担当 | SRS-O-005 / SRS-D-010 / RCM-011 への対応。後段(Inc.4)で完成だが、本 v0.1 で IF-U-007(Orchestrator → AuditLogger)とプレースホルダタスクを確定 |
| 並行処理モデル | **C++20 `std::thread` + `std::atomic` + lock-free queue**、共有変数禁止 | SDP §6.1 / SRS-D-009 / RCM-002 / RCM-004 / RCM-019 と整合。clang-tidy `concurrency-*` ルール群で機械的検出(Step 14 で確立済) |
| ライフサイクル | **起動時自己診断 → 治療開始準備 → ビームオン/オフ → 終了** の状態機械 | SRS-012 / RCM-013 への対応。状態遷移を `enum class LifecycleState` で型安全化 |

### 4.2 全体構成図

```
+--------------------------------------------------------------+
|                       Operator Workstation                   |
|                                                              |
|  +------------------------------------------------------+    |
|  |  ARCH-001: Operator UI Process  (Inc.4 で本格実装)  |    |
|  |    - PrescriptionEditor                              |    |
|  |    - BeamCommandConsole                              |    |
|  |    - AlarmDisplay (SRS-UX-001/002)                   |    |
|  |    - AuthenticationGateway (SRS-SEC-001)             |    |
|  +------------------------------------------------------+    |
|        ^   IF-E-001 (gRPC over Unix Domain Socket;          |
|        |    確定済データのみ、共有メモリ禁止)                |
|        |    -> SEP-001 / RCM-018                            |
|        v                                                     |
|  +------------------------------------------------------+    |
|  |  ARCH-002: Safety Core Process  (Inc.1 中核)        |    |
|  |                                                      |    |
|  |  +-------------------------------------------+      |    |
|  |  |  ARCH-002.1 SafetyCoreOrchestrator        |      |    |
|  |  |     (イベントループ・メッセージディス     |      |    |
|  |  |      パッチ、単一スレッド)                |      |    |
|  |  +-------------------------------------------+      |    |
|  |     |                |                |              |    |
|  |     v                v                v              |    |
|  |  +----------+  +-------------+  +------------+      |    |
|  |  | 002.2    |  | 002.3       |  | 002.4      |      |    |
|  |  | Treatment|  | Beam        |  | Dose       |      |    |
|  |  | Mode     |  | Controller  |  | Manager    |      |    |
|  |  | Manager  |  |             |  |            |      |    |
|  |  +----------+  +-------------+  +------------+      |    |
|  |     |                |                |              |    |
|  |     v                v                v              |    |
|  |  +----------+  +-------------+  +------------+      |    |
|  |  | 002.5    |  | 002.6       |  | 002.7      |      |    |
|  |  | Turntable|  | Bending     |  | Safety     |      |    |
|  |  | Manager  |  | Magnet      |  | Monitor    |      |    |
|  |  |          |  | Manager     |  | (Inc.2)    |      |    |
|  |  +----------+  +-------------+  +------------+      |    |
|  |                                                      |    |
|  |  +----------+  +-------------+  +------------+      |    |
|  |  | 002.8    |  | 002.9       |  | 002.10     |      |    |
|  |  | Startup  |  | Audit       |  | Auth       |      |    |
|  |  | SelfCheck|  | Logger      |  | Gateway    |      |    |
|  |  |          |  | (Inc.4)     |  | (Inc.4)    |      |    |
|  |  +----------+  +-------------+  +------------+      |    |
|  |                                                      |    |
|  |  ARCH-004: MessageBus  (lock-free queue)            |    |
|  |     RCM-019 / SEP-003: 全タスク間通信               |    |
|  +------------------------------------------------------+    |
|        ^   IF-E-002 (gRPC over Unix Domain Socket;          |
|        |    SEP-002)                                         |
|        v                                                     |
|  +------------------------------------------------------+    |
|  |  ARCH-003: Hardware Simulator Process                |    |
|  |    - 003.1 ElectronGunSim    (SRS-IF-001)            |    |
|  |    - 003.2 BendingMagnetSim  (SRS-IF-002)            |    |
|  |    - 003.3 TurntableSim      (SRS-IF-003、3 系統)    |    |
|  |    - 003.4 IonChamberSim     (SRS-IF-004、2 組)      |    |
|  +------------------------------------------------------+    |
|                                                              |
|  IF-E-003: Append-only audit log file (JSON Lines)          |
|  -> Filesystem (SRS-O-005)                                  |
+--------------------------------------------------------------+
```

### 4.3 ソフトウェア項目一覧

ソフトウェア項目(Software Item)とソフトウェアユニット(Software Unit)を階層的に識別する。本表のうち **ユニット** レベル(`ARCH-002.x.y`)は SDD-TH25-001 v0.1(Step 16)で詳細化する。本 SAD では「項目」レベル(`ARCH-NNN` および `ARCH-NNN.x`)を確定する。

| 項目 ID | 名称 | 分類 | 安全クラス | プロセス | Inc.確定範囲 | 概要(対応 SRS / RCM) |
|---------|------|------|----------|---------|-------------|---------------------|
| ARCH-001 | Operator UI | 項目 | C(分離後 §9) | Operator UI Process(別プロセス) | Inc.4 で本格化、本 v0.1 では IF とプレースホルダのみ | 操作者との対話。SRS-I-001〜004 / SRS-O-004 / SRS-UX-001〜003 / SRS-SEC-001/002 |
| ARCH-001.1 | PrescriptionEditor | 項目 | C(分離後 §9) | ARCH-001 内 | Inc.4 | 治療計画(モード/エネルギー/ドーズ)入力 UI(SRS-I-001/002/003 / SRS-UX-003) |
| ARCH-001.2 | BeamCommandConsole | 項目 | C(分離後 §9) | ARCH-001 内 | Inc.4 | ビームオン/オフ操作 UI(SRS-I-004) |
| ARCH-001.3 | AlarmDisplay | 項目 | C(分離後 §9) | ARCH-001 内 | Inc.4 | アラーム/エラーメッセージ表示(SRS-O-004 / SRS-ALM-001〜005 / SRS-UX-001/002) |
| ARCH-001.4 | UIAuthenticationGateway | 項目 | C(分離後 §9) | ARCH-001 内 | Inc.4 | 操作者認証 UI(SRS-SEC-001 / RCM-015) |
| ARCH-002 | Safety Core | 項目 | **C** | Safety Core Process(本体) | **Inc.1 中核** | TH25-CTRL の中核処理。本書 §3.1 で確立する全 SRS 要求の主担当 |
| ARCH-002.1 | SafetyCoreOrchestrator | 項目 | C | ARCH-002 内 | Inc.1 中核 | イベントループ(単一スレッド)。MessageBus からメッセージを取得し、各サブ項目へディスパッチ。状態機械 `LifecycleState` を保持(SRS-002 / SRS-007 / SRS-012 / RCM-001 / RCM-013 / RCM-017) |
| ARCH-002.2 | TreatmentModeManager | 項目 | C | ARCH-002 内 | **Inc.1 中核** | 治療モード(`enum class TreatmentMode`)の状態機械。モード遷移整合性チェック(SRS-001 / SRS-002 / SRS-003 / SRS-RCM-001 / RCM-001、HZ-001 直接対応) |
| ARCH-002.3 | BeamController | 項目 | C | ARCH-002 内 | **Inc.1 中核** | ビームオン/オフ指令の発行と即時遷移保証(SRS-007 / SRS-011 / SRS-101 / SRS-RCM-017 / RCM-017、HZ-001 / HZ-004 対応) |
| ARCH-002.4 | DoseManager | 項目 | C | ARCH-002 内 | **Inc.1 中核** | ドーズ目標管理・積算カウンタ・目標到達検知(SRS-008 / SRS-009 / SRS-010 / SRS-103 / SRS-RCM-008 / RCM-008、HZ-005 直接対応)。**`std::atomic<uint64_t>` で実装、HZ-003 整数オーバフロー類型を構造的に排除(RCM-003 部分実装、Inc.3 で詳細要求化)** |
| ARCH-002.5 | TurntableManager | 項目 | C | ARCH-002 内 | **Inc.1 中核** | ターンテーブル位置指令・3 系統センサ集約(SRS-002 / SRS-006 / SRS-I-005 / SRS-104 / SRS-RCM-005 / RCM-005、HZ-001 / HZ-003 対応)。3 系統独立センサの値集約・偏差判定 |
| ARCH-002.6 | BendingMagnetManager | 項目 | C | ARCH-002 内 | **Inc.1 中核** | ベンディング磁石電流指令・計測値偏差監視(SRS-005 / SRS-I-007 / SRS-O-002 / SRS-ALM-004) |
| ARCH-002.7 | SafetyMonitor | 項目 | **C(分離後)** | ARCH-002 内、独立タスク | Inc.2 で本格化、本 v0.1 では IF と空殻のみ | 独立並行タスクとしてのインターロック監視(SRS-ALM-001/002/003/004/005 / SRS-RCM-005 / RCM-005 / RCM-006 / RCM-007、HZ-004 / HZ-008 対応)。ARCH-002.1 とは MessageBus 経由で疎結合 |
| ARCH-002.8 | StartupSelfCheck | 項目 | C | ARCH-002 内 | **Inc.1 中核** | 起動時自己診断(SRS-012 / SRS-ALM-005 / RCM-013、HZ-009 部分対応) |
| ARCH-002.9 | AuditLogger | 項目 | C | ARCH-002 内、独立 IO タスク | Inc.4 で本格化、本 v0.1 では IF と空殻のみ | append-only 構造化ログ書込(SRS-O-005 / SRS-D-010 / RCM-011) |
| ARCH-002.10 | CoreAuthenticationGateway | 項目 | C | ARCH-002 内 | Inc.4 で本格化、本 v0.1 では IF と空殻のみ | 操作者認証検証・処方データ完全性検証(SRS-SEC-001 / SRS-SEC-002 / RCM-015 / RCM-016) |
| ARCH-003 | Hardware Simulator | 項目 | **B(分離後 §9)** | Hardware Simulator Process(別プロセス) | **Inc.1 中核** | 仮想ハードウェア(電子銃/磁石/ターンテーブル/イオンチェンバ)の挙動模擬。SRS-IF-001〜004 を実装側として担当 |
| ARCH-003.1 | ElectronGunSim | 項目 | B(分離後) | ARCH-003 内 | Inc.1 | 仮想電子銃の応答模擬(SRS-IF-001 / SRS-I-008 / SRS-O-001) |
| ARCH-003.2 | BendingMagnetSim | 項目 | B(分離後) | ARCH-003 内 | Inc.1 | 仮想ベンディング磁石の応答模擬(SRS-IF-002 / SRS-I-007 / SRS-O-002) |
| ARCH-003.3 | TurntableSim | 項目 | B(分離後) | ARCH-003 内 | Inc.1 | 仮想ターンテーブル(3 系統独立センサ)の応答模擬(SRS-IF-003 / SRS-I-005 / SRS-O-003)。**3 系統センサ間の故障注入機能を組込み**(故障モード試験用、IT/ST で活用) |
| ARCH-003.4 | IonChamberSim | 項目 | B(分離後) | ARCH-003 内 | Inc.1 | 仮想イオンチェンバ(2 組)の応答模擬(SRS-IF-004 / SRS-I-006)。サチュレーション・片系失陥模擬機能を組込み(Inc.2 RCM-007 検証用) |
| ARCH-004 | MessageBus | 項目 | C | ARCH-002 内(Safety Core 内のタスク間通信)、ARCH-001/003 にも各プロセス内に配置 | **Inc.1 中核** | プロセス内タスク間通信の lock-free queue 抽象。`folly::ProducerConsumerQueue` 等の SPSC 実装、または同等の MPSC 実装(SDD で確定)。**SRS-RCM-019 / RCM-019 / SEP-003 の構造的実体** |
| ARCH-004.1 | InProcessQueue | ユニット | C | ARCH-004 内 | Inc.1 | プロセス内 lock-free queue 実装(SDD §5.4.2 で詳細化) |
| ARCH-004.2 | InterProcessChannel | ユニット | C | ARCH-004 内 | Inc.1 | プロセス間通信ラッパ(gRPC over UDS)。SRS-IF-005 を実装側として担当(SDD §5.4.2 で詳細化) |

> **クラス分離の根拠(§9 と整合):** ARCH-001(Operator UI Process)と ARCH-003(Hardware Simulator Process)は、§9 SEP-001 / SEP-002 によりプロセス分離下で **クラス B 相当** に低クラス化可能(IEC 62304 §5.3.6)。ただし本 SAD では学習目的および安全余裕確保のため、UI 側はクラス C を維持する(分離はあるが UI 側のバグが処方データを誤って送信する経路を排除しきれないため)。Hardware Simulator は分離後クラス B(物理 HW を模擬する非治療経路、治療結果に直接寄与しない)に低クラス化する。

## 5. ソフトウェア項目間のインタフェース(箇条 5.3.2)

### 5.1 内部 IF(プロセス内、`IF-U-NNN`)

プロセス内のソフトウェア項目間 IF は、本 SAD では **MessageBus 経由のメッセージパッシング**(RCM-019)を **唯一の通信手段** として規定する。直接 API 呼出は同期処理(同一スレッド内のサブルーチン呼出)に限定し、共有変数アクセスは一切行わない(SRS-D-009)。

| IF ID | 呼出側 | 被呼出側 | 種別 | 仕様(メッセージ型・同期/非同期) | 関連 SRS / RCM |
|-------|-------|---------|------|--------------------------------|--------------|
| IF-U-001 | ARCH-002.1 SafetyCoreOrchestrator | ARCH-002.2 TreatmentModeManager | メッセージ(MessageBus 経由) | `ModeChangeRequest{TreatmentMode, Energy}` / `ModeChangeResult{ErrorCode, TreatmentMode}` | SRS-001 / SRS-002 / RCM-001 |
| IF-U-002 | ARCH-002.1 | ARCH-002.3 BeamController | メッセージ | `BeamOnRequest{}` / `BeamOffRequest{}` / `BeamStateChange{BeamState, ErrorCode}` | SRS-007 / SRS-011 / RCM-017 |
| IF-U-003 | ARCH-002.1 | ARCH-002.4 DoseManager | メッセージ | `SetDoseTarget{DoseUnit_cGy}` / `DoseAccumulated{DoseUnit_cGy, PulseCount}` / `DoseTargetReached{}` | SRS-008 / SRS-009 / SRS-010 / RCM-008 |
| IF-U-004 | ARCH-002.1 | ARCH-002.5 TurntableManager | メッセージ | `MoveTurntable{Position_mm}` / `TurntablePositionReport{Position_mm[3], Discrepancy_mm}` | SRS-002 / SRS-006 / SRS-I-005 / RCM-005 |
| IF-U-005 | ARCH-002.1 | ARCH-002.6 BendingMagnetManager | メッセージ | `SetMagnetCurrent{MagnetCurrent_A}` / `MagnetCurrentReport{MagnetCurrent_A, Deviation_pct}` | SRS-005 / SRS-I-007 / SRS-O-002 |
| IF-U-006 | ARCH-002.7 SafetyMonitor → ARCH-002.1 | ARCH-002.1 ← ARCH-002.7 | メッセージ(独立タスク → イベントループ、非同期) | `SafetyAlarm{AlarmId, Severity, Detail}` / `SafetyAlarmAck{AlarmId}` | SRS-ALM-001〜005 / RCM-006 / RCM-007(Inc.2 で完成) |
| IF-U-007 | 全項目(ログ書込元) | ARCH-002.9 AuditLogger | メッセージ(独立 IO タスク、非同期) | `AuditLogEntry{Timestamp_ISO8601_UTC, EventType, Fields...}` | SRS-O-005 / SRS-D-010 / RCM-011(Inc.4 で完成) |
| IF-U-008 | 全項目(送受信元) | ARCH-004 MessageBus | API(producer/consumer) | `template<typename Msg> bool try_publish(Msg&&)`, `template<typename Msg> std::optional<Msg> try_consume()`, `void publish_blocking(Msg&&)`(同期版、queue 容量超過時 fail-stop) | SRS-RCM-019 / RCM-019 |
| IF-U-009 | ARCH-002.1 | ARCH-002.8 StartupSelfCheck | API(同期、起動時のみ) | `StartupCheckResult perform_self_check()` (戻り値: 成功/失敗の構造化結果) | SRS-012 / RCM-013 |
| IF-U-010 | ARCH-002.1 | ARCH-002.10 CoreAuthenticationGateway | メッセージ | `AuthenticationVerify{OperatorId, Credential}` / `AuthenticationResult{ErrorCode, OperatorId}` | SRS-SEC-001 / RCM-015(Inc.4 で完成) |

### 5.2 外部 IF(プロセス間 / プロセス外、`IF-E-NNN`)

| IF ID | 端点 A | 端点 B | プロトコル・トランスポート | データ形式 | 関連 SRS / RCM / SEP |
|-------|-------|-------|---------------------------|----------|---------------------|
| IF-E-001 | ARCH-001 Operator UI Process | ARCH-002 Safety Core Process | **gRPC over Unix Domain Socket**(候補、SDD で確定。代替: ZeroMQ / 名前付きパイプ + 構造化メッセージ)。**共有メモリ禁止、確定済データのみ送受信** | Protocol Buffers スキーマ(SDD §5.4.3 で確定)。主要メッセージ: `PrescriptionSet`, `BeamCommand`, `StateNotification`, `AlarmNotification` | SRS-IF-005 / SRS-RCM-018 / RCM-018 / **SEP-001** |
| IF-E-002 | ARCH-002 Safety Core Process | ARCH-003 Hardware Simulator Process | gRPC over Unix Domain Socket(同上) | Protocol Buffers スキーマ。主要メッセージ: `ElectronGunCommand`, `MagnetCommand`, `TurntableCommand`, `SensorReadResponse` | SRS-IF-001/002/003/004 / **SEP-002** |
| IF-E-003 | ARCH-002.9 AuditLogger | OS ファイルシステム | POSIX `open(O_APPEND \| O_CREAT, 0600)` + `fdatasync()`(各エントリ書込後)/ JSON Lines | UTC ISO 8601 時刻 + イベント種別 + 詳細フィールド(構造化 JSON、各行 1 エントリ) | SRS-O-005 / SRS-D-010 / RCM-011(Inc.4 で完成) |
| IF-E-004 | ARCH-001 Operator UI Process | OS 入出力(端末/GUI ライブラリ) | Inc.4 で確定(候補: ImGui, Dear ImGui+GLFW、または curses ベース TUI) | SDD で確定 | SRS-UX-001〜003(Inc.4 で完成) |

> **IF-E-001 / IF-E-002 のメッセージスキーマは SDD-TH25-001 v0.1(Step 16)§5.4.3 で確定する**。本 SAD では「gRPC over UDS を第一候補とし、共有メモリ禁止」という構造的制約のみを確定する(SEP-001 / SEP-002 / RCM-018 への対応)。

## 6. SOUP の識別

CIL-TH25-001 v0.6 §5 で正式登録(Step 14)した 13 件の SOUP を本 SAD の対象として再掲する。本書では各 SOUP の機能要求(§7)・HW/SW 要求(§8)を割付ける。

| SOUP ID | 名称 | バージョン | 用途 | 入手元 | ライセンス | 関連 ARCH |
|---------|------|----------|------|--------|----------|----------|
| SOUP-001 | clang++ | 17.0+ | C++20 コンパイラ(主) | LLVM プロジェクト(GitHub Release) | Apache 2.0 with LLVM Exceptions | 全 ARCH(ビルド時) |
| SOUP-002 | g++ | 13.0+ | C++20 コンパイラ(副、CI でクロス検証) | GCC プロジェクト | GPL v3 + GCC Runtime Library Exception | 全 ARCH(ビルド時) |
| SOUP-003 | libc++ | 17.0+(SOUP-001 同梱) | C++ 標準ライブラリ実装(主) | LLVM プロジェクト | Apache 2.0 with LLVM Exceptions | 全 ARCH(ランタイム) |
| SOUP-004 | libstdc++ | 13.0+(SOUP-002 同梱) | C++ 標準ライブラリ実装(副) | GCC プロジェクト | GPL v3 + GCC Runtime Library Exception | 全 ARCH(ランタイム) |
| SOUP-005 | CMake | 3.25+ | ビルドシステム | Kitware | BSD 3-Clause | 全 ARCH(ビルド時) |
| SOUP-006 | Ninja | 1.11+ | ビルド実行エンジン | ninja-build プロジェクト | Apache 2.0 | 全 ARCH(ビルド時) |
| SOUP-007 | vcpkg | manifest mode、baseline `c3867e714dd3a51c272826eea77267876517ed99`(2026.03.18) | C++ パッケージ管理 | Microsoft | MIT | 全 ARCH(ビルド時) |
| SOUP-008 | GoogleTest | 1.14+ | ユニット試験フレームワーク | Google(GitHub) | BSD 3-Clause | tests/ 配下、ARCH-002 の UT |
| SOUP-009 | GoogleMock | 1.14+(SOUP-008 同梱) | モックフレームワーク | Google | BSD 3-Clause | tests/ 配下、ARCH-002 の UT(IF-U-001〜010 のモック化) |
| SOUP-010 | clang-tidy | 17.0+(SOUP-001 同梱) | 静的解析(MISRA C++ 2023 / CERT C++ 補助) | LLVM プロジェクト | Apache 2.0 with LLVM Exceptions | 全 ARCH(ビルド時) |
| SOUP-013 | AddressSanitizer ランタイム | clang/gcc 同梱 | ヒープ/スタックメモリ安全性検出 | LLVM / GCC プロジェクト | Apache 2.0 / GPL | 全 ARCH(試験時、`asan` プリセット) |
| SOUP-014 | UndefinedBehaviorSanitizer ランタイム | clang/gcc 同梱 | 未定義動作(整数オーバフロー、null 参照等)検出 | 同上 | 同上 | 全 ARCH(試験時、`ubsan` プリセット)。**HZ-003 整数オーバフロー類型の機械的検出** |
| SOUP-015 | **ThreadSanitizer ランタイム** | clang/gcc 同梱 | **データ競合、ロック順序逆転、スレッドリーク検出** | 同上 | 同上 | **全 ARCH(試験時、`tsan` プリセット)。HZ-002 race condition 直接対応(Tyler 第 1 例の構造的予防の機械検証手段)** |

## 7. SOUP の機能的及び性能的要求事項の指定(箇条 5.3.3 ― クラス B, C)

各 SOUP に対する機能・性能要求を、本 SAD 上の利用文脈(関連 ARCH / 関連 SRS)に基づき設定する。本要求は SDD(Step 16)で実装方針として参照され、結合試験(IT)・システム試験(ST)で検証する。

| SOUP ID | 機能要求(本プロジェクトでの利用に必要な機能) | 性能要求(スループット・応答・精度等) | 検証方法(後続 IT/ST で実施) |
|---------|---------------------------------------|---------------------------------|--------------------------|
| SOUP-001 / SOUP-002 | C++20 標準準拠(`enum class`, `std::strong_ordering`, `concepts`, `std::span`, `std::expected`(C++23 候補は禁止、C++20 範囲のみ)、`std::atomic` 完全実装、`std::thread`/`std::mutex`/`std::condition_variable` 安定動作)。**TSan/ASan/UBSan サポート**(SOUP-013/014/015 の前提)。`-Wall -Wextra -Wpedantic -Werror` でビルド可能 | コンパイル時間 < 5 分(Inc.1 規模、smoke build)、最適化 -O2 で実行時オーバヘッド < 5%(Sanitizer なし) | CI cpp-build マトリクスで両コンパイラ × 4 プリセットの全 8 ジョブが Pass |
| SOUP-003 / SOUP-004 | C++20 標準ライブラリ完全実装。**`std::atomic<uint64_t>` の lock-free 動作**(SRS-009 / RCM-003 の前提)。`std::mutex` の優先度継承サポート(オプション、SDD で必要性確定) | 共有なし(各プロセス独立)、ロック取得 < 100 ns(uncontended)、メモリオーダリング(acquire/release)が CPU メモリモデル(x86_64 TSO / arm64 weak)で正しく動作 | UT で `std::atomic_uint64_t::is_lock_free() == true` を表明、IT で TSan 全 Pass |
| SOUP-005 | CMakePresets v6 サポート、vcpkg toolchain 統合(`CMAKE_TOOLCHAIN_FILE` 1 行)、`target_compile_features(... cxx_std_20)` で C++20 強制、`add_test()` + `gtest_discover_tests()` で GoogleTest 統合 | configure 時間 < 30 秒(Inc.1 規模、依存解決 1 回後) | CI cpp-build の configure ステップで Pass |
| SOUP-006 | CMake からの `cmake --build --preset` で並列ビルド、依存トラッキング正確 | 並列 4 ジョブで Inc.1 規模を < 60 秒でビルド | CI cpp-build の build ステップで Pass |
| SOUP-007 | manifest mode で `vcpkg.json` の依存解決、`builtin-baseline` で commit 完全固定、CMake から `vcpkg-installed/` の自動参照 | 初回依存解決 < 10 分、キャッシュヒット時 < 30 秒 | CI cpp-build の vcpkg bootstrap ステップで Pass |
| SOUP-008 / SOUP-009 | `TEST(Suite, Name)` / `TEST_F(Fixture, Name)` / `TEST_P(Param, Name)` のサポート、`EXPECT_*` / `ASSERT_*` マクロ群、`gmock` の `EXPECT_CALL` の正確な検証、デスマッチサポート(`EXPECT_DEATH`) | 試験 1 件 < 100 ms(Inc.1 範囲)、全試験スイート < 60 秒(Inc.1 範囲) | CI cpp-build の ctest ステップで Pass |
| SOUP-010 | `bugprone-*` / `cert-*` / `concurrency-*` / `cppcoreguidelines-*` / `hicpp-*` / `misc-*` / `modernize-*` / `performance-*` / `portability-*` / `readability-*` チェック群を有効化、`compile_commands.json` 生成済プロジェクトで実行可能 | Inc.1 規模で実行時間 < 5 分 | CI clang-tidy ジョブで Pass(警告 0、`-Werror` 設定で失敗時 fail) |
| SOUP-013 | ヒープバッファオーバフロー / use-after-free / double free / メモリリーク検出。`ASAN_OPTIONS=halt_on_error=1` で初回検出時に即停止 | 実行時オーバヘッド ≦ 3x、メモリオーバヘッド ≦ 3x | CI cpp-build asan プリセットで Pass(0 検出) |
| SOUP-014 | 符号付き整数オーバフロー / 0 除算 / null 参照 / ミスアラインメモリアクセス / 配列範囲外検出。`UBSAN_OPTIONS=halt_on_error=1` で初回検出時に即停止。**HZ-003(整数オーバフロー)類型の機械的検出** | 実行時オーバヘッド ≦ 1.5x | CI cpp-build ubsan プリセットで Pass(0 検出) |
| **SOUP-015** | **データ競合(race condition)、ロック順序逆転、スレッドリーク検出。`TSAN_OPTIONS=halt_on_error=1, second_deadlock_stack=1` で初回検出時に即停止。HZ-002 race condition 類型(Tyler 第 1 例)の機械的検出** | 実行時オーバヘッド ≦ 15x、メモリオーバヘッド ≦ 8x | **CI cpp-build tsan プリセットで Pass(0 検出)。Inc.3 で完成する RCM-002/004/019 の 3 層検証(SRMP §6.1)の機械検証手段** |

## 8. SOUP に必要なシステム上のハードウェア及びソフトウェアの指定(箇条 5.3.4 ― クラス B, C)

各 SOUP が正しく動作するために必要な、システムレベルの HW/SW 要求を明示する。**SCMP §4.1 重大区分判定で「コンパイラ・標準ライブラリ更新は重大」と規定されている根拠の構造的記録**(HZ-007 直接対応)。

| SOUP ID | 必要 HW(CPU/RAM/タイマ等) | 必要 SW(OS/ミドルウェア等) | 根拠 / 関連 SRS / 関連 HZ |
|---------|--------------------------|-------------------------|----------------------|
| SOUP-001 / SOUP-002 | x86_64 または arm64、RAM ≧ 4 GB(ビルド時)、ストレージ ≧ 10 GB(toolchain + vcpkg cache) | Linux(Ubuntu 22.04+ または同等 glibc 2.35+)/ macOS 13+ / Windows は本書スコープ外 | SRS-§4.8 設置要求と整合。HZ-007 関連: コンパイラバージョン更新時は SCMP §4.1 重大区分として CCB 審議 + 全試験再実行 |
| SOUP-003 / SOUP-004 | SOUP-001/002 と同じ | SOUP-001/002 同梱 | `std::atomic<uint64_t>::is_lock_free() == true` を保証する CPU(x86_64 / arm64v8+)。HZ-002 関連: 標準ライブラリ更新時は HZ-007 リスク評価必須 |
| SOUP-005 | CPU 任意、RAM ≧ 1 GB | Python 3.10+(CMake 内部スクリプト用) | CMake の find_package、generator 機能はスクリプト依存 |
| SOUP-006 | CPU 任意、RAM ≧ 512 MB | OS 標準(POSIX または Win32) | 単体実行可能、追加ランタイム不要 |
| SOUP-007 | RAM ≧ 4 GB(依存ビルド時に大型ライブラリのコンパイル要)、ストレージ ≧ 5 GB | Git 2.30+、curl 7.80+、Python 3.10+(bootstrap 用)、bash(POSIX)/ PowerShell(Windows 非対応) | builtin-baseline の commit pinning が機能する Git バージョン |
| SOUP-008 / SOUP-009 | SOUP-001/002 と同じ | SOUP-001/002/003/004 必須(C++ 標準ライブラリ依存) | UT 実行時にプロセス内に静的リンク、ランタイム外部依存なし |
| SOUP-010 | RAM ≧ 4 GB(LLVM AST 解析時) | SOUP-001 と同じ(Clang フロントエンドを共有) | `compile_commands.json` 生成のため CMake 必須 |
| SOUP-013 / SOUP-014 | RAM ≧ 元プロセス × 3 倍(シャドウメモリ用) | SOUP-001/002 同梱、Linux カーネル ≧ 5.0(MMU 機能依存) | 本番ビルドでは無効化(性能要件に影響)。試験ビルドのみ |
| **SOUP-015** | **RAM ≧ 元プロセス × 8 倍、CPU 命令キャッシュ ≧ 32 KB(性能オーバヘッド許容)。x86_64 / arm64v8+(下位アーキは未サポート)** | **SOUP-001/002 同梱(clang ≧ 11、gcc ≧ 11)、Linux カーネル ≧ 4.4 / macOS 12+** | **HZ-002 直接対応の機械検証手段。`tsan` プリセットでのみ有効化、本番ビルドでは無効化** |

> **HZ-007 リスク評価の継続適用:** 上記 SOUP-001〜015 のいずれかが更新された場合、SCMP §4.1 重大区分として CCB 審議 + 全試験再実行 + HZ-007 リスク評価記録が必須(SCMP §4.3)。本 SAD は SOUP の **現バージョン断面** を記録するものであり、更新時は本書の改訂と SAD バージョン昇格を伴う。

## 9. リスクコントロール手段のためのソフトウェア項目の分離(箇条 5.3.5 ― クラス C)

### 9.1 分離設計の方針

**本 SAD の核心セクション。** Therac-25 race condition(Tyler 第 1 例、Leveson & Turner 1993 §IV.B)の根本構造である「UI と制御の同一プロセス・共有メモリ動作」を、本プロジェクトでは **アーキテクチャ時点で構造的に不可能化** する。具体的には以下 3 種の分離を確立する。

1. **SEP-001:** Operator UI Process と Safety Core Process の **OS プロセス分離**(別 PID、別仮想アドレス空間、別ファイル記述子テーブル)
2. **SEP-002:** Hardware Simulator Process と Safety Core Process の OS プロセス分離(故障注入の容易性 + 試験時の独立制御性)
3. **SEP-003:** プロセス内の全タスク間通信を **lock-free queue によるメッセージパッシング** に限定し、共有変数を **設計上禁止**(clang-tidy `concurrency-*` で機械的検出、SRS-D-009)

### 9.2 分離手段の詳細

| 分離 ID | 対象項目 | 分離手段 | 実装方法 | 関連 RCM / HZ | 検証方法(後続 IT/ST で実施) |
|--------|---------|---------|---------|-------------|--------------------------|
| **SEP-001** | ARCH-001(Operator UI Process)/ ARCH-002(Safety Core Process) | **OS プロセス分離** | (a) 起動時に別 `fork()` + `execve()` で別プロセスとして起動 (b) 通信は IF-E-001(gRPC over Unix Domain Socket、Protocol Buffers スキーマで型強制)に限定 (c) **共有メモリ(POSIX `shm_open`、System V `shmget`)を一切使用しない**、(d) 推奨: 異なる UNIX ユーザ ID で起動(本 v0.1 では推奨に留める、Inc.4 で確定) | **RCM-018 / SRS-RCM-018 / HZ-002 / HZ-007 直接対応** | (1) `ps -ef` で別 PID 確認、(2) `/proc/<pid>/maps` で共有メモリ領域がないことを確認、(3) `strace -f` で通信が UDS 経由のみであることを確認、(4) UI プロセス強制終了(SIGKILL)で安全コアが「UI 切断」として安全側に縮退することを確認 |
| **SEP-002** | ARCH-002(Safety Core Process)/ ARCH-003(Hardware Simulator Process) | **OS プロセス分離**(SEP-001 と同様の手段) | 同 SEP-001 の (a)〜(c)。ARCH-003 は試験中の故障注入(センサ故障値、応答遅延、無応答)を組込み | RCM-018 関連(構造的安全余裕)、HW シミュレータと制御の独立性確保 | SEP-001 と同様の検証 + (5) Hardware Simulator 強制終了で安全コアが「HW 通信障害」として fail-safe(ビームオフ + アラーム)に遷移することを確認 |
| **SEP-003** | ARCH-002 内の全タスク(SafetyCoreOrchestrator、SafetyMonitor、AuditLogger、各 Manager) | **lock-free queue によるメッセージパッシング**、共有変数禁止 | (a) ARCH-004 MessageBus を **唯一のタスク間通信手段** とする (b) `std::atomic` 以外の共有変数(`static`、グローバル変数、クラスのメンバ変数を複数スレッドから参照)を **設計上禁止** (c) clang-tidy `concurrency-mt-unsafe` `cppcoreguidelines-avoid-non-const-global-variables` `cert-pos35-c` 等で機械的検出 (d) コードレビューで全 PR の共有変数アクセスを確認 | **RCM-019 / SRS-RCM-019 / SRS-D-009 / HZ-002 / HZ-003 直接対応** | (1) clang-tidy 警告 0 を CI で強制、(2) **TSan(SOUP-015)を CI で常時実行し race condition 検出 0 を確認**、(3) UT で MessageBus の lock-free 性(`is_lock_free() == true`)を表明、(4) MessageBus の queue 容量超過時の挙動(blocking / dropping / failure)を境界値試験で確認 |

### 9.3 分離が成立しない場合の扱い

IEC 62304 §5.3.6 に従い、分離が確立されていない項目は同一(最高)安全クラス C として扱う。

- 本 SAD では **SEP-001〜SEP-003 の確立が成立しない場合、ARCH-001 / ARCH-003 はクラス C のままとする**(分離による低クラス化 B は適用不可)。
- SAD 検証(§10)で SEP-001〜SEP-003 のいずれかに不備が見つかった場合、CR を起票して SAD を改訂する。

### 9.4 Therac-25 教訓との対応

| Therac-25 事故主要因(SPRP §4.3.1 類型) | 本 SAD での構造的対応 |
|-----------------------------------|--------------------|
| **A: 並行処理(race condition)** — Tyler 第 1 例で UI と制御が同一プロセス・共有メモリで動作し、8 秒以内編集で race condition 発現 | **SEP-001(プロセス分離)+ SEP-003(共有変数禁止)で構造的に不可能化**。MessageBus は確定済データのみ送受信、競合の発生空間を消去 |
| **B: 整数オーバフロー** — Yakima Class3 変数の周期的ゼロ値が安全チェックバイパス | DoseManager の `std::atomic<uint64_t>` 採用(RCM-003)+ UBSan(SOUP-014)で機械的検出。SAD 上は ARCH-002.4 DoseManager に集中させ、共有変数化を SEP-003 で禁止 |
| **C: 共有状態の競合的アクセス** | **SEP-003 で共有変数を全廃**、メッセージパッシングのみ |
| **D: インターロック欠落** | ARCH-002.7 SafetyMonitor を独立タスクとして配置、IF-U-006 で SafetyCoreOrchestrator と疎結合(Inc.2 で本格化、RCM-006/007) |
| **E: バイパス UI** | SEP-001 で UI 層を別プロセス化、UI 内のバグや誤操作が安全コアの判断を直接バイパスできない構造を確立(Inc.4 で完成、RCM-009/010) |
| **F: レガシーコード再利用前提喪失** | SOUP §7/§8 で各 SOUP の HW/SW 要求を明示、コンパイラ・標準ライブラリ更新は SCMP §4.1 重大区分(RCM-012、HZ-007 直接対応) |

## 10. ソフトウェアアーキテクチャの検証(箇条 5.3.6)

### 10.1 検証チェックリスト

本アーキテクチャは以下を満たすことを設計レビュー(本書セルフレビュー、CCB §5.4 1 分インターバル後)で確認する。

- [x] **SRS のすべての要求事項を実装可能である**: §11.1 トレーサビリティマトリクスで全 SRS 要求(58 項目以上)に対し ARCH-NNN を割付け
- [x] **ソフトウェア項目間・外部システムとのインタフェースが一貫している**: §5.1 IF-U(10 件)+ §5.2 IF-E(4 件)で内部・外部 IF を網羅、メッセージ型は SDD §5.4.3 で確定予定として整合
- [x] **医療機器のリスクコントロール手段の実装を支援する**: §11.1 で全 RCM(RCM-001〜019 のうち Inc.1 範囲分)を ARCH-NNN に割付け、構造的 RCM(RCM-018/019)を §9 SEP-001/SEP-003 として確立
- [x] **安全クラスに応じた分離が適切に設計されている**: §9 SEP-001/002/003 でクラス C 必須(IEC 62304 §5.3.5)を満たす
- [x] **SOUP の仕様(機能・性能要求)が記述されている**: §7 で 13 件の機能・性能要求、§8 で HW/SW 要求を明示(クラス B/C 必須、IEC 62304 §5.3.3 / §5.3.4)
- [x] **Therac-25 事故主要因類型(SPRP §4.3.1 A〜F)に対応する構造的対応が示されている**: §9.4 で対応表を明示
- [x] **後続フェーズ(SDD/実装/試験)で検証可能な粒度に分解されている**: ARCH-NNN は項目レベル、ARCH-NNN.x はサブ項目レベル、ユニットレベルは SDD §5.4.2 で展開
- [x] **新規ハザードが識別された場合は RMF を更新する**: 本 SAD 作成中に **新規ハザードは識別されなかった**(§10.3 参照)

### 10.2 レビュー記録

| 項目 | 結果 | レビュー日 | 記録 ID |
|------|------|----------|---------|
| 本 SAD v0.1 全体(セルフレビュー) | 承認(全 §1〜§11 整合確認、§9 SEP-001〜003 が SRS-RCM-018/019 と整合、§7/§8 SOUP 記述が CIL §5 13 件と整合、§11 トレーサビリティが SRS 全要求をカバー) | 2026-04-26 | 本書 v0.1 のコミット SHA(本ステップコミット時に追記) |

### 10.3 SAD 作成中に識別したリスク再評価

SAD 作成中に **新規ハザードは識別されなかった**。RMF-TH25-001 v0.1 への追加は不要。

ただし、以下の点は SDD/実装着手時に再評価する:

- **IF-E-001 / IF-E-002(gRPC over UDS)** の具体実装で、IPC 障害(プロセス強制終了、メッセージ欠落、デシリアライズ失敗)に伴う新規 sub-hazard が生じる可能性 → SDD §5.4.3 で IPC エラー処理ロジックを設計、Inc.1 IT で fail-safe 確認
- **MessageBus の queue 容量超過時の挙動**(blocking / dropping / failure)が安全側 / 危険側のいずれに振れるかを SDD で確定。本 SAD では「fail-stop(プロセス停止)を第一候補」とし、SDD で技術的に困難な場合は CR で改訂

### 10.4 残された未確定事項(SDD で確定)

| 未確定事項 | 確定先 | 暫定方針 |
|---------|------|---------|
| MessageBus の具体実装(SPSC / MPSC、`folly::ProducerConsumerQueue` 等の選定) | SDD §5.4.2 | 候補: `folly::ProducerConsumerQueue`(SPSC、Inc.1 範囲では十分)。MPSC 必要時は `boost::lockfree::queue` |
| IF-E-001 / IF-E-002 の Protocol Buffers スキーマ | SDD §5.4.3 | gRPC over UDS、`PrescriptionSet` / `BeamCommand` / `StateNotification` / `AlarmNotification` / `ElectronGunCommand` / `MagnetCommand` / `TurntableCommand` / `SensorReadResponse` を主要 RPC とする |
| `LifecycleState` 状態機械の状態数・遷移表 | SDD §5.4.2(SafetyCoreOrchestrator) | 候補: `Init / SelfCheck / Idle / PrescriptionSet / Ready / BeamOn / Halted / Error` の 8 状態 |
| `enum class ErrorCode` の階層・コード体系 | SDD §5.4.2 | 候補: `Mode系 / Beam系 / Dose系 / Turntable系 / Magnet系 / IPC系 / Auth系 / Internal系` の 8 系統で階層化 |
| Inc.4 で確定予定の UI フレームワーク | SDD(Inc.4 改訂時) | 候補: ImGui + GLFW / curses ベース TUI |

## 11. トレーサビリティマトリクス

### 11.1 SRS ↔ ARCH ↔ RCM ↔ HZ 対応

> **凡例:** 列「Inc 確定範囲」は本 SAD v0.1 で当該 ARCH 要素の **項目レベル** 確定範囲(Inc.1 = 中核確定、Inc.2/3/4 = 構造的前提のみ確定、詳細は後続 Inc 改訂)。

| SRS ID | 要求概要 | ARCH 項目 | 関連 RCM | 関連 HZ | Inc 確定範囲 |
|--------|--------|---------|---------|--------|------------|
| SRS-001 | TreatmentMode `enum class` 型安全管理 | ARCH-002.2 | RCM-001 | HZ-001 | Inc.1 |
| SRS-002 | モード遷移シーケンス(6 ステップ) | ARCH-002.2 + ARCH-002.5 + ARCH-002.6 | RCM-001 / RCM-005 | HZ-001 | Inc.1 |
| SRS-003 | ビームオン要求時のモード/位置整合再確認 | ARCH-002.2 + ARCH-002.5 + ARCH-002.3 | RCM-001 / RCM-005 | HZ-001 | Inc.1 |
| SRS-004 | エネルギー設定値の範囲管理 | ARCH-002.2 | RCM-008 | HZ-005 | Inc.1 |
| SRS-005 | ベンディング磁石電流(強い型 + エネルギー対応表) | ARCH-002.6 | RCM-008 | HZ-005 | Inc.1 |
| SRS-006 | ターンテーブル位置(強い型 + 偏差判定) | ARCH-002.5 | RCM-001 / RCM-005 | HZ-001 | Inc.1 |
| SRS-007 | ビームオン指令(許可フラグ確認) | ARCH-002.3 + ARCH-002.2 | RCM-017 | HZ-001 | Inc.1 |
| SRS-008 | 目標ドーズ値(強い型 + 範囲) | ARCH-002.4 | RCM-008 | HZ-005 | Inc.1 |
| SRS-009 | ドーズ積算カウンタ `std::atomic<uint64_t>` | ARCH-002.4 | RCM-003 / RCM-008 | HZ-003 / HZ-005 | Inc.1 |
| SRS-010 | ドーズ目標到達検知 → 即時ビームオフ(< 1 ms) | ARCH-002.4 + ARCH-002.3 | RCM-008 | HZ-005 | Inc.1 |
| SRS-011 | ビームオフ → 電子銃停止 < 10 ms | ARCH-002.3 | RCM-017 | HZ-001 / HZ-004 | Inc.1 |
| SRS-012 | 起動時自己診断(4 項目) | ARCH-002.8 + ARCH-002.1 | RCM-013 | HZ-009 | Inc.1 |
| SRS-I-001 | モード選択コマンド入力 | ARCH-001.1(IF) + IF-E-001 | RCM-018 | HZ-002 | Inc.1(IF)/ Inc.4(UI 内部) |
| SRS-I-002 | エネルギー設定値入力 | ARCH-001.1(IF) + IF-E-001 | RCM-018 | HZ-002 | 同上 |
| SRS-I-003 | 目標ドーズ値入力 | ARCH-001.1(IF) + IF-E-001 | RCM-018 | HZ-002 | 同上 |
| SRS-I-004 | ビームオン/オフ指令入力 | ARCH-001.2(IF) + IF-E-001 | RCM-018 | HZ-002 | 同上 |
| SRS-I-005 | ターンテーブル位置センサ × 3 | ARCH-003.3 + IF-E-002 + ARCH-002.5 | RCM-005 | HZ-001 / HZ-003 | Inc.1 |
| SRS-I-006 | ドーズモニタ × 2 | ARCH-003.4 + IF-E-002 + ARCH-002.4 | RCM-007 | HZ-008 | Inc.1(IF)/ Inc.2(比較ロジック) |
| SRS-I-007 | ベンディング磁石電流計測 | ARCH-003.2 + IF-E-002 + ARCH-002.6 | — | — | Inc.1 |
| SRS-I-008 | 電子銃ビーム電流計測 | ARCH-003.1 + IF-E-002 + ARCH-002.3 | RCM-017 | HZ-001 | Inc.1 |
| SRS-O-001 | 電子銃ビーム電流指令 | ARCH-002.3 + IF-E-002 + ARCH-003.1 | RCM-017 | HZ-001 | Inc.1 |
| SRS-O-002 | ベンディング磁石電流指令 | ARCH-002.6 + IF-E-002 + ARCH-003.2 | — | — | Inc.1 |
| SRS-O-003 | ターンテーブル位置指令 | ARCH-002.5 + IF-E-002 + ARCH-003.3 | RCM-001 | HZ-001 | Inc.1 |
| SRS-O-004 | 状態表示・エラーメッセージ | ARCH-001.3 + IF-E-001 | RCM-009 | HZ-006 | Inc.1(IF)/ Inc.4(UI 内部) |
| SRS-O-005 | 操作・エラーログ(append-only) | ARCH-002.9 + IF-E-003 | RCM-011 | HZ-006 | Inc.1(空殻)/ Inc.4(完成) |
| SRS-IF-001 | 仮想電子銃 IF | ARCH-003.1 | — | — | Inc.1 |
| SRS-IF-002 | 仮想ベンディング磁石 IF | ARCH-003.2 | — | — | Inc.1 |
| SRS-IF-003 | 仮想ターンテーブル IF(3 系統) | ARCH-003.3 | RCM-005 | HZ-001 | Inc.1 |
| SRS-IF-004 | 仮想イオンチェンバ IF(2 組) | ARCH-003.4 | RCM-007 | HZ-008 | Inc.1 |
| SRS-IF-005 | 操作者 UI ↔ 安全コア IF(プロセス間、共有変数禁止) | **IF-E-001 + SEP-001 + SEP-003 + ARCH-004.2** | **RCM-018 / RCM-019** | **HZ-002 / HZ-007** | **Inc.1**(構造的確定)/ Inc.4(完成) |
| SRS-ALM-001 | モード/位置不整合検知 | ARCH-002.7 + ARCH-002.5 + ARCH-002.2 | RCM-001 / RCM-005 | HZ-001 | Inc.1(検知ロジック)/ Inc.2(SafetyMonitor 完成) |
| SRS-ALM-002 | ドーズ目標超過 | ARCH-002.7 + ARCH-002.4 | RCM-008 | HZ-005 | 同上 |
| SRS-ALM-003 | ターンテーブル位置センサ 3 系統不一致 | ARCH-002.7 + ARCH-002.5 | RCM-005 | HZ-001 / HZ-003 | 同上 |
| SRS-ALM-004 | ベンディング磁石電流偏差 | ARCH-002.7 + ARCH-002.6 | — | — | Inc.2 |
| SRS-ALM-005 | 起動時自己診断失敗 | ARCH-002.8 + ARCH-002.7 + ARCH-001.3 | RCM-013 | HZ-009 | Inc.1(StartupSelfCheck)/ Inc.4(UI 表示) |
| SRS-SEC-001 | 操作者認証(初版 IF) | ARCH-002.10 + ARCH-001.4 + IF-U-010 + IF-E-001 | RCM-015 | HZ-010 | Inc.1(IF 空殻)/ Inc.4(完成) |
| SRS-SEC-002 | 治療パラメータ完全性(初版 IF) | ARCH-002.10 + IF-E-001 | RCM-016 | HZ-010 | 同上 |
| SRS-UX-001 | エラーメッセージ構造化(現象+原因+対処) | ARCH-001.3 + ARCH-002 全体(`enum class ErrorCode`)| RCM-009 | HZ-006 | Inc.1(`enum class ErrorCode` 空殻)/ Inc.4(完成) |
| SRS-UX-002 | 致死的エラーのバイパス禁止 | ARCH-001.2 + ARCH-002.1(`LifecycleState` `Halted`)| RCM-010 | HZ-006 | 同上 |
| SRS-UX-003 | 治療開始前の処方確認画面 | ARCH-001.1 + ARCH-002.10 | RCM-009 | HZ-010 | Inc.4 |
| SRS-D-001 | TreatmentMode `enum class` | ARCH-002.2 | RCM-001 | HZ-001 | Inc.1 |
| SRS-D-002〜D-008 | 強い型(Energy_MeV / DoseUnit_cGy / PulseCount / MagnetCurrent_A / Position_mm / ElectronGunCurrent_mA) | ARCH-002 全体(共通型ライブラリ、SDD §5.4.2 で確定) | RCM-008 | HZ-005 | Inc.1 |
| **SRS-D-009** | **共有変数の `std::atomic`/`std::mutex` 必須(コーディング規約)** | **§9 SEP-003 + clang-tidy ルール群** | **RCM-002 / RCM-004** | **HZ-002** | **Inc.1**(構造的確定) |
| SRS-D-010 | 操作・エラーログ(構造化、append-only) | ARCH-002.9 + IF-E-003 | RCM-011 | HZ-006 | Inc.1(空殻)/ Inc.4(完成) |
| SRS-REG-001〜004 | 規制要求(IEC 60601-2-1/62366-1/60601-1-8/81001-5-1) | (本 SAD 全体で派生 SRS-UX/SEC/ALM 経由) | — | — | Inc.1〜4 |
| **SRS-RCM-001** | **RCM-001 モード遷移整合性チェック** | **ARCH-002.2** | **RCM-001** | **HZ-001** | **Inc.1** |
| **SRS-RCM-005** | **RCM-005 ターンテーブル in-position 冗長確認** | **ARCH-002.5 + ARCH-003.3** | **RCM-005** | **HZ-001 / HZ-003** | **Inc.1**(3 系統センサ部分)/ Inc.2(完成) |
| **SRS-RCM-008** | **RCM-008 ドーズ計算境界値・型による単位安全** | **ARCH-002.2 + ARCH-002.4 + ARCH-002.6 + 共通型ライブラリ** | **RCM-008** | **HZ-005** | **Inc.1** |
| **SRS-RCM-017** | **RCM-017 ビームオン → ビームオフ即時遷移保証** | **ARCH-002.3** | **RCM-017** | **HZ-001 / HZ-004** | **Inc.1** |
| **SRS-RCM-018** | **RCM-018 UI 層と安全コア分離(IEC 62304 §5.3.5)** | **§9 SEP-001 + IF-E-001 + ARCH-001 / ARCH-002 プロセス分離** | **RCM-018** | **HZ-002 / HZ-007** | **Inc.1**(構造的確定) |
| **SRS-RCM-019** | **RCM-019 タスク間メッセージパッシング(共有変数排除)** | **§9 SEP-003 + ARCH-004 MessageBus + IF-U-001〜010 全体** | **RCM-019** | **HZ-002 / HZ-003** | **Inc.1**(構造的確定) |
| SRS-101 | ビームオフ → 電子銃停止 < 10 ms | ARCH-002.3 + IF-E-002 | RCM-017 | HZ-001 / HZ-004 | Inc.1(IT で測定) |
| SRS-102 | モード変更 → 完了通知 < 500 ms | ARCH-002.2 + ARCH-002.5 + ARCH-002.6 | — | — | Inc.1(IT で測定) |
| SRS-103 | ドーズモニタ更新レート ≧ 1 kHz | ARCH-003.4 + IF-E-002 + ARCH-002.4 | — | — | Inc.1(IT で測定) |
| SRS-104 | ターンテーブル位置センササンプリング ≧ 100 Hz | ARCH-003.3 + IF-E-002 + ARCH-002.5 | — | — | Inc.1(IT で測定) |

### 11.2 SOUP ↔ ARCH 割付(本 SAD §7 / §8 で確定)

| SOUP ID | 関連 ARCH | 関連 SRS | 関連 HZ |
|---------|---------|---------|--------|
| SOUP-001 / 002 | 全 ARCH(ビルド時) | (全 SRS、ビルド前提) | HZ-007 |
| SOUP-003 / 004 | 全 ARCH(ランタイム) | SRS-009 / SRS-D-009 | HZ-002 / HZ-003 / HZ-007 |
| SOUP-005 / 006 / 007 | 全 ARCH(ビルド時) | (全 SRS、ビルド前提) | HZ-007 |
| SOUP-008 / 009 | tests/ 配下、全 ARCH の UT | (全 SRS、UT 検証) | — |
| SOUP-010 | 全 ARCH(ビルド時の機械検出) | SRS-D-009 / SRS-RCM-019 | HZ-002 |
| SOUP-013 | 全 ARCH(asan プリセット) | (メモリ安全性、全 SRS 派生) | — |
| SOUP-014 | 全 ARCH(ubsan プリセット) | SRS-009 / SRS-D-002〜008 | **HZ-003** |
| **SOUP-015** | **全 ARCH(tsan プリセット)** | **SRS-D-009 / SRS-RCM-019** | **HZ-002**(直接対応) |

### 11.3 SEP ↔ HZ ↔ RCM 対応(分離設計のトレース)

| 分離 ID | 対応 HZ | 対応 RCM | 対応 SRS-RCM | 対応 Therac-25 主要因類型 |
|--------|--------|---------|-------------|----------------------|
| **SEP-001** | **HZ-002 / HZ-007** | **RCM-018** | **SRS-RCM-018** | **A**(race condition、Tyler 第 1 例)、**E**(バイパス UI、East Texas) |
| **SEP-002** | (構造的安全余裕、特定 HZ 対応なし) | (RCM-018 関連の余裕) | — | — |
| **SEP-003** | **HZ-002 / HZ-003** | **RCM-002 / RCM-004 / RCM-019** | **SRS-RCM-019 / SRS-D-009** | **A**(race condition)、**B**(整数オーバフロー)、**C**(共有状態競合) |

## 12. 改訂履歴

| バージョン | 日付 | 変更内容 | 変更者 |
|----------|------|---------|--------|
| 0.1 | 2026-04-26 | 初版作成(Step 15 / CR-0003、本プロジェクト Issue #3)。**Inc.1 範囲(ビーム生成・モード制御コア)** のアーキテクチャを項目レベルで確定。**§4 アーキテクチャ概要:** マルチプロセス + イベントループ + 独立監視タスクの 3 層方針、ARCH-001(Operator UI Process、Inc.4)/ ARCH-002(Safety Core Process、Inc.1 中核、サブ項目 10 件)/ ARCH-003(Hardware Simulator Process、Inc.1)/ ARCH-004(MessageBus、Inc.1 中核)を確定。**§5 IF:** 内部 IF 10 件(IF-U-001〜010、全て MessageBus 経由メッセージパッシング)+ 外部 IF 4 件(IF-E-001〜004、gRPC over UDS + ファイル IO)を定義。**§7 / §8 SOUP(IEC 62304 §5.3.3 / §5.3.4 クラス B/C 必須):** CIL §5 13 件全 SOUP の機能・性能要求 + HW/SW 要求を SRS への割付として記述。**§9 分離設計(IEC 62304 §5.3.5 クラス C 必須、本 SAD の核心):** SEP-001(UI 層と安全コアの OS プロセス分離、RCM-018 直接対応、HZ-002/007)/ SEP-002(Hardware Simulator と安全コアの分離)/ SEP-003(共有変数禁止 + lock-free queue メッセージパッシング、RCM-019/SRS-D-009 直接対応、HZ-002/003)を確定。**§9.4 で Therac-25 事故主要因類型 A〜F への対応表を明示**。**§11 トレーサビリティ:** SRS 全 58 件以上の要求事項を ARCH-NNN に割付け、Inc 確定範囲(Inc.1 / Inc.2 / Inc.3 / Inc.4)を明示。**Therac-25 学習目的の中核:** Tyler 第 1 例 race condition の根本構造(UI と制御の同一プロセス・共有メモリ動作)を、本 SAD で SEP-001(プロセス分離)+ SEP-003(共有変数禁止)として **アーキテクチャ時点で構造的に不可能化**。HZ-007(レガシー前提喪失)は §8 SOUP HW/SW 要求の明文化で SCMP §4.1 重大区分との連携を確立 | k-abe |
