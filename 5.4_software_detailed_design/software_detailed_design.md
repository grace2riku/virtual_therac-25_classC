# ソフトウェア詳細設計書(SDD)

**ドキュメント ID:** SDD-TH25-001
**バージョン:** 0.1
**作成日:** 2026-04-26
**対象製品:** 仮想 Therac-25(Virtual Therac-25) / TH25-SIM-001
**対象ソフトウェアバージョン:** 0.2.0(Inc.1 完了で初回機能リリース予定)
**安全クラス:** C(IEC 62304)
**変更要求:** CR-0004(GitHub Issue #4)

| 役割 | 氏名 | 所属 | 日付 | 署名 |
|------|------|------|------|------|
| 作成者 | k-abe | — | 2026-04-26 | |
| レビュー者 | k-abe(セルフ、CCB §5.4 1 分インターバル後) | — | 2026-04-26 | |
| 承認者 | k-abe(セルフ承認、CCB §3 単独開発兼任) | — | 2026-04-26 | |

> **本プロジェクトの位置づけ(注記)**
>
> 本ドキュメントは IEC 62304 に基づく医療機器ソフトウェア開発プロセスの学習・参考実装を目的とした **仮想プロジェクト** の成果物である。題材は 1985-1987 年に放射線過剰照射事故を起こした実在の医療機器 **Therac-25**(AECL)である。本書の記述は、当時の AECL 社内文書ではなく、公開された事故報告書・学術文献(Leveson & Turner 1993 ほか)に基づく **学習用の再構成** である。
>
> **本 v0.1 は Inc.1(ビーム生成・モード制御コア)範囲の詳細設計を中心に確定する初版**である。ARCH-001(Operator UI Process、Inc.4 で本格化)、ARCH-002.7 SafetyMonitor(Inc.2 で本格化)、ARCH-002.9 AuditLogger / ARCH-002.10 CoreAuthenticationGateway(Inc.4 で本格化)に該当する UNIT は、本 v0.1 では **空殻ユニット**(構造的前提を確立する最小プレースホルダ)として記述する。詳細は後続 Inc の SDD 改訂(v0.2/v0.3/v0.4)で展開する。
>
> **本書の核心(クラス C 必須要件):** IEC 62304 §5.4.2(各ソフトウェアユニットの詳細設計、クラス C 必須)/ §5.4.3(インタフェースの詳細設計、クラス C 必須)/ §5.4.4(詳細設計の検証、クラス C 必須)を Inc.1 範囲で実体化し、本 SDD の完了をもって `inc1-design-frozen` タグ(ベースライン ID `BL-20260426-002`)を付与し、Inc.1 設計を不可逆に凍結する。Step 17+ のコード実装は本 SDD を **唯一の根拠** として進める。

---

## 1. 目的と適用範囲

### 1.1 目的

本書は、SAD-TH25-001 v0.1 で定義された Inc.1 範囲のソフトウェア項目(ARCH-NNN)を IEC 62304 箇条 5.4 に従ってソフトウェアユニット(UNIT-NNN)に分解し、各ユニットの詳細設計(関数シグネチャ・データ構造・状態遷移・タイミング制約・例外処理)、ユニット間 IF の詳細(メッセージ型・スキーマ)、外部 IF の詳細(Protocol Buffers スキーマ)を確定する。

具体的には以下を成立させる:

- **箇条 5.4.1**: ソフトウェア項目をソフトウェアユニットに改良(本書 §3、ARCH-NNN.x → UNIT-NNN)
- **箇条 5.4.2**(クラス C 必須): 各ソフトウェアユニットの詳細設計(本書 §4、UNIT 単位の詳細設計)
- **箇条 5.4.3**(クラス C 必須): ソフトウェアユニット間および外部のインタフェースの詳細設計(本書 §5)
- **箇条 5.4.4**(クラス C 必須): 詳細設計の検証(本書 §8、6 項目チェックリスト)

### 1.2 適用範囲

- 対象ソフトウェア: TH25-CTRL(C++20 で実装される仮想シミュレータ制御ソフトウェア)
- 対象バージョン: 0.2.0 以降(本 v0.1 SDD は Inc.1 範囲、Inc.2/3/4 範囲は構造的前提のみ確定)
- 対象 SAD: SAD-TH25-001 v0.1 全 ARCH 要素(ARCH-001〜004、IF-U-001〜010、IF-E-001〜004、SEP-001/002/003)
- 対象 SRS: SRS-TH25-001 v0.1 全要求事項
- 対象 RMF: RMF-TH25-001 v0.1 の HZ-001〜010 / RCM-001〜019(Inc.1 該当 RCM)
- スコープ外: コード実装(Step 17+)、ユニット試験ケース実装(UTPR-TH25-001 v0.1、Step 17+)

### 1.3 用語と略語

本書で使用する略語は CLAUDE.md「英語略語の統一」表に従う。本書固有の用語は以下のとおり。

| 用語 | 定義 |
|------|------|
| ユニット(Unit) | 検証可能な粒度のソフトウェア最小単位。本書では C++ クラスまたは関連する自由関数群に相当(IEC 62304 §3.27) |
| 公開 API(Public API) | ユニット外部から呼出可能な関数・メソッド。本書では `public:` メンバ関数・自由関数として記述 |
| 内部実装(Internal) | ユニット内部のみで使用する関数・データ。本書では `private:` または無名名前空間 |
| 強い型(Strong Type) | 単位や意味を C++ 型として表現し、暗黙変換による単位混在を静的に防ぐ型(例: `Energy_MeV`、`DoseUnit_cGy`)。本書では `class` ベースの単一値ラップ + `explicit` コンストラクタ + 演算子オーバーロードを最小限とする設計を採用 |
| メッセージパッシング(Message Passing) | 共有変数を介さず、複製済み確定データを queue または IPC channel で送受信する通信方式(SRS-RCM-019 / SAD §9 SEP-003) |
| 状態機械(State Machine) | `enum class` で表現した有限状態の集合と、状態遷移関数群。本書では `LifecycleState` を SafetyCoreOrchestrator(UNIT-201)で実装 |
| SPSC / MPSC | Single-Producer Single-Consumer / Multi-Producer Single-Consumer の lock-free queue 区分 |

## 2. 参照文書

| ID | 文書名 | バージョン |
|----|--------|----------|
| [1] | IEC 62304:2006+A1:2015 / JIS T 2304:2017 ヘルスソフトウェア — ソフトウェアライフサイクルプロセス | — |
| [2] | ISO 14971:2019 医療機器 — リスクマネジメントの医療機器への適用 | — |
| [3] | ISO/IEC 14882:2020(C++20)国際標準 | — |
| [4] | MISRA C++ 2023 | — |
| [5] | CERT C++ Secure Coding Standard | — |
| [6] | Google Protocol Buffers Language Guide(proto3) | — |
| [7] | ソフトウェア要求仕様書(SRS-TH25-001) | 0.1 |
| [8] | ソフトウェアアーキテクチャ設計書(SAD-TH25-001) | 0.1 |
| [9] | リスクマネジメントファイル(RMF-TH25-001) | 0.1 |
| [10] | ソフトウェア開発計画書(SDP-TH25-001) | 0.1 |
| [11] | ソフトウェアリスクマネジメント計画書(SRMP-TH25-001) | 0.1 |
| [12] | ソフトウェア構成管理計画書(SCMP-TH25-001) | 0.1 |
| [13] | ソフトウェア安全クラス決定記録(SSC-TH25-001) | 0.1 |
| [14] | ソフトウェア問題解決手順書(SPRP-TH25-001) | 0.1 |
| [15] | 構成アイテム一覧(CIL-TH25-001) | 0.7 |
| [16] | CCB 運用規程(CCB-TH25-001) | 0.1 |
| [17] | 変更要求台帳(CRR-TH25-001) | 0.5 |
| [18] | N. G. Leveson, C. S. Turner. "An Investigation of the Therac-25 Accidents." IEEE Computer, 26(7):18-41, 1993. | — |
| [19] | C. Bandela. *folly::ProducerConsumerQueue ドキュメント*(Facebook folly ライブラリの SPSC 実装、本 SDD では参照実装として引用) | GitHub HEAD |

## 3. ソフトウェア項目のソフトウェアユニットへの改良(箇条 5.4.1)

### 3.1 ユニット階層

SAD §4.3 の項目構造をユニットレベルまで分解する。命名規則は `UNIT-{プロセス番号}{プロセス内連番}`(プロセス番号: 1=UI、2=Safety Core、3=Hardware Simulator、4=MessageBus、共通=200/2 系統内)。

```
ARCH-001 Operator UI Process  (Inc.4 で本格化、本 v0.1 では空殻)
├── UNIT-101  PrescriptionEditor       (Inc.4)
├── UNIT-102  BeamCommandConsole       (Inc.4)
├── UNIT-103  AlarmDisplay             (Inc.4)
└── UNIT-104  UIAuthenticationGateway  (Inc.4)

ARCH-002 Safety Core Process  (Inc.1 中核)
├── UNIT-200  CommonTypes              (本 v0.1 で確定: 強い型、enum class、Result<T,E>)
├── UNIT-201  SafetyCoreOrchestrator   (Inc.1: イベントループ、状態機械)
├── UNIT-202  TreatmentModeManager     (Inc.1: モード遷移整合性)
├── UNIT-203  BeamController           (Inc.1: ビームオン/オフ即時遷移)
├── UNIT-204  DoseManager              (Inc.1: ドーズ目標管理・積算)
├── UNIT-205  TurntableManager         (Inc.1: 3 系統センサ集約)
├── UNIT-206  BendingMagnetManager     (Inc.1: 磁石電流指令・偏差監視)
├── UNIT-207  SafetyMonitor            (Inc.2 で本格化、本 v0.1 では空殻 + IF のみ)
├── UNIT-208  StartupSelfCheck         (Inc.1: 起動時 4 項目自己診断)
├── UNIT-209  AuditLogger              (Inc.4 で本格化、本 v0.1 では空殻 + IF のみ)
└── UNIT-210  CoreAuthenticationGateway(Inc.4 で本格化、本 v0.1 では空殻 + IF のみ)

ARCH-003 Hardware Simulator Process  (Inc.1)
├── UNIT-301  ElectronGunSim
├── UNIT-302  BendingMagnetSim
├── UNIT-303  TurntableSim             (3 系統センサ + 故障注入機能)
└── UNIT-304  IonChamberSim            (2 組 + サチュレーション/片系失陥模擬)

ARCH-004 MessageBus
├── UNIT-401  InProcessQueue           (Inc.1: SPSC lock-free queue 抽象)
└── UNIT-402  InterProcessChannel      (Inc.1: gRPC over UDS ラッパ)
```

### 3.2 ユニット一覧

| ユニット ID | 名称 | 所属項目 | 安全クラス | Inc 確定範囲 | 概要 |
|------------|------|---------|----------|-------------|------|
| UNIT-101 | PrescriptionEditor | ARCH-001.1 | C(分離後) | Inc.4 | 治療計画入力 UI(空殻、Inc.4 で本格化) |
| UNIT-102 | BeamCommandConsole | ARCH-001.2 | C(分離後) | Inc.4 | ビームオン/オフ操作 UI(空殻、Inc.4) |
| UNIT-103 | AlarmDisplay | ARCH-001.3 | C(分離後) | Inc.4 | アラーム/エラー表示 UI(空殻、Inc.4) |
| UNIT-104 | UIAuthenticationGateway | ARCH-001.4 | C(分離後) | Inc.4 | 操作者認証 UI(空殻、Inc.4) |
| UNIT-200 | CommonTypes | ARCH-002 共通 | C | **Inc.1** | 強い型 8 種 + `enum class` 4 種(`TreatmentMode` / `LifecycleState` / `ErrorCode` / `BeamState`) + `Result<T,E>` 型 + `enum class Severity` |
| UNIT-201 | SafetyCoreOrchestrator | ARCH-002.1 | C | **Inc.1** | イベントループ(単一スレッド) + `LifecycleState` 状態機械(8 状態)+ メッセージディスパッチ |
| UNIT-202 | TreatmentModeManager | ARCH-002.2 | C | **Inc.1** | `TreatmentMode` 遷移整合性チェック + ターンテーブル位置・磁石電流確認(SRS-001/002/003) |
| UNIT-203 | BeamController | ARCH-002.3 | C | **Inc.1** | ビームオン許可フラグ + ビームオフ即時遷移保証(< 10 ms、SRS-007/011/SRS-101) |
| UNIT-204 | DoseManager | ARCH-002.4 | C | **Inc.1** | `std::atomic<uint64_t>` ドーズ積算 + 目標到達検知(SRS-008/009/010) |
| UNIT-205 | TurntableManager | ARCH-002.5 | C | **Inc.1** | 3 系統独立センサ集約 + 偏差判定(SRS-006/SRS-I-005/SRS-RCM-005) |
| UNIT-206 | BendingMagnetManager | ARCH-002.6 | C | **Inc.1** | 磁石電流指令 + 偏差監視(SRS-005/SRS-I-007/SRS-ALM-004) |
| UNIT-207 | SafetyMonitor | ARCH-002.7 | C | Inc.2 で本格化 | 独立並行タスクのインターロック監視(本 v0.1 では `IF-U-006` メッセージ受信のみ実装、判定ロジックは Inc.2) |
| UNIT-208 | StartupSelfCheck | ARCH-002.8 | C | **Inc.1** | 起動時 4 項目自己診断(SRS-012/SRS-ALM-005/RCM-013) |
| UNIT-209 | AuditLogger | ARCH-002.9 | C | Inc.4 で本格化 | append-only 構造化ログ書込(本 v0.1 では `IF-U-007` メッセージ受信 + stderr プレースホルダ出力のみ、永続化は Inc.4) |
| UNIT-210 | CoreAuthenticationGateway | ARCH-002.10 | C | Inc.4 で本格化 | 認証検証(本 v0.1 では `IF-U-010` メッセージ受信 + 常時拒否プレースホルダ、認証ロジックは Inc.4) |
| UNIT-301 | ElectronGunSim | ARCH-003.1 | B(分離後) | **Inc.1** | 仮想電子銃応答模擬(SRS-IF-001) |
| UNIT-302 | BendingMagnetSim | ARCH-003.2 | B(分離後) | **Inc.1** | 仮想ベンディング磁石応答模擬(SRS-IF-002) |
| UNIT-303 | TurntableSim | ARCH-003.3 | B(分離後) | **Inc.1** | 仮想ターンテーブル(3 系統独立センサ)応答模擬 + 故障注入(SRS-IF-003) |
| UNIT-304 | IonChamberSim | ARCH-003.4 | B(分離後) | **Inc.1** | 仮想イオンチェンバ(2 組)応答模擬 + サチュレーション/片系失陥模擬(SRS-IF-004) |
| UNIT-401 | InProcessQueue | ARCH-004.1 | C | **Inc.1** | SPSC lock-free queue 抽象(`folly::ProducerConsumerQueue` 採用、SRS-RCM-019/SEP-003) |
| UNIT-402 | InterProcessChannel | ARCH-004.2 | C | **Inc.1** | gRPC over Unix Domain Socket ラッパ(SRS-IF-005/SEP-001/SEP-002) |

## 4. ソフトウェアユニットの詳細設計(箇条 5.4.2 ― クラス C)

各ソフトウェアユニットについて、実装可能な粒度で詳細を定義する。**Inc.1 中核ユニット**(UNIT-200/201/202/203/204/205/206/208/301/302/303/304/401/402)は本書 v0.1 で全項目を確定する。**Inc.2/4 該当ユニット**(UNIT-207/209/210 および UNIT-101〜104)は本書 v0.1 では公開 API 骨格と空殻実装方針のみ確定し、詳細は後続 Inc SDD 改訂で展開する。

### 4.1 UNIT-200: CommonTypes

- **目的 / 責務:** 全 UNIT が共有する型定義(強い型・`enum class`・`Result<T,E>`)を集約する。本ユニットはヘッダオンリー実装(`include/th25_ctrl/common_types.hpp`)とし、リンク時依存を最小化する。
- **関連 SRS:** SRS-D-001〜010、SRS-001、SRS-RCM-008
- **関連 RCM:** RCM-001(`enum class TreatmentMode`)/ RCM-003(`std::atomic<uint64_t>` 経由の `PulseCount`)/ RCM-008(強い型による単位安全)
- **安全クラス:** C
- **コーディング規約:** MISRA C++ 2023 全章準拠、`-Wall -Wextra -Wpedantic -Werror`、clang-tidy `bugprone-*`/`cert-*`/`cppcoreguidelines-*` 全 Pass

#### 公開型(主要のみ抜粋、完全リストはヘッダ参照)

| 型名 | 定義概要 | 値域 | スレッド安全性 | 関連 |
|------|---------|------|--------------|------|
| `enum class TreatmentMode : uint8_t` | `Electron = 1, XRay = 2, Light = 3` の 3 値。`uint8_t` 基底で他整数型との暗黙変換禁止 | 3 値のみ | 値型のためスレッド安全 | SRS-001 / SRS-D-001 / RCM-001 |
| `enum class LifecycleState : uint8_t` | `Init / SelfCheck / Idle / PrescriptionSet / Ready / BeamOn / Halted / Error` の 8 値 | 8 値 | UNIT-201 内部のみで使用、外部公開時は値コピー | SRS-002 / SRS-007 / SRS-012 |
| `enum class BeamState : uint8_t` | `Off / Arming / On / Stopping` の 4 値 | 4 値 | 値型 | SRS-007 / SRS-011 |
| `enum class ErrorCode : uint16_t` | 8 系統階層化(下記 §4.1.1) | 列挙値 | 値型 | SRS-UX-001 / RCM-009 |
| `enum class Severity : uint8_t` | `Critical / High / Medium / Low` の 4 値 | 4 値 | 値型 | SRS-ALM-001〜005 / RCM-010 |
| `class Energy_MeV` | `double` ラップ、`explicit` コンストラクタ、`operator<=>` のみ提供、算術演算子非提供 | 1.0〜25.0 MeV(電子線)/ 5.0〜25.0 MV(X 線、`Energy_MV` で別型) | 値型 | SRS-004 / SRS-D-002 / RCM-008 |
| `class Energy_MV` | `Energy_MeV` と別型(暗黙変換禁止) | 5.0〜25.0 MV | 値型 | SRS-D-003 |
| `class DoseUnit_cGy` | `double` ラップ、加算は `add_dose(DoseUnit_cGy)` メソッドで明示、減算非提供(放射線は累積のみ) | 0.01〜10000.0 cGy | 値型 | SRS-008 / SRS-D-004 / RCM-008 |
| `class PulseCount` | `uint64_t` ラップ、`std::atomic<uint64_t>` 経由のみインクリメント可能(`PulseCounter` クラスで保護)、本クラス自体は値スナップショット | 0〜2^64-1 | 値型(コピー時は atomic load) | SRS-009 / SRS-D-005 / RCM-003 |
| `class PulseCounter` | `std::atomic<uint64_t>` 内包、`fetch_add(PulseCount delta, std::memory_order_acq_rel)` のみ提供 | — | **lock-free**(`is_lock_free() == true` を `static_assert`) | SRS-009 / SRS-RCM-019 / RCM-002/003/004 |
| `class MagnetCurrent_A` | `double` ラップ | 0.0〜500.0 A | 値型 | SRS-005 / SRS-D-006 |
| `class Position_mm` | `double` ラップ、`abs_diff(Position_mm)` 提供 | -100.0〜+100.0 mm | 値型 | SRS-006 / SRS-D-007 / RCM-005 |
| `class ElectronGunCurrent_mA` | `double` ラップ | 0.0〜10.0 mA | 値型 | SRS-O-001 / SRS-D-008 |
| `template<class T, class E = ErrorCode> class Result` | `std::variant<T, E>` ベース。C++23 `std::expected` 互換 IF(`has_value()`, `value()`, `error()`)。本プロジェクトは C++20 のため独自実装 | — | 値型 | SRS-007 / SRS-UX-001 |

#### §4.1.1 `enum class ErrorCode` の階層

8 系統 × 各系統内 4〜6 件の階層化された列挙値。**上位 8 ビットを系統識別、下位 8 ビットを系統内連番** とすることで、ビット演算で系統判定可能(`(static_cast<uint16_t>(ec) >> 8) == 0x01` でモード系判定)。

| 系統 | 上位バイト | 主要 ErrorCode | 関連 Severity |
|------|---------|--------------|-------------|
| Mode 系 | `0x01` | `ModeInvalidTransition` (0x0101) / `ModePositionMismatch` (0x0102) / `ModeBeamOnNotAllowed` (0x0103) | Critical |
| Beam 系 | `0x02` | `BeamOnNotPermitted` (0x0201) / `BeamOffTimeout` (0x0202) / `ElectronGunCurrentOutOfRange` (0x0203) | Critical |
| Dose 系 | `0x03` | `DoseOutOfRange` (0x0301) / `DoseTargetExceeded` (0x0302) / `DoseSensorFailure` (0x0303) / `DoseOverflow` (0x0304) | Critical |
| Turntable 系 | `0x04` | `TurntableOutOfPosition` (0x0401) / `TurntableSensorDiscrepancy` (0x0402) / `TurntableMoveTimeout` (0x0403) | Critical / High |
| Magnet 系 | `0x05` | `MagnetCurrentDeviation` (0x0501) / `MagnetCurrentSensorFailure` (0x0502) | High / Medium |
| IPC 系 | `0x06` | `IpcChannelClosed` (0x0601) / `IpcMessageTooLarge` (0x0602) / `IpcDeserializationFailure` (0x0603) / `IpcQueueOverflow` (0x0604) | High |
| Auth 系 | `0x07` | `AuthRequired` (0x0701) / `AuthInvalid` (0x0702) / `AuthExpired` (0x0703) | High(Inc.4 で確定) |
| Internal 系 | `0xFF` | `InternalAssertion` (0xFF01) / `InternalQueueFull` (0xFF02) / `InternalUnexpectedState` (0xFF03) | Critical(原則 fail-stop) |

**Severity 判定規則(SRS-UX-002 / RCM-010 への対応):** Critical は `LifecycleState` を `Halted` に遷移させ、操作者の単一キー操作によるバイパスを禁止(物理的再起動が必要)。High/Medium/Low は警告表示 + ログ記録のみ。

#### 資源使用量・タイミング制約

- 本ユニットはヘッダオンリーのため、リンク時の追加バイナリサイズ < 1 KB(テンプレートインスタンス化分のみ)
- すべての演算は値コピーまたは atomic load/store のみ。動的メモリ確保なし
- スタック使用量 ≦ 256 バイト(最大型 `Result<DoseUnit_cGy, ErrorCode>` で `sizeof = 24 バイト` 程度)

### 4.2 UNIT-201: SafetyCoreOrchestrator

- **目的 / 責務:** Safety Core Process のエントリポイント。単一スレッドのイベントループで `LifecycleState` 状態機械を駆動し、`MessageBus`(UNIT-401)から取得したメッセージを各 Manager(UNIT-202〜206)へディスパッチする。
- **関連 SRS:** SRS-002 / SRS-007 / SRS-012 / SRS-RCM-001
- **関連 RCM:** RCM-001(状態機械による不正遷移の構造的排除)/ RCM-013(起動時自己診断)/ RCM-017(ビームオン→オフ即時遷移)
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `int run_event_loop()` | なし | プロセス終了コード(0=正常、非 0=異常) | `init_subsystems()` が成功済(自己診断含む) | 終了時は全 Manager に `ShutdownRequest` 送信、queue ドレイン | 例外は `InternalAssertion` ErrorCode で fail-stop |
| `Result<void, ErrorCode> init_subsystems()` | なし | `Result` | プロセス起動直後に main から 1 回のみ呼出 | `LifecycleState = Init` から `SelfCheck` への遷移完了 | 自己診断失敗時は `InternalAssertion` を含む `Result::error()` を返す |
| `LifecycleState current_state() const noexcept` | なし | 現在の `LifecycleState` | なし | なし(read-only) | なし(noexcept) |
| `void shutdown() noexcept` | なし | なし | 任意 | `BeamState = Stopping` 強制、queue クローズ | `noexcept`、シグナルハンドラから呼出可 |

#### `LifecycleState` 状態機械

```
   ┌─────┐
   │Init │
   └──┬──┘
      │ init_subsystems()
      ▼
   ┌──────────┐    fail   ┌──────┐
   │SelfCheck │──────────▶│Error │ (致死的: shutdown)
   └────┬─────┘           └──────┘
        │ pass
        ▼
   ┌─────┐  PrescriptionSet message  ┌────────────────┐
   │Idle │◀───────────────────────────│PrescriptionSet │
   └──┬──┘                            └───────┬────────┘
      │ ResetPrescription                     │ all params validated
      │                                       ▼
      │                                  ┌──────┐
      │ ◀──ResetPrescription──────────── │Ready │
      │                                  └──┬───┘
      │                                     │ BeamOnRequest (許可フラグ確認)
      │                                     ▼
      │                                  ┌──────┐
      │ ◀──BeamOff/DoseTargetReached──── │BeamOn│
      │                                  └──┬───┘
      │                                     │ Critical alarm OR shutdown signal
      │                                     ▼
      │                                  ┌──────┐
      │ ◀──Manual restart only───────── │Halted │
      │                                  └──────┘
      ▼
   (loop back to Idle)
```

**重要な遷移制約(RCM-001):**
- `Idle → BeamOn` の直接遷移は禁止(必ず `PrescriptionSet → Ready → BeamOn` を経由)
- `BeamOn → Idle` の直接遷移は禁止(必ず `BeamOff` 完了通知を経由、< 10 ms 保証 SRS-101)
- `Halted` からの遷移は **物理的再起動のみ**(`shutdown()` でプロセス終了 → 再起動 → `Init`)

#### 資源使用量・タイミング制約

- スタック使用量(イベントループスレッド): ≦ 8 KB(関数呼出深さ ≦ 16、ローカル変数最大 1 KB × 8 フレーム)
- ヒープ確保: 起動時のみ(MessageBus / 各 Manager のインスタンス化)。イベントループ中は **動的メモリ確保禁止**(MISRA C++ 21-2-x)
- メッセージディスパッチ周期: 100 μs 以内(目標、queue から取得 → switch dispatch → Manager メソッド呼出 → 戻り)
- `BeamOff` 指令から電子銃停止までの最大遅延: < 10 ms(SRS-101、ウォッチドッグタイマで監視)

#### 例外・異常系の扱い

| 異常条件 | 検出方法 | 処置 |
|---------|---------|------|
| `MessageBus` queue full(`IpcQueueOverflow`) | `try_publish()` の戻り値 false | Critical: `LifecycleState = Halted`、`AuditLogger` に記録、`AlarmDisplay` に通知、shutdown |
| Manager メソッドからの `Result::error()` | switch case 内で `has_value()` チェック | Severity 別に処置: Critical → `Halted` 遷移、High → 警告表示+継続、Medium/Low → ログのみ |
| 不正状態遷移試行 | `transition_to()` 内で許可表(下記)と照合 | `ModeInvalidTransition` を `Result::error()` で返却、Halted 遷移 |
| 例外スロー(C++ 標準ライブラリから) | `try { event_loop_step(); } catch (...)` | `InternalAssertion` で fail-stop、shutdown(MISRA C++ では関数内例外スロー禁止だが、SOUP からの伝播は許容) |

### 4.3 UNIT-202: TreatmentModeManager

- **目的 / 責務:** 治療モード(`TreatmentMode`)の状態と、SRS-002 のモード遷移シーケンス(6 ステップ)を実装する。HZ-001(電子モード+X 線ターゲット非挿入)への直接対応の中核ユニット。
- **関連 SRS:** SRS-001 / SRS-002 / SRS-003 / SRS-004 / SRS-RCM-001
- **関連 RCM:** RCM-001(モード遷移整合性)/ RCM-005(in-position 冗長確認、TurntableManager と連携)
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `Result<void, ErrorCode> request_mode_change(TreatmentMode new_mode, Energy_MeV energy)` | 新モード、エネルギー(電子線時、`Energy_MV` は別オーバーロード) | `Result` | `BeamState == Off`(SRS-002 step 1) | 成功時: `TurntableManager::move_to(...)` および `BendingMagnetManager::set_current(...)` がスケジュール済 | ビーム照射中時 `ModeBeamOnNotAllowed`、エネルギー範囲外時 `Mode系` 該当 ErrorCode |
| `Result<void, ErrorCode> verify_mode_consistency(TreatmentMode requested_mode) const` | 要求モード(ビームオン要求時に再確認) | `Result` | 任意 | なし(read-only) | センサ 3 系統と要求モードの不整合時 `ModePositionMismatch` |
| `TreatmentMode current_mode() const noexcept` | なし | 現在モード | なし | なし | なし |

#### モード遷移許可表(SRS-002)

| 現在モード | 要求モード | 遷移可否 | 検証ステップ |
|----------|---------|---------|------------|
| Light | Electron | 可 | 6 ステップ全実施(SRS-002) |
| Light | XRay | 可 | 同上 |
| Electron | XRay | 可 | 同上(中間で Light を経由しない直接遷移を許容、ただし磁場安定待機 100 ms 必須) |
| XRay | Electron | 可 | 同上 |
| いずれか → 同モード | 同モード | 可(no-op、ただし `request_mode_change` は成功 Result を返す) | パラメータ更新のみ |
| いずれか | Light | 可(治療終了時) | ビームオフ確認後、ターンテーブル位置のみ移動 |

#### 資源使用量・タイミング制約

- スタック使用量: ≦ 1 KB
- ヒープ確保: なし(メンバ変数のみ、すべて値型)
- `request_mode_change` 完了通知までの最大遅延: < 500 ms(SRS-102、ターンテーブル移動 + 磁石電流安定待機 100 ms を含む)

### 4.4 UNIT-203: BeamController

- **目的 / 責務:** ビームオン許可フラグの管理と、ビームオン/オフ指令の即時遷移保証。HZ-001 / HZ-004 への直接対応。
- **関連 SRS:** SRS-007 / SRS-011 / SRS-101 / SRS-RCM-017
- **関連 RCM:** RCM-017(ビームオン → ビームオフ即時遷移、< 10 ms)
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `Result<void, ErrorCode> request_beam_on()` | なし | `Result` | `LifecycleState == Ready`、ビームオン許可フラグ true | 成功時: `BeamState = Arming → On`、`InterProcessChannel` 経由で `ElectronGunCurrent_mA` を電子銃に設定 | 許可フラグ未設定時 `BeamOnNotPermitted`、モード/位置不整合時 `ModePositionMismatch`(UNIT-202 経由で確認) |
| `Result<void, ErrorCode> request_beam_off()` | なし | `Result` | 任意 | `BeamState = On → Stopping → Off`、< 10 ms で電子銃電流 = 0 mA(SRS-101) | タイムアウト時 `BeamOffTimeout`(Critical) |
| `BeamState current_state() const noexcept` | なし | 現在状態 | なし | なし | なし |

#### ビームオン許可フラグの設定根拠(RCM-001 連携)

ビームオン許可フラグは `std::atomic<bool>` で実装し、UNIT-202 が SRS-002 シーケンス(6 ステップ)完了時のみ true に設定する。`request_beam_on()` 内で再度フラグを確認(`compare_exchange_strong` で false に reset)し、フラグが true でなければ `BeamOnNotPermitted` を返す。これにより以下を保証:

1. SRS-002 シーケンス未完了状態でのビームオン要求は構造的に拒否
2. 1 回のビームオン要求で 1 回しか照射されない(`compare_exchange` による消費)
3. race condition は発生しない(SAD §9 SEP-003 + `std::atomic` メモリオーダリング)

#### 資源使用量・タイミング制約

- スタック使用量: ≦ 512 バイト
- `request_beam_off()` から電子銃停止確認までの最大遅延: **< 10 ms**(SRS-101 / SRS-011)。UNIT-201 が `BeamOff` 指令を発行 → UNIT-203 が `InterProcessChannel` に書込 → UNIT-301 ElectronGunSim が 0 mA に設定 → 計測値 = 0 mA を `IF-E-002` 経由で受信 → UNIT-203 が `BeamState = Off` に遷移、までを 10 ms 以内に完了

### 4.5 UNIT-204: DoseManager

- **目的 / 責務:** ドーズ目標管理 + ドーズ積算 + 目標到達検知 + 即時ビームオフ指令。HZ-005(ドーズ計算誤り)直接対応 + HZ-003(整数オーバフロー)構造的排除の中核。
- **関連 SRS:** SRS-008 / SRS-009 / SRS-010 / SRS-103 / SRS-RCM-008
- **関連 RCM:** RCM-003(整数オーバフロー防御、`std::atomic<uint64_t>` 採用)/ RCM-004(共有変数の atomic 必須化)/ RCM-008(境界値・型による単位安全)
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `Result<void, ErrorCode> set_dose_target(DoseUnit_cGy target)` | 目標ドーズ | `Result` | `LifecycleState ∈ {PrescriptionSet, Ready}` | 内部目標 = `target`、累積ドーズ = 0 にリセット | 範囲外(0.01〜10000.0 cGy)時 `DoseOutOfRange` |
| `void on_dose_pulse(PulseCount pulse_delta) noexcept` | 1 パルス分のドーズ増分 | なし | 任意(IonChamberSim から 1 kHz で呼出される) | `accumulated_pulses_` に `fetch_add(pulse_delta, std::memory_order_acq_rel)`。目標到達検知時は `IF-U-002` 経由で `BeamOff` 要求送信 | `noexcept`、`SafetyMonitor` が後段で監視 |
| `DoseUnit_cGy current_accumulated() const noexcept` | なし | 現在の累積ドーズ | なし | なし | なし(read-only、`atomic load`) |

#### `PulseCounter` の使用方針(RCM-003 構造的予防)

DoseManager は `PulseCounter accumulated_pulses_;`(UNIT-200 で定義)を内部に保持する。`PulseCounter` は内部で `std::atomic<uint64_t>` を持ち、`fetch_add(PulseCount, std::memory_order_acq_rel)` のみ提供する。これにより:

1. **整数オーバフロー(HZ-003)の構造的排除:** `uint64_t` の最大値 2^64 - 1 ≒ 1.8 × 10^19 パルスは、1 kHz サンプリングで 5,800 万年連続照射相当。実用上オーバフロー不可能(SRS-009 と整合)
2. **race condition(HZ-002)の構造的排除:** `std::atomic` + `acq_rel` メモリオーダリングで、UNIT-201 イベントループスレッドと IonChamberSim からの呼出スレッド間の競合を排除(TSan で機械的検証可能、SOUP-015)
3. **Yakima Class3 オーバフロー類型の排除:** Therac-25 の `Class3` は 8bit unsigned カウンタの 256 周期問題が事故主要因だったが、本設計は 64bit + atomic で構造的に発生不可能

#### 目標到達検知ロジック

```cpp
void on_dose_pulse(PulseCount pulse_delta) noexcept {
    PulseCount current = accumulated_pulses_.fetch_add(pulse_delta);
    DoseUnit_cGy current_dose = pulse_count_to_dose(current);  // 単位変換、明示的関数
    if (current_dose >= target_dose_) {
        // 目標到達: BeamOff 要求を MessageBus に publish (lock-free, < 1 μs)
        bus_.try_publish(BeamOffRequest{Reason::DoseTargetReached});
    }
}
```

**重要:** `pulse_count_to_dose()` は強い型 `PulseCount` から `DoseUnit_cGy` への明示的変換関数。倍率は `DoseRatePerPulse_cGy_per_pulse` 強い型でコンストラクタ引数として注入(SDD §6.5)。これにより校正値変更を構成定義として扱える。

#### 資源使用量・タイミング制約

- スタック使用量: ≦ 256 バイト
- ヒープ確保: なし
- `on_dose_pulse` の最大実行時間: **< 1 μs**(`atomic fetch_add` + 比較 + 条件付き `try_publish`)
- 目標到達検知から BeamOff 指令発行までの遅延: **< 1 ms**(SRS-010、UNIT-201 のディスパッチ周期 100 μs を含む)

### 4.6 UNIT-205: TurntableManager

- **目的 / 責務:** ターンテーブル位置指令と 3 系統独立センサ集約 + 偏差判定。RCM-005(in-position 冗長確認)の中核。
- **関連 SRS:** SRS-002 / SRS-006 / SRS-I-005 / SRS-104 / SRS-ALM-003 / SRS-RCM-005
- **関連 RCM:** RCM-001 / RCM-005
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `Result<void, ErrorCode> move_to(Position_mm target)` | 目標位置(所定 3 位置のいずれか) | `Result` | 任意 | `InterProcessChannel` 経由で `TurntableSim` に位置指令、3 系統センサが目標位置 ±0.5 mm 以内に到達するまで待機(タイムアウト 500 ms) | タイムアウト時 `TurntableMoveTimeout`、3 系統不一致(差 > 1.0 mm)時 `TurntableSensorDiscrepancy` |
| `Result<TurntablePosition, ErrorCode> read_position()` | なし | 3 系統センサ集約結果 | 任意 | なし | センサ通信失敗時 `IpcChannelClosed` |
| `bool is_in_position(Position_mm expected) const` | 期待位置 | bool | 任意 | なし | なし |

#### 3 系統センサ集約アルゴリズム(RCM-005)

```cpp
struct TurntablePosition {
    Position_mm sensor_0;
    Position_mm sensor_1;
    Position_mm sensor_2;

    // 集約値: 3 系統の中央値(med-of-3)を採用
    Position_mm median() const noexcept;

    // 偏差判定: max - min が閾値超か
    bool has_discrepancy(double threshold_mm = 1.0) const noexcept;
};

// is_in_position 実装:
bool is_in_position(Position_mm expected) const {
    auto pos = read_position().value();  // エラー時は false
    if (pos.has_discrepancy()) return false;       // 3 系統不一致は in-position と認めない
    return pos.median().abs_diff(expected) <= Position_mm{0.5};
}
```

**RCM-005 への対応:** Inc.1 では「3 系統センサ部分」のみ実装(SRS-RCM-005 の表記と整合)。Inc.2 で「モデル予測 + 前回照射時刻」を追加し、3 系統独立確認に拡張する。

#### 資源使用量・タイミング制約

- スタック使用量: ≦ 1 KB
- ヒープ確保: なし
- センサ更新レート: ≧ 100 Hz(SRS-104、`TurntableSim` から polling)
- `move_to()` の最大実行時間: 500 ms(タイムアウト含む)

### 4.7 UNIT-206: BendingMagnetManager

- **目的 / 責務:** ベンディング磁石電流指令(エネルギー対応表に基づく) + 計測値偏差監視。
- **関連 SRS:** SRS-005 / SRS-I-007 / SRS-O-002 / SRS-ALM-004
- **関連 RCM:** RCM-008(強い型による単位安全)
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `Result<void, ErrorCode> set_current_for_energy(TreatmentMode mode, Energy_MeV energy_e_or_mv)` | モード(電子線/X 線判定用) + エネルギー | `Result` | 任意 | `BendingMagnetSim` に `MagnetCurrent_A` 指令送信、`±5%` 以内に到達するまで待機(タイムアウト 200 ms) | 範囲外時 `MagnetCurrentDeviation` |
| `MagnetCurrent_A current_actual() const` | なし | 計測値 | 任意 | なし | センサ失敗時例外なし、最後の有効値を返す |
| `bool is_within_tolerance() const` | なし | 偏差 5% 以内か | 任意 | なし | なし |

#### エネルギー → 磁石電流 対応表(SRS-005)

対応表は `class EnergyMagnetMap` として `UNIT-206` に内包し、初期化時に SDD §6.5 で定義する `dose_ratio.toml` または C++ 構造体で注入する(SDP §6.1 で確定予定)。本 SDD では電子線時 1〜25 MeV / X 線時 5〜25 MV の対応表を線形補間で実装することのみ確定し、具体値は **構成定義** として扱う(コードに直接埋め込まない、HZ-007 構造的予防)。

#### 資源使用量・タイミング制約

- スタック使用量: ≦ 512 バイト
- ヒープ確保: 起動時のみ(対応表ロード時)
- `set_current_for_energy()` の最大実行時間: 200 ms(電流安定待機含む)

### 4.8 UNIT-207: SafetyMonitor(Inc.2 で本格化、本 v0.1 では空殻)

- **目的 / 責務:** 独立並行タスクのインターロック監視。本 v0.1 では `IF-U-006` メッセージ受信のみ実装し、判定ロジックは Inc.2 で実装。
- **関連 SRS:** SRS-ALM-001/002/003/004/005、SRS-RCM-005、SRS-RCM-006(Inc.2 で要求化予定)
- **関連 RCM:** RCM-006 / RCM-007(Inc.2 で本格化)
- **安全クラス:** C

#### 公開 API(本 v0.1 範囲)

| 関数 | 引数 | 戻り値 | 備考 |
|------|------|-------|------|
| `void run_monitor_thread()` | なし | なし | 別スレッドで起動。本 v0.1 では `MessageBus` から `SafetyAlarm` メッセージを受信して `AuditLogger` に転送するのみ。実際の異常検知ロジックは Inc.2 |
| `void stop()` | なし | なし | スレッド停止要求 |

#### 空殻実装方針

```cpp
void SafetyMonitor::run_monitor_thread() {
    while (!stop_requested_.load(std::memory_order_acquire)) {
        if (auto msg = bus_.try_consume<SafetyAlarm>()) {
            // Inc.1: 単純に AuditLogger へ転送(スタブ実装)
            audit_.publish(AuditLogEntry{...});
            // Inc.2 で本格的な検知ロジック追加予定
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
```

**Inc.2 での拡張点(本 SDD では予告のみ):** ターンテーブル位置センサ 3 系統不一致検知、ドーズ目標超過検知、ベンディング磁石電流偏差検知、起動時自己診断失敗検知を本ユニット内で並行監視する。

### 4.9 UNIT-208: StartupSelfCheck

- **目的 / 責務:** 起動時 4 項目自己診断(SRS-012)。
- **関連 SRS:** SRS-012 / SRS-ALM-005 / RCM-013
- **関連 RCM:** RCM-013(起動時状態検証)
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `Result<void, ErrorCode> perform_self_check()` | なし | `Result` | プロセス起動直後、UNIT-201::init_subsystems() から呼出 | 4 項目全 Pass で成功、いずれか Fail で `Result::error()` | 失敗内容を `ErrorCode` に格納 |

#### 自己診断 4 項目(SRS-012)

1. **電子銃電流確認**: `ElectronGunSim` から計測値読取、0.0 mA であること(`abs_diff < 0.01 mA`)
2. **ターンテーブル位置確認**: `TurntableSim` から 3 系統センサ読取、Light 位置(-50.0 mm)±0.5 mm 以内、3 系統偏差 < 1.0 mm
3. **ベンディング磁石電流確認**: `BendingMagnetSim` から計測値読取、0.0 A であること
4. **ドーズ積算カウンタ初期化確認**: UNIT-204 の `accumulated_pulses_` が 0 であること(プロセス起動直後は当然 0、健全性チェック)

各項目失敗時は対応する `ErrorCode` を `Result::error()` で返却。UNIT-201 が `Halted` 遷移し、操作者 UI に詳細メッセージ表示(Inc.4)。

### 4.10 UNIT-209: AuditLogger(Inc.4 で本格化、本 v0.1 では空殻)

#### 公開 API(本 v0.1 範囲)

| 関数 | 引数 | 戻り値 | 備考 |
|------|------|-------|------|
| `void publish(AuditLogEntry entry)` | ログエントリ | なし | 本 v0.1 では `std::cerr` への JSON Lines 出力のみ(永続化は Inc.4) |
| `void run_io_thread()` | なし | なし | 独立 IO スレッド。`MessageBus` 経由で `AuditLogEntry` を受信し fsync 付き書込(Inc.4) |

### 4.11 UNIT-210: CoreAuthenticationGateway(Inc.4 で本格化、本 v0.1 では空殻)

#### 公開 API(本 v0.1 範囲)

| 関数 | 引数 | 戻り値 | 備考 |
|------|------|-------|------|
| `Result<void, ErrorCode> verify(OperatorId, Credential)` | 操作者 ID + 資格情報 | `Result` | 本 v0.1 では常時 `AuthRequired` を返す(認証ロジックは Inc.4) |

### 4.12 UNIT-301〜304: Hardware Simulator 群

各 Sim ユニットは ARCH-003 配下の独立プロセスで動作。本 v0.1 では基本的な応答模擬のみ実装し、Inc.2 以降で故障注入機能を拡張する。

#### UNIT-301 ElectronGunSim

| 関数 | 引数 | 戻り値 | 備考 |
|------|------|-------|------|
| `void set_current(ElectronGunCurrent_mA)` | 指令電流 | なし | 内部状態に保存、応答遅延 1 ms 模擬 |
| `ElectronGunCurrent_mA read_actual_current() const` | なし | 計測値 | 設定値 ± 1% のノイズ模擬 |

#### UNIT-302 BendingMagnetSim

| 関数 | 引数 | 戻り値 | 備考 |
|------|------|-------|------|
| `void set_current(MagnetCurrent_A)` | 指令電流 | なし | 内部状態に保存、応答遅延 50 ms(電流安定待機模擬) |
| `MagnetCurrent_A read_actual_current() const` | なし | 計測値 | 設定値 ± 2% のノイズ模擬 |

#### UNIT-303 TurntableSim(故障注入機能あり)

| 関数 | 引数 | 戻り値 | 備考 |
|------|------|-------|------|
| `void command_position(Position_mm)` | 目標位置 | なし | 移動模擬(50 mm/s で線形遷移) |
| `Position_mm read_sensor(SensorId)` | 0/1/2 のいずれか | センサ値 | 故障注入無効時は実位置 ± 0.1 mm。**故障注入有効時は指定値を返す**(IT/ST 用) |
| `void inject_fault(SensorId, FaultMode)` | センサ ID + 故障モード(StuckAt / Delay / NoResponse) | なし | 試験用故障注入 |

#### UNIT-304 IonChamberSim(故障注入機能あり)

| 関数 | 引数 | 戻り値 | 備考 |
|------|------|-------|------|
| `DoseUnit_cGy read_dose(ChannelId)` | 0/1 のいずれか | 累積ドーズ | 1 kHz でビーム電流 × 校正係数を積算 |
| `void inject_saturation(ChannelId)` | チャネル ID | なし | サチュレーション模擬(最大値返却) |
| `void inject_channel_failure(ChannelId)` | チャネル ID | なし | 片系失陥模擬(0 返却) |

### 4.13 UNIT-401: InProcessQueue(MessageBus SPSC 実装)

- **目的 / 責務:** プロセス内 SPSC lock-free queue 抽象。`folly::ProducerConsumerQueue` を採用し、薄いラッパで本プロジェクト固有 API を提供する。
- **関連 SRS:** SRS-RCM-019 / SRS-D-009
- **関連 RCM:** RCM-002 / RCM-004 / RCM-019
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `template<typename Msg> bool try_publish(Msg&& msg) noexcept` | メッセージ(値渡しまたは右辺値参照) | true(成功)/ false(queue full) | producer は単一スレッドのみ | 成功時 queue にエンキュー | queue full 時 false 返却(呼出側で `IpcQueueOverflow` ErrorCode を生成) |
| `template<typename Msg> std::optional<Msg> try_consume() noexcept` | なし | 取得成功時 `Msg`、失敗時 `nullopt` | consumer は単一スレッドのみ | 成功時 queue からデキュー | queue empty 時 `nullopt` 返却 |
| `void publish_blocking(Msg&& msg)` | メッセージ | なし | 上記 + queue full 時に呼出側がブロック許容 | 成功保証 | タイムアウト時(5 秒)`InternalQueueFull` 例外 |

#### 採用ライブラリと根拠

- **採用:** `folly::ProducerConsumerQueue<T>`(Facebook folly ライブラリ、Apache 2.0)
- **根拠:** SPSC で十分な使用シーン(UNIT-201 → 各 Manager は 1:1)、`is_lock_free() == true`、メモリオーダリング検証済(folly 内部で TSan 対応済)、capacity 固定(起動時に 4096 メッセージで初期化、SDD §6.5 で構成定義)
- **代替検討:** MPSC が必要な場面(複数 Sim から SafetyMonitor への通知等)では `boost::lockfree::queue<T>` を別途採用予定(Inc.2 で本格化、本 SDD §10 未確定事項として記載)

#### 資源使用量・タイミング制約

- ヒープ確保: 起動時のみ(queue capacity 4096 × `sizeof(largest_message)` ≒ 4096 × 256 バイト = 1 MB)
- `try_publish` / `try_consume` の最大実行時間: < 100 ns(lock-free atomic 操作のみ)
- `is_lock_free()` を `static_assert` で検証(SOUP-003/004 が `std::atomic<uint64_t>` lock-free を保証する前提)

### 4.14 UNIT-402: InterProcessChannel

- **目的 / 責務:** プロセス間通信(gRPC over Unix Domain Socket)のラッパ。Protocol Buffers シリアライズ/デシリアライズを内包。
- **関連 SRS:** SRS-IF-005、SAD §5.2 IF-E-001/002
- **関連 RCM:** RCM-018(SEP-001/SEP-002 の実装層)
- **安全クラス:** C

#### 公開 API

| 関数 | 引数 | 戻り値 | 事前条件 | 事後条件 | エラー処理 |
|------|------|-------|---------|---------|----------|
| `Result<void, ErrorCode> connect(std::string_view socket_path)` | UDS パス | `Result` | プロセス起動直後 | 接続確立 | `IpcChannelClosed` |
| `Result<void, ErrorCode> send(const SafetyCoreMessage& msg)` | proto3 メッセージ | `Result` | `connect()` 成功済 | gRPC 送信完了 | `IpcChannelClosed` / `IpcMessageTooLarge`(> 1 MB) |
| `Result<SafetyCoreMessage, ErrorCode> recv()` | なし | `Result<Msg>` | 同上 | 受信完了 | `IpcDeserializationFailure` |
| `void close() noexcept` | なし | なし | 任意 | 接続クローズ | なし |

#### Protocol Buffers スキーマの所在

スキーマ定義は本 SDD §5.2 で確定する(IF-E-001 / IF-E-002 / IF-E-003)。proto3 ファイルは `proto/th25_messages.proto` として CIL-CFG として登録予定(Step 17+ で実装)。

## 5. インタフェースの詳細設計(箇条 5.4.3 ― クラス C)

### 5.1 ユニット間 IF(プロセス内)— メッセージ型詳細

SAD §5.1 で定義した IF-U-001〜010 の各メッセージ型を `struct` として詳細化。すべて UNIT-200 CommonTypes に定義し、MessageBus で型安全にディスパッチする。

| IF ID | 呼出側 → 被呼出側 | メッセージ型(struct 定義の主要フィールドのみ) | 同期/非同期 | エラー返却 |
|-------|-----------------|--------------------------------------|-----------|----------|
| IF-U-001 | UNIT-201 → UNIT-202 | `struct ModeChangeRequest { TreatmentMode new_mode; std::variant<Energy_MeV, Energy_MV> energy; uint64_t request_id; };` / `struct ModeChangeResult { uint64_t request_id; Result<TreatmentMode, ErrorCode> outcome; };` | 非同期(reply は別メッセージ、`request_id` で対応) | `Result<>` |
| IF-U-002 | UNIT-201 → UNIT-203 | `struct BeamOnRequest { uint64_t request_id; };` / `struct BeamOffRequest { Reason reason; };` / `struct BeamStateChange { BeamState new_state; std::optional<ErrorCode> error; };` | 非同期 | 同上 |
| IF-U-003 | UNIT-201 → UNIT-204 | `struct SetDoseTarget { DoseUnit_cGy target; uint64_t request_id; };` / `struct DoseAccumulated { DoseUnit_cGy current; PulseCount pulses; };` / `struct DoseTargetReached {};` | 非同期 | 同上 |
| IF-U-004 | UNIT-201 → UNIT-205 | `struct MoveTurntable { Position_mm target; uint64_t request_id; };` / `struct TurntablePositionReport { Position_mm sensor_0, sensor_1, sensor_2; double max_discrepancy_mm; };` | 非同期 | 同上 |
| IF-U-005 | UNIT-201 → UNIT-206 | `struct SetMagnetCurrent { MagnetCurrent_A target; uint64_t request_id; };` / `struct MagnetCurrentReport { MagnetCurrent_A actual; double deviation_pct; };` | 非同期 | 同上 |
| IF-U-006 | UNIT-207 → UNIT-201 | `struct SafetyAlarm { AlarmId id; Severity severity; std::string detail; std::chrono::system_clock::time_point timestamp; };` / `struct SafetyAlarmAck { AlarmId id; };` | 非同期(独立タスク → イベントループ) | なし(片方向) |
| IF-U-007 | 全 UNIT → UNIT-209 | `struct AuditLogEntry { std::chrono::system_clock::time_point timestamp; EventType event_type; std::map<std::string, std::variant<...>> fields; };` | 非同期(IO スレッド) | なし(片方向) |
| IF-U-008 | 全 UNIT ↔ UNIT-401 | `template<typename Msg> bool try_publish(Msg&& msg)` / `template<typename Msg> std::optional<Msg> try_consume()` | 非同期 | bool / `std::optional` |
| IF-U-009 | UNIT-201 → UNIT-208 | `StartupCheckResult perform_self_check()`(同期 API、起動時のみ) | 同期 | `Result<void, ErrorCode>` |
| IF-U-010 | UNIT-201 → UNIT-210 | `struct AuthenticationVerify { OperatorId id; Credential cred; uint64_t request_id; };` / `struct AuthenticationResult { uint64_t request_id; Result<OperatorId, ErrorCode> outcome; };` | 非同期 | 同上 |

> **メッセージ型の不変条件:** すべてのメッセージは **値型(値コピー可能、所有権独立)** とし、ポインタ・参照を含まない。`std::string` / `std::vector` / `std::map` 等のヒープ確保型は許容するが、起動時に予約済(`reserve()`)とする方針(動的メモリ確保を実行時に最小化、SDP §6.1 MISRA C++ 21-2-x)。

### 5.2 外部 IF(プロセス間 / プロセス外)— Protocol Buffers スキーマ

#### IF-E-001: Operator UI ↔ Safety Core

```protobuf
syntax = "proto3";
package th25.api.v1;

// PrescriptionSet: 治療計画パラメータの設定
message PrescriptionSet {
    enum Mode {
        MODE_UNSPECIFIED = 0;
        MODE_ELECTRON = 1;
        MODE_XRAY = 2;
        MODE_LIGHT = 3;
    }
    Mode mode = 1;
    double energy_value = 2;        // MeV (Electron) or MV (XRay)
    double dose_target_cgy = 3;
    fixed64 request_id = 4;
}

// BeamCommand: ビームオン/オフ指令
message BeamCommand {
    enum Command {
        COMMAND_UNSPECIFIED = 0;
        COMMAND_ON = 1;
        COMMAND_OFF = 2;
    }
    Command command = 1;
    fixed64 request_id = 2;
}

// StateNotification: Safety Core → UI への状態通知
message StateNotification {
    enum LifecycleState {
        STATE_UNSPECIFIED = 0;
        STATE_INIT = 1;
        STATE_SELF_CHECK = 2;
        STATE_IDLE = 3;
        STATE_PRESCRIPTION_SET = 4;
        STATE_READY = 5;
        STATE_BEAM_ON = 6;
        STATE_HALTED = 7;
        STATE_ERROR = 8;
    }
    LifecycleState state = 1;
    fixed64 timestamp_unix_nano = 2;
}

// AlarmNotification: Safety Core → UI へのアラーム通知
message AlarmNotification {
    enum Severity {
        SEVERITY_UNSPECIFIED = 0;
        SEVERITY_CRITICAL = 1;
        SEVERITY_HIGH = 2;
        SEVERITY_MEDIUM = 3;
        SEVERITY_LOW = 4;
    }
    uint32 error_code = 1;          // ErrorCode の uint16_t 値を upcast
    Severity severity = 2;
    string detail_message = 3;      // SRS-UX-001: 現象 + 原因 + 推奨対処を含む
    fixed64 timestamp_unix_nano = 4;
}

// gRPC service
service SafetyCoreUiService {
    rpc SubmitPrescription(PrescriptionSet) returns (StateNotification);
    rpc SubmitBeamCommand(BeamCommand) returns (StateNotification);
    rpc StreamStateUpdates(stream StateNotification) returns (stream AlarmNotification);
}
```

#### IF-E-002: Safety Core ↔ Hardware Simulator

```protobuf
syntax = "proto3";
package th25.hwsim.v1;

message ElectronGunCommand { double current_ma = 1; }
message ElectronGunReport { double actual_current_ma = 1; }

message MagnetCommand { double current_a = 1; }
message MagnetReport { double actual_current_a = 1; }

message TurntableCommand { double target_mm = 1; }
message TurntableReport {
    double sensor_0_mm = 1;
    double sensor_1_mm = 2;
    double sensor_2_mm = 3;
}

message IonChamberReport {
    double channel_0_cgy = 1;
    double channel_1_cgy = 2;
}

// Fault injection (試験時のみ)
message FaultInjection {
    enum Target {
        TARGET_UNSPECIFIED = 0;
        TARGET_TURNTABLE_SENSOR_0 = 1;
        TARGET_TURNTABLE_SENSOR_1 = 2;
        TARGET_TURNTABLE_SENSOR_2 = 3;
        TARGET_ION_CHAMBER_0 = 4;
        TARGET_ION_CHAMBER_1 = 5;
    }
    Target target = 1;
    enum FaultMode {
        FAULT_UNSPECIFIED = 0;
        FAULT_STUCK_AT = 1;
        FAULT_DELAY = 2;
        FAULT_NO_RESPONSE = 3;
        FAULT_SATURATION = 4;
    }
    FaultMode mode = 2;
    double parameter = 3;  // StuckAt 値、Delay 秒数等
}

service HardwareSimulatorService {
    rpc SetElectronGun(ElectronGunCommand) returns (ElectronGunReport);
    rpc SetMagnet(MagnetCommand) returns (MagnetReport);
    rpc CommandTurntable(TurntableCommand) returns (TurntableReport);
    rpc StreamSensors(stream ElectronGunReport) returns (stream IonChamberReport);
    rpc InjectFault(FaultInjection) returns (FaultInjection);  // ack
}
```

#### IF-E-003: AuditLogger → Filesystem

JSON Lines 形式、各行 1 エントリ。スキーマは下記のとおり。

```json
{
  "timestamp": "2026-04-26T07:25:26.123456Z",
  "event_type": "MODE_CHANGE_REQUEST",
  "lifecycle_state": "PrescriptionSet",
  "fields": {
    "request_id": 42,
    "new_mode": "Electron",
    "energy_mev": 10.0
  }
}
```

ファイル書込: `open(path, O_WRONLY | O_APPEND | O_CREAT, 0600)` + 各エントリ書込後 `fdatasync(fd)`。

#### IF-E-004: Operator UI ↔ OS 入出力

Inc.4 で確定(候補: Dear ImGui + GLFW、または curses ベース TUI)。本 v0.1 では IF 仕様未確定。

## 6. 共通型・データ構造の詳細設計

### 6.1 `LifecycleState` 状態機械(SafetyCoreOrchestrator UNIT-201 内)

§4.2 §「`LifecycleState` 状態機械」参照。本節では遷移許可表を表形式で示す。

| 現在状態 | イベント | 次状態 | 備考 |
|---------|--------|-------|------|
| `Init` | `init_subsystems()` 開始 | `SelfCheck` | StartupSelfCheck 起動 |
| `SelfCheck` | 4 項目全 Pass | `Idle` | 治療開始準備完了 |
| `SelfCheck` | いずれか Fail | `Error` → 即 `shutdown()` | 致死的、再起動が必要 |
| `Idle` | `PrescriptionSet` メッセージ受信 | `PrescriptionSet` | パラメータ未確定 |
| `PrescriptionSet` | パラメータ全項目 validated | `Ready` | UNIT-202/204 から完了通知 |
| `PrescriptionSet` | `ResetPrescription` メッセージ | `Idle` | 操作者キャンセル |
| `Ready` | `BeamOnRequest`(許可フラグ true) | `BeamOn` | UNIT-203 が照射開始 |
| `Ready` | `ResetPrescription` | `Idle` | キャンセル |
| `BeamOn` | `BeamOff` 完了通知 | `Idle` | 正常終了 |
| `BeamOn` | `DoseTargetReached` | `Idle` | 目標到達 |
| `BeamOn` | Critical alarm | `Halted` | 致死的、操作者の物理的再起動が必要 |
| `Halted` | (なし) | (`shutdown()` のみ) | 物理的再起動以外の遷移なし |

**不正遷移の処理:** `transition_to(LifecycleState target)` 関数は上記許可表をハードコードし、許可されていない遷移は `ModeInvalidTransition` ErrorCode を `Result::error()` で返却 + `Halted` 遷移。

### 6.2 `enum class ErrorCode` 階層

§4.1 §4.1.1 参照(8 系統 × 各系統内 4〜6 件、上位 8 ビットで系統判定可能な整列)。

### 6.3 強い型(Strong Type)実装方針

UNIT-200 で定義する強い型は、すべて以下の共通パターン:

```cpp
class Energy_MeV {
public:
    explicit Energy_MeV(double value) : value_(value) {}
    double value() const noexcept { return value_; }
    auto operator<=>(const Energy_MeV&) const = default;
    bool operator==(const Energy_MeV&) const = default;
private:
    double value_;
};
```

**演算子オーバーロード方針:**
- `operator<=>` および `operator==`: 提供(比較は単位混在の危険なし)
- `operator+` / `operator-` / `operator*` / `operator/`: **非提供**(単位混在の危険あり、明示的変換関数のみ提供)
- 暗黙変換コンストラクタ: **禁止**(`explicit` 必須)
- 算術が必要な場面では `add_dose(DoseUnit_cGy)` のような明示的メンバ関数を提供

### 6.4 `Result<T, E>` 型

C++23 `std::expected` 互換 IF を C++20 で実装(C++20 範囲のみ使用、SOUP-001/002 の機能要求と整合)。`std::variant<T, E>` ベース。

```cpp
template<typename T, typename E = ErrorCode>
class Result {
public:
    Result(T value) : storage_(std::move(value)) {}
    Result(E error) : storage_(std::move(error)) {}

    bool has_value() const noexcept { return storage_.index() == 0; }
    explicit operator bool() const noexcept { return has_value(); }

    const T& value() const& { return std::get<T>(storage_); }
    T& value() & { return std::get<T>(storage_); }

    const E& error() const& { return std::get<E>(storage_); }

private:
    std::variant<T, E> storage_;
};
```

### 6.5 構成定義(校正値・閾値)

以下の値はコードに直接埋め込まず、構成定義として外部化する(HZ-007 構造的予防):

| 名称 | 型 | 値 | 用途 |
|------|---|-----|------|
| `DoseRatePerPulse_cGy_per_pulse` | `double` | TBD(Step 17+ で校正試験により決定) | UNIT-204 `pulse_count_to_dose()` |
| `EnergyMagnetMap` | テーブル(Energy_MeV → MagnetCurrent_A) | TBD(Step 17+ 校正データ) | UNIT-206 `set_current_for_energy()` |
| `TurntablePositions` | `{ Light: -50.0, Electron: 0.0, XRay: 50.0 } mm` | 固定(SRS-006 と整合) | UNIT-205 |
| `TurntableTolerance` | `Position_mm{0.5}` | 固定(SRS-006) | UNIT-205 |
| `BeamOffMaxDelay` | `std::chrono::milliseconds{10}` | 固定(SRS-101) | UNIT-203 ウォッチドッグ |
| `MessageBusCapacity` | `size_t = 4096` | 固定(本 SDD §4.13 で根拠) | UNIT-401 |

構成定義ファイル形式は TOML を採用予定(Step 17+ で確定)。CIL に `CI-CFG-016 calibration.toml` として追加予定(Inc.1 ベースライン時)。

### 6.6 `MessageBus` SPSC/MPSC 選定

- **Inc.1 範囲:** 全 IF が SPSC で十分(UNIT-201 → 各 Manager は 1:1)。`folly::ProducerConsumerQueue` を採用。
- **Inc.2 で MPSC 検討:** UNIT-207 SafetyMonitor が複数の Sim プロセスから通知を受信する場面では MPSC が必要。`boost::lockfree::queue` または独自実装(本 SDD §10 未確定事項)。

## 7. SOUP の使用箇所(UNIT レベル明示、HZ-007 影響評価支援)

各 SOUP の使用箇所を UNIT 単位で明示する。コンパイラ・標準ライブラリ更新時(SCMP §4.1 重大区分)に「どの UNIT のどの API が影響を受けるか」を即座に特定可能とする。

| SOUP ID | 使用 UNIT | 主な使用 API / 機能 | HZ-007 影響度 |
|---------|---------|------------------|------------|
| SOUP-001/002 | 全 UNIT | C++20 全機能(`enum class`、`std::atomic`、`std::variant`、`std::optional`、`std::thread`、`std::chrono`) | **高**(コンパイラ更新時は全 UNIT 再ビルド + 全試験再実行必須) |
| SOUP-003/004 | 全 UNIT | C++ 標準ライブラリ全般 | **高**(同上) |
| SOUP-005/006 | (ビルド時のみ) | CMake / Ninja | 中(ビルドシステム更新時は CI で確認) |
| SOUP-007 | (ビルド時のみ) | vcpkg(GoogleTest 取得、folly 取得予定) | 中(`builtin-baseline` で固定済) |
| SOUP-008/009 | tests/ 配下(全 UNIT の UT) | GoogleTest / GoogleMock | 低(試験フレームワークのため、機能検証が試験で担保) |
| SOUP-010 | (静的解析時のみ、全 UNIT) | clang-tidy `bugprone-*` / `cert-*` / `concurrency-*` 等 | 中(ルール更新時は全 UNIT 再解析、警告に対応必要) |
| SOUP-013 | 全 UNIT(`asan` プリセット) | AddressSanitizer ランタイム | 中 |
| SOUP-014 | UNIT-204(`PulseCounter` の整数演算)、その他全 UNIT | UndefinedBehaviorSanitizer ランタイム | **高**(HZ-003 直接対応) |
| **SOUP-015** | **UNIT-201/204/207/401(共有状態 + lock-free queue)、その他全 UNIT** | **ThreadSanitizer ランタイム** | **最高**(HZ-002 直接対応) |
| (予定) folly::ProducerConsumerQueue | UNIT-401 | SPSC lock-free queue 実装 | **高**(更新時は MessageBus 動作の再検証必須、本 SDD §10 で正式 SOUP 登録予定) |
| (予定) gRPC + Protocol Buffers | UNIT-402 | IPC | 中(`vcpkg.json` で固定予定) |

> **本 SDD §10 で SOUP 追加予定:** folly(UNIT-401)、gRPC + Protocol Buffers(UNIT-402)、TOML パーサ(構成定義読込)。Step 17+ コード実装時に CIL §5 へ追加昇格(SCMP §4.1 重大区分の別 CR で対応)。

## 8. 詳細設計の検証(箇条 5.4.4 ― クラス C)

詳細設計は以下の観点で設計レビュー(本書セルフレビュー、CCB §5.4 1 分インターバル後)で検証する。

- [x] **アーキテクチャ設計で定義された制約・インタフェースを実装している:** §3 で全 ARCH-NNN.x を UNIT-NNN に分解、§4 で各 UNIT の公開 API を SAD §5 IF-U-001〜010 / IF-E-001〜004 に整合
- [x] **SRS の要求事項を実装可能な形で具体化している:** §9 トレーサビリティで全 SRS 要求を UNIT に割付、§4 で関数シグネチャ・データ構造・状態遷移を実装可能粒度で記述
- [x] **リスクコントロール手段を正しく実現している:** §4.1〜§4.14 で各 RCM の実装方針を明示(RCM-001 = `enum class TreatmentMode` + 状態機械、RCM-003 = `std::atomic<uint64_t>` + `PulseCounter`、RCM-008 = 強い型 + 明示的変換、RCM-017 = ウォッチドッグ + < 10 ms 保証、RCM-018 = SEP-001 実装 = UNIT-402、RCM-019 = SEP-003 実装 = UNIT-401 + メッセージパッシング)
- [x] **ソフトウェアユニット単位で試験可能に記述されている:** 各 UNIT の公開 API は Result/optional/値型のみで構成、依存性注入(`IElectronGun` 等の抽象 IF)で GoogleMock によるモック差替え可能。**MISRA C++ + clang-tidy で機械的に副作用を制限**
- [x] **異常系・境界条件が網羅的に定義されている:** §4 各 UNIT の「例外・異常系の扱い」表 + §6.2 ErrorCode 階層(8 系統 × 4〜6 件 = 計 30 件以上)
- [x] **資源制約(スタック、実行時間等)が守られる設計となっている:** §4 各 UNIT の「資源使用量・タイミング制約」、SRS-101(< 10 ms)/ SRS-102(< 500 ms)/ SRS-103(≧ 1 kHz)/ SRS-104(≧ 100 Hz)を満たす設計を確定
- [x] **Therac-25 事故主要因類型(SPRP §4.3.1 A〜F)に対応する詳細設計が示されている:** §4.5 DoseManager(B 整数オーバフロー構造的排除)、§6.6 MessageBus(A race condition)、§4.10 AuditLogger 空殻 + §4.13/4.14 lock-free queue(C 共有状態回避)、§4.8 SafetyMonitor(D インターロック)、§6.2 ErrorCode 階層 + Severity(E バイパス UI 阻止)、§7 SOUP 使用箇所明示(F レガシー前提喪失予防)
- [x] **新規ハザードが識別された場合は RMF を更新する:** 本 SDD 作成中に **新規ハザードは識別されなかった**(§8.1 参照)

### 8.1 SDD 作成中に識別したリスク再評価

SDD 作成中に **新規ハザードは識別されなかった**。RMF-TH25-001 v0.1 への追加は不要。

ただし、以下の点は Step 17+ コード実装時に再評価する:

- **`folly::ProducerConsumerQueue` の境界値挙動:** queue 容量超過時の挙動(本 SDD §4.13 では `try_publish` が false 返却、`publish_blocking` は 5 秒タイムアウトで例外)が安全側に振れることを Inc.1 IT で確認
- **gRPC over UDS のメッセージ欠落・破損挙動:** UNIT-402 のエラー処理(`IpcChannelClosed` / `IpcDeserializationFailure`)が fail-safe(ビームオフ + アラーム)に正しく接続されることを Inc.1 IT で確認
- **`std::chrono::system_clock` の単調性:** AuditLogger でのタイムスタンプは `system_clock` を使用するが、システム時刻調整(NTP 同期等)で逆行する可能性あり。append-only 性は維持されるが、ログ順序保証は別途検討(Inc.4 で確定)

### 8.2 レビュー記録

| 項目 | 結果 | レビュー日 | 記録 ID |
|------|------|----------|---------|
| 本 SDD v0.1 全体(セルフレビュー) | 承認(全 §1〜§10 整合確認、§4 全 UNIT が SAD §4.3 ARCH-NNN.x に対応、§5 全 IF が SAD §5 IF-U/IF-E に対応、§9 トレーサビリティが SRS 全要求をカバー、§7 SOUP 使用箇所が CIL §5 13 件 + 追加予定 3 件と整合) | 2026-04-26 | 本書 v0.1 のコミット SHA(本ステップコミット時に追記) |

## 9. トレーサビリティマトリクス

### 9.1 SRS ↔ ARCH ↔ UNIT ↔ UT(計画)対応

> **凡例:** UT 列は計画段階の ID(`UT-NNN`)。本 SDD ではユニット試験計画書 UTPR-TH25-001(Step 17+ で作成)への前提として ID のみ採番する。

| SRS ID | ARCH 項目(SAD) | UNIT(本 SDD) | UT(計画) | Inc 確定範囲 |
|--------|--------------|-------------|---------|------------|
| SRS-001 | ARCH-002.2 | UNIT-202 | UT-001 (TreatmentMode 型安全) | Inc.1 |
| SRS-002 | ARCH-002.2 + 002.5 + 002.6 | UNIT-202 + UNIT-205 + UNIT-206 | UT-002 (モード遷移シーケンス 6 ステップ網羅) | Inc.1 |
| SRS-003 | ARCH-002.2 + 002.5 + 002.3 | UNIT-202 + UNIT-205 + UNIT-203 | UT-003 (ビームオン時整合再確認 9 通り) | Inc.1 |
| SRS-004 | ARCH-002.2 | UNIT-202 + UNIT-200 | UT-004 (エネルギー範囲境界値: 0/1/25/26 MeV、4/5/25/26 MV) | Inc.1 |
| SRS-005 | ARCH-002.6 | UNIT-206 + UNIT-200 | UT-005 (`MagnetCurrent_A` 強い型、暗黙変換禁止) | Inc.1 |
| SRS-006 | ARCH-002.5 | UNIT-205 + UNIT-200 | UT-006 (`Position_mm` 偏差判定境界値: 0.49/0.50/0.51 mm) | Inc.1 |
| SRS-007 | ARCH-002.3 + 002.2 | UNIT-203 + UNIT-202 | UT-007 (許可フラグ + `compare_exchange`) | Inc.1 |
| SRS-008 | ARCH-002.4 | UNIT-204 + UNIT-200 | UT-008 (`DoseUnit_cGy` 範囲境界値: 0.005/0.01/10000/10001 cGy) | Inc.1 |
| SRS-009 | ARCH-002.4 | UNIT-204 + UNIT-200 (`PulseCounter`) | UT-009 (`std::atomic_uint64_t::is_lock_free()` 表明、`numeric_limits::max()` 検証) | Inc.1 |
| SRS-010 | ARCH-002.4 + 002.3 | UNIT-204 + UNIT-203 | UT-010 (目標到達 → BeamOff < 1 ms、10^4 サイクル) | Inc.1 |
| SRS-011 | ARCH-002.3 | UNIT-203 | UT-011 (BeamOff → 電子銃停止 < 10 ms、10^6 サイクル) | Inc.1 (IT-011 で本格測定) |
| SRS-012 | ARCH-002.8 + 002.1 | UNIT-208 + UNIT-201 | UT-012 (4 項目自己診断、各失敗シナリオ) | Inc.1 |
| SRS-I-001〜004 | ARCH-001.x + IF-E-001 | UNIT-101〜104(空殻、Inc.4)+ UNIT-402 | (Inc.4 で UT 追加) | Inc.1 (IF のみ) |
| SRS-I-005 | ARCH-003.3 + IF-E-002 + ARCH-002.5 | UNIT-303 + UNIT-402 + UNIT-205 | UT-105 (3 系統センサ集約、故障注入 9 通り) | Inc.1 |
| SRS-I-006 | ARCH-003.4 + IF-E-002 + ARCH-002.4 | UNIT-304 + UNIT-402 + UNIT-204 | UT-106 (1 kHz 取得レート測定) | Inc.1 (IT-103) |
| SRS-I-007 | ARCH-003.2 + IF-E-002 + ARCH-002.6 | UNIT-302 + UNIT-402 + UNIT-206 | UT-107 (磁石電流取得、偏差判定) | Inc.1 |
| SRS-I-008 | ARCH-003.1 + IF-E-002 + ARCH-002.3 | UNIT-301 + UNIT-402 + UNIT-203 | UT-108 (電子銃電流取得、停止確認) | Inc.1 |
| SRS-O-001 | ARCH-002.3 + IF-E-002 + ARCH-003.1 | UNIT-203 + UNIT-402 + UNIT-301 | UT-201 (電子銃電流指令) | Inc.1 |
| SRS-O-002 | ARCH-002.6 + IF-E-002 + ARCH-003.2 | UNIT-206 + UNIT-402 + UNIT-302 | UT-202 (磁石電流指令) | Inc.1 |
| SRS-O-003 | ARCH-002.5 + IF-E-002 + ARCH-003.3 | UNIT-205 + UNIT-402 + UNIT-303 | UT-203 (ターンテーブル位置指令) | Inc.1 |
| SRS-O-004 | ARCH-001.3 + IF-E-001 | UNIT-103 (空殻) + UNIT-402 | (Inc.4) | Inc.1 (IF のみ) |
| SRS-O-005 | ARCH-002.9 + IF-E-003 | UNIT-209 (空殻) | UT-209 (空殻動作: stderr 出力確認) | Inc.1 (空殻) / Inc.4 (永続化) |
| SRS-IF-001 | ARCH-003.1 | UNIT-301 | UT-301 (`IElectronGun` 抽象 IF + Mock) | Inc.1 |
| SRS-IF-002 | ARCH-003.2 | UNIT-302 | UT-302 (`IBendingMagnet`) | Inc.1 |
| SRS-IF-003 | ARCH-003.3 | UNIT-303 | UT-303 (`ITurntable` + 故障注入) | Inc.1 |
| SRS-IF-004 | ARCH-003.4 | UNIT-304 | UT-304 (`IIonChamber` + サチュレーション/片系失陥) | Inc.1 |
| SRS-IF-005 | IF-E-001 + SEP-001 + SEP-003 + ARCH-004.2 | UNIT-402 | IT-401 (プロセス分離検証 strace、UDS 通信のみ) | Inc.1 |
| SRS-ALM-001〜005 | ARCH-002.7 + 各 Manager | UNIT-207 (空殻) + UNIT-202/205/206 等 | (Inc.2 で本格 UT) | Inc.1 (IF のみ) |
| SRS-SEC-001/002 | ARCH-002.10 + ARCH-001.4 + IF-E-001 | UNIT-210 (空殻) + UNIT-104 (空殻) + UNIT-402 | (Inc.4 で本格 UT) | Inc.1 (IF のみ) |
| SRS-UX-001/002/003 | ARCH-001.x + ARCH-002 共通 | UNIT-103 (空殻) + UNIT-200 (`ErrorCode` 階層) + UNIT-201 (`Halted` 遷移) | UT-uxX (Inc.4) | Inc.1 (構造的前提のみ) |
| SRS-D-001〜010 | ARCH-002 共通 | UNIT-200(`enum class TreatmentMode` / 強い型 8 種 / `PulseCounter` / `std::atomic`) | UT-d001〜d010 (各型単体試験) | Inc.1 |
| **SRS-D-009** | **§9 SEP-003 + clang-tidy** | **UNIT-401(MessageBus 経由のみ通信)+ clang-tidy ルール群で機械的検出** | **clang-tidy CI ジョブ Pass(警告 0)+ TSan CI 全 Pass** | **Inc.1**(構造的確定) |
| **SRS-RCM-001** | **ARCH-002.2** | **UNIT-202** | **UT-001 / UT-002 / UT-003** | **Inc.1** |
| **SRS-RCM-005** | **ARCH-002.5 + ARCH-003.3** | **UNIT-205 + UNIT-303** | **UT-006 / UT-105 / IT-005** | **Inc.1**(3 系統センサ部分)/ Inc.2(完成) |
| **SRS-RCM-008** | **共通型ライブラリ + ARCH-002.{2,4,6}** | **UNIT-200 + UNIT-202 + UNIT-204 + UNIT-206** | **UT-004 / UT-005 / UT-008 / UT-009 / UT-d001〜d010** | **Inc.1** |
| **SRS-RCM-017** | **ARCH-002.3** | **UNIT-203** | **UT-011 / IT-011** | **Inc.1** |
| **SRS-RCM-018** | **§9 SEP-001 + IF-E-001 + ARCH-001 / ARCH-002 プロセス分離** | **UNIT-402(InterProcessChannel 実装)+ プロセス起動順序** | **IT-401(プロセス分離 strace 検証)** | **Inc.1**(構造的確定) |
| **SRS-RCM-019** | **§9 SEP-003 + ARCH-004 + IF-U-001〜010 全体** | **UNIT-401 + UNIT-200(`PulseCounter`)+ 全 UNIT のメッセージパッシング設計** | **UT-401(`is_lock_free()` 表明、容量超過時境界値) + TSan CI 全 Pass + clang-tidy 警告 0** | **Inc.1**(構造的確定) |
| SRS-101 | (全層) | UNIT-203 + UNIT-402 + UNIT-301 | IT-101 (10^6 サイクル測定、最悪値・p95・平均) | Inc.1 |
| SRS-102 | (全層) | UNIT-202 + UNIT-205 + UNIT-206 | IT-102 (典型 + 最悪) | Inc.1 |
| SRS-103 | UNIT-304 + UNIT-402 + UNIT-204 | UT-106 / IT-103 | (測定) | Inc.1 |
| SRS-104 | UNIT-303 + UNIT-402 + UNIT-205 | UT-105 / IT-104 | (測定) | Inc.1 |

### 9.2 RCM ↔ UNIT ↔ 検証手段(SRMP §6.1 / §7.1 と整合)

| RCM ID | 実装 UNIT(本 SDD) | 検証手段 | Inc 確定範囲 |
|--------|-----------------|--------|------------|
| RCM-001 | UNIT-202 + UNIT-201 (`LifecycleState`) | UT-002 + 設計レビュー | Inc.1 |
| RCM-002 | UNIT-401 (`std::atomic` + memory_order) + UNIT-204 (`PulseCounter`) | UT-401 + TSan + モデル検査(Inc.3) | Inc.1 (atomic 部分) / Inc.3 (完全) |
| RCM-003 | UNIT-204 (`PulseCounter` + `std::atomic<uint64_t>`) | UT-009 (`numeric_limits::max()` 検証) + UBSan | Inc.1 |
| RCM-004 | 全 UNIT (clang-tidy `concurrency-*` ルール群) + SDD §6.6 | clang-tidy CI Pass(警告 0)+ 設計レビュー + TSan | Inc.1 (機械検証手段確立) |
| RCM-005 | UNIT-205 (3 系統センサ集約 + 偏差判定) + UNIT-303 (故障注入) | UT-006 + UT-105 + IT-005 | Inc.1 (3 系統部分) / Inc.2 (モデル予測追加) |
| RCM-006 | UNIT-207 (Inc.2 で本格化) | (Inc.2 で IT 追加) | Inc.2 |
| RCM-007 | UNIT-304 (2 組比較) + UNIT-207 (Inc.2) | (Inc.2 で UT/IT 追加) | Inc.2 |
| RCM-008 | UNIT-200 (強い型 8 種) + UNIT-202/204/206 | UT-004 / UT-005 / UT-008 / UT-d001〜d010 + libFuzzer (Step 17+) | Inc.1 |
| RCM-009 | UNIT-200 (`ErrorCode` 階層) + UNIT-103 (Inc.4) | (Inc.4 ST、IEC 62366-1 妥当性確認) | Inc.4 (UI) / Inc.1 (型レベル基盤) |
| RCM-010 | UNIT-200 (`Severity` enum) + UNIT-201 (`Halted` 遷移) + UNIT-102 (Inc.4) | (Inc.4 ST、バイパス試行拒否確認) | Inc.4 (UI) / Inc.1 (型レベル基盤) |
| RCM-011 | UNIT-209 (Inc.4 で本格化) | (Inc.4 UT/ST、append-only/完全性) | Inc.4 |
| RCM-012 | 全 UNIT(SOUP 使用箇所文書化、本 SDD §7) | コードレビュー + コンパイラ更新時の HZ-007 評価記録 | Inc.1 |
| RCM-013 | UNIT-208 + UNIT-201 (`init_subsystems()`) | UT-012 + IT-201 (再起動シナリオ) | Inc.1 |
| RCM-014 | UNIT-201 (Inc.4 で電源喪失復旧シナリオ追加) | (Inc.4 ST) | Inc.4 |
| RCM-015 | UNIT-210 (Inc.4 で本格化) | (Inc.4 UT/ST) | Inc.4 |
| RCM-016 | UNIT-209 (Inc.4 で processVariableLogger 追加) | (Inc.4 UT) | Inc.4 |
| RCM-017 | UNIT-203 (ウォッチドッグ + < 10 ms 保証) | UT-011 + IT-011 | Inc.1 |
| **RCM-018** | **UNIT-402 (InterProcessChannel) + プロセス起動順序 + SAD §9 SEP-001** | **設計レビュー(本 SDD §10 自己レビュー済) + IT-401 (strace、プロセス分離 OS 機能検証) + TSan** | **Inc.1**(構造的確定) |
| **RCM-019** | **UNIT-401 (InProcessQueue) + UNIT-200 (`PulseCounter`) + 全 UNIT のメッセージパッシング設計** | **設計レビュー + UT-401 (`is_lock_free()` + 容量超過境界値) + TSan + モデル検査(Inc.3)** | **Inc.1**(構造的確定) |

## 10. 残された未確定事項(Step 17+ コード実装時に確定)

| 未確定事項 | 確定先 | 暫定方針(本 SDD で記述) |
|---------|------|------------------------|
| MessageBus MPSC 実装(SafetyMonitor 周り) | Inc.2 SDD 改訂(SDD v0.2) | `boost::lockfree::queue` 候補。または folly MPSC 実装 |
| Protocol Buffers ライブラリのバージョン | Step 17+(`vcpkg.json` 追加) | gRPC `1.60+`、protobuf `25.x`(vcpkg ベースライン commit と整合) |
| folly ライブラリの `vcpkg.json` 追加 | Step 17+ | folly 全体ではなく `folly::ProducerConsumerQueue` のみ部分依存(または独自実装の選択肢も検討) |
| TOML パーサライブラリ(構成定義読込) | Step 17+(`vcpkg.json` 追加) | `toml++`(C++17/20)候補、または `tomlplusplus` |
| `EnergyMagnetMap` の校正値 | Step 17+ 校正試験 | 線形補間、SRS-005 と整合 |
| `DoseRatePerPulse_cGy_per_pulse` | Step 17+ 校正試験 | UNIT-204 で構成定義として注入 |
| Inc.4 UI フレームワーク | SDD v0.4(Inc.4) | Dear ImGui + GLFW / curses TUI 候補 |

## 11. 改訂履歴

| バージョン | 日付 | 変更内容 | 変更者 |
|----------|------|---------|--------|
| 0.1 | 2026-04-26 | 初版作成(Step 16 / CR-0004、本プロジェクト Issue #4)。**Inc.1 範囲(ビーム生成・モード制御コア)** の詳細設計をユニットレベルで確定し、`inc1-design-frozen` タグ(BL-20260426-002)で凍結。**§3 ユニットへの分解(IEC 62304 §5.4.1):** ARCH-NNN.x を UNIT-NNN(計 22 ユニット: Inc.1 中核 14、Inc.2 空殻 1、Inc.4 空殻 7)まで分解。**§4 各ユニットの詳細設計(IEC 62304 §5.4.2 クラス C 必須):** 全 UNIT について公開 API・データ構造・アルゴリズム/状態遷移・資源使用量/タイミング制約・例外/異常系の扱いを記述。中核は UNIT-200 CommonTypes(強い型 8 種 + `enum class` 4 種 + `Result<T,E>`)、UNIT-201 SafetyCoreOrchestrator(8 状態の `LifecycleState` 状態機械)、UNIT-204 DoseManager(`std::atomic<uint64_t>` + `PulseCounter` で HZ-003 構造的排除)、UNIT-401 InProcessQueue(`folly::ProducerConsumerQueue` SPSC、SRS-RCM-019 構造的実体)、UNIT-402 InterProcessChannel(gRPC over UDS、RCM-018 実装層)。**§5 IF 詳細設計(IEC 62304 §5.4.3 クラス C 必須):** IF-U-001〜010 のメッセージ型 struct 詳細、IF-E-001〜003 の Protocol Buffers スキーマ(proto3、Operator UI / Hardware Simulator / AuditLogger)を確定。**§6 共通型・データ構造:** `LifecycleState` 状態機械遷移許可表、`enum class ErrorCode` 階層(8 系統 × 4〜6 件、上位 8 ビットで系統判定)、強い型実装方針、`Result<T,E>` 実装、構成定義(校正値・閾値の外部化、HZ-007 構造的予防)、MessageBus SPSC/MPSC 選定。**§7 SOUP 使用箇所の UNIT レベル明示:** 13 件の SOUP + 追加予定 3 件(folly / gRPC+protobuf / TOML)を UNIT に割付、HZ-007 影響度を明示。**§8 詳細設計の検証(IEC 62304 §5.4.4 クラス C 必須):** 7 項目チェックリストを全クリア。**§9 トレーサビリティ:** SRS 全要求 → ARCH → UNIT → UT(計画 ID)を双方向で構築、RCM ↔ UNIT ↔ 検証手段の対応表(SRMP §6.1 / §7.1 と整合)。**Therac-25 学習目的の中核:** Tyler 第 1 例 race condition は SAD §9 で構造的に不可能化済の上で、本 SDD では §4.13 UNIT-401 + §6.6 MessageBus 設計で「メッセージパッシングの境界値挙動」を明示的に設計し、Inc.3 RCM-002 の lock-free queue 動作試験 + TSan 全 Pass + モデル検査の 3 層検証(SRMP §6.1)の前提を確立。Yakima Class3 オーバフローは §4.5 UNIT-204 + §6.3 強い型 `PulseCounter` で構造的に発生不可能化(`uint64_t` + atomic + 5,800 万年連続照射相当の余裕)。本 SDD 完了をもって `inc1-design-frozen` タグを Step 16 本体コミットに付与し、Inc.1 設計を不可逆に凍結 | k-abe |
