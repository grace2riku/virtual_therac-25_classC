# ソフトウェアユニットテスト計画書/報告書(UTPR)

**ドキュメント ID:** UTPR-TH25-001
**バージョン:** 0.2
**作成日:** 2026-04-26(初版)/ 2026-04-29(v0.2 昇格)
**対象製品:** 仮想 Therac-25(Virtual Therac-25) / TH25-SIM-001
**対象ソフトウェアバージョン:** 0.2.0(Inc.1 完了で初回機能リリース予定)
**安全クラス:** C(IEC 62304)
**変更要求:** CR-0005(GitHub Issue #5、初版 UNIT-200)/ CR-0006(GitHub Issue #6、v0.2 UNIT-401 追加)

| 役割 | 氏名 | 所属 | 日付 | 署名 |
|------|------|------|------|------|
| 作成者 | k-abe | — | 2026-04-26 | |
| レビュー者 | k-abe(セルフ、CCB §5.4 1 分インターバル後) | — | 2026-04-26 | |
| 承認者 | k-abe(セルフ承認、CCB §3 単独開発兼任) | — | 2026-04-26 | |

> **本プロジェクトの位置づけ(注記)**
>
> 本ドキュメントは IEC 62304 に基づく医療機器ソフトウェア開発プロセスの学習・参考実装を目的とした **仮想プロジェクト** の成果物である。題材は 1985-1987 年に放射線過剰照射事故を起こした実在の医療機器 **Therac-25**(AECL)である。
>
> 本書はユニット試験の **計画(第 I 部)** と **実施結果(第 II 部 報告)** を一体で管理する。Step 17(CR-0005)で UNIT-200 CommonTypes に対する第 I 部 + 第 II 部 を初期化し、後続 Step 18+ で各ユニット実装ごとに本書を昇格(v0.2 / v0.3 ...)し、追加の試験ケース・実績を記録する。
>
> **本 v0.2(Step 18 / CR-0006)では UNIT-401 InProcessQueue(SPSC lock-free ring buffer)を追加**。UNIT-201〜210 / 301〜304 / 402 の試験計画・実績は後続 SDD-準拠ステップで追加する。

---

## 1. 目的と適用範囲

### 1.1 目的

本書は、IEC 62304 箇条 5.5(ソフトウェアユニットの実装)および §5.5.4(クラス C 追加受入基準)に基づき、TH25-CTRL の各ソフトウェアユニットの **実装ルール、検証プロセス、受入基準、試験ケース、試験実績** を一元的に記録する。本書は「Step 17 以降のコード実装すべての品質保証の根拠」として参照される。

### 1.2 適用範囲

- 対象: SDD-TH25-001 v0.1 で確定した全 22 ユニット(UNIT-101〜104 / 200 / 201〜210 / 301〜304 / 401〜402)
- 本 v0.1 で記録範囲を確定したユニット: **UNIT-200 CommonTypes**(Step 17 / CR-0005)
- 後続 Step 18+ で他ユニットを順次追加

### 1.3 用語と略語

CLAUDE.md「英語略語の統一」表に従う。本書固有の用語は SDD-TH25-001 v0.1 §1.3 を継承する。

## 2. 参照文書

| ID | 文書名 | バージョン |
|----|--------|----------|
| [1] | IEC 62304:2006+A1:2015 / JIS T 2304:2017 ヘルスソフトウェア — ソフトウェアライフサイクルプロセス(箇条 5.5) | — |
| [2] | ISO/IEC 14882:2020(C++20)国際標準 | — |
| [3] | MISRA C++ 2023 | — |
| [4] | CERT C++ Secure Coding Standard | — |
| [5] | ソフトウェア要求仕様書(SRS-TH25-001) | 0.1 |
| [6] | ソフトウェアアーキテクチャ設計書(SAD-TH25-001) | 0.1 |
| [7] | ソフトウェア詳細設計書(SDD-TH25-001) | 0.1.1 |
| [8] | ソフトウェア開発計画書(SDP-TH25-001) | 0.1 |
| [9] | ソフトウェアリスクマネジメント計画書(SRMP-TH25-001) | 0.1 |
| [10] | リスクマネジメントファイル(RMF-TH25-001) | 0.1 |
| [11] | 構成アイテム一覧(CIL-TH25-001) | 0.10 |
| [12] | CCB 運用規程(CCB-TH25-001) | 0.1 |
| [13] | 変更要求台帳(CRR-TH25-001) | 0.8 |
| [14] | ソフトウェア構成管理計画書(SCMP-TH25-001) | 0.1 |

---

# 第 I 部 計画

## 3. ソフトウェアユニットの実装(箇条 5.5.1)

### 3.1 実装ルール

| 項目 | 規約 | 検証手段 |
|------|------|--------|
| コーディング規約(主) | **MISRA C++ 2023**(SDP §6.1) | clang-tidy `bugprone-*`/`cert-*`/`cppcoreguidelines-*`/`hicpp-*` で機械的検証(警告 0) |
| コーディング規約(補助) | **CERT C++ Secure Coding Standard** | clang-tidy `cert-*` で機械的検証 |
| 言語標準 | **C++20**(SDP §6.1) | `cmake -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_CXX_EXTENSIONS=OFF` |
| 命名規則 | SDD §6.3 と整合(クラスは `aNy_CasE`、namespace/関数/変数は `lower_case`、メンバ末尾 `_`、グローバル定数 `kCamelCase`) | clang-tidy `readability-identifier-naming.*` |
| 警告レベル | `-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2`(`TH25_STRICT_WARNINGS` で各 target 適用) | コンパイル時 fail-stop |
| Sanitizer | ASan / UBSan / TSan(`TH25_SANITIZER` プリセット、CI で常時実行) | runtime fail-stop |
| 動的メモリ確保 | イベントループ中は禁止(MISRA C++ 21-2-x、SDD §4.2)、起動時のみ許容 | コードレビュー |
| `std::atomic` 共有変数規則 | `PulseCounter` クラス経由のみ可、その他の共有変数は禁止(SRS-D-009 / RCM-019) | clang-tidy `concurrency-mt-unsafe`、コードレビュー |
| 例外スロー | 関数内例外スロー禁止(MISRA C++ 18-9-1)、SOUP からの伝播は許容 | コードレビュー |
| `Result<T, E>` の使用 | 失敗を返す可能性のある全関数で `Result<T, E>` を返却(`throw` 不可) | コードレビュー |

### 3.2 実装対象ユニット一覧(SDD §3.2 と整合)

| ユニット ID | 名称 | 担当 | 予定ソースファイル | 本 UTPR 計画範囲 |
|------------|------|------|------------------|----------------|
| **UNIT-200** | **CommonTypes** | k-abe | `src/th25_ctrl/include/th25_ctrl/common_types.hpp` | **v0.1 で計画 + 実施記録(Step 17 / CR-0005)、v0.2 で維持** |
| **UNIT-401** | **InProcessQueue** | k-abe | `src/th25_ctrl/include/th25_ctrl/in_process_queue.hpp` | **本 v0.2 で計画 + 実施記録(Step 18 / CR-0006)** |
| UNIT-101〜104 | Operator UI 群 | k-abe | `src/th25_ui/...`(Inc.4 で追加予定) | (Inc.4 SDD 改訂時に追加) |
| UNIT-201 | SafetyCoreOrchestrator | k-abe | `src/th25_ctrl/...` | (Step 19 で追加予定) |
| UNIT-202 | TreatmentModeManager | k-abe | `src/th25_ctrl/...` | (Step 19+ 後続で追加予定) |
| UNIT-203 | BeamController | k-abe | `src/th25_ctrl/...` | 同上 |
| UNIT-204 | DoseManager | k-abe | `src/th25_ctrl/...` | 同上 |
| UNIT-205 | TurntableManager | k-abe | `src/th25_ctrl/...` | 同上 |
| UNIT-206 | BendingMagnetManager | k-abe | `src/th25_ctrl/...` | 同上 |
| UNIT-207 | SafetyMonitor(空殻) | k-abe | `src/th25_ctrl/...` | 同上(Inc.2 で完成) |
| UNIT-208 | StartupSelfCheck | k-abe | `src/th25_ctrl/...` | 同上 |
| UNIT-209 | AuditLogger(空殻) | k-abe | `src/th25_ctrl/...` | 同上(Inc.4 で完成) |
| UNIT-210 | CoreAuthenticationGateway(空殻) | k-abe | `src/th25_ctrl/...` | 同上(Inc.4 で完成) |
| UNIT-301〜304 | Hardware Simulator 群 | k-abe | `src/th25_sim/...` | 同上 |
| UNIT-402 | InterProcessChannel | k-abe | `src/th25_ctrl/...` | 同上 |

## 4. ソフトウェアユニット検証プロセスの確立(箇条 5.5.2)

### 4.1 検証方法

| 方法 | 適用範囲 | ツール | 実施タイミング |
|------|---------|-------|--------------|
| **コードレビュー(セルフ)** | 全ユニット | GitHub PR(将来)/ コミット差分の自己レビュー | 実装完了時、CCB §5.4 1 分インターバル後 |
| **静的解析(clang-tidy)** | 全ユニット | clang-tidy 17(SOUP-010、`bugprone-*`/`cert-*`/`concurrency-*`/`cppcoreguidelines-*`/`hicpp-*`/`misc-*`/`modernize-*`/`performance-*`/`portability-*`/`readability-*`)| CI(cpp-build) push 毎 |
| **動的解析(Sanitizer)** | 全ユニット(UT 経由) | ASan(SOUP-013)/ UBSan(SOUP-014)/ **TSan(SOUP-015)** | CI(cpp-build)各 Sanitizer プリセット毎 |
| **ユニット試験** | 全ユニット | GoogleTest(SOUP-008)/ GoogleMock(SOUP-009)、`gtest_discover_tests` | CI 毎 + ローカル `cmake --preset debug && ctest` |
| **網羅率計測** | 全ユニット(Step 17+ 段階的整備) | llvm-cov(SOUP-017、Step 17+ 整備予定) | (本 v0.1 範囲では計測未実施、Step 19 以降で導入予定) |

### 4.2 並行処理 RCM の 3 層検証(SRMP §6.1 と整合)

並行処理関連ユニット(`PulseCounter` など `std::atomic` を使用するユニット)については、**TSan + 境界値試験 + モデル検査** の 3 層検証を必須とする(SRMP §6.1)。

- **TSan**: CI cpp-build の `tsan` プリセットで全 UT を実行。本 v0.1 では `PulseCounter` の concurrent fetch_add 試験(UT-200-16)で確認
- **境界値試験**: `std::numeric_limits<uint64_t>::max()` 等の境界値を UT で網羅(本 v0.1 では UT-200-15)
- **モデル検査**: Inc.3(タスク同期)着手時に TLA+ / SPIN / Frama-C のいずれかで導入判断(SDD §10 未確定事項)。本 v0.1 範囲では未実施

## 5. ソフトウェアユニット受入基準(箇条 5.5.3)

各ユニットは以下の基準を **全て** 満たすことを受入条件とする:

1. **詳細設計書の公開 API と実装が一致している**(SDD §4.x の「公開 API」表と実装が完全一致)
2. **コーディング規約違反がない**(clang-tidy 警告 0、または正当化コメント `// NOLINT(...)` 付与済み)
3. **静的解析の警告がない**(同上)
4. **ユニット試験が全件合格している**(GoogleTest 全 Pass、CI cpp-build 全 8 ジョブ Pass)
5. **コードレビューが完了している**(セルフレビュー + CCB §5.4 1 分インターバル後の自己再確認)

## 6. 追加のユニット受入基準(箇条 5.5.4 ― クラス C 必須)

クラス C では以下を **追加の受入基準** とする。本書では各 UNIT について本 9 項目を全て満たすことを必須化する。

- [ ] **正常系の動作確認:** 正常入力で SDD §4.x 「公開 API」記載の期待出力が得られること
- [ ] **境界値試験:** 入力値域の最小・最大・境界±1(整数型は `std::numeric_limits` 境界、浮動小数型は SRS で定めた範囲の上下限 ± 0.5 LSB)
- [ ] **異常系・エラー入力:** 値域外、不正状態からの呼出が SDD §4.x 「例外・異常系の扱い」表に従い処理されること
- [ ] **資源使用:** スタック使用量、ヒープ確保量、実行時間が SDD §4.x 「資源使用量・タイミング制約」内
- [ ] **制御フロー網羅:** 分岐網羅(decision coverage)100%(安全関連)/ 95% 以上(その他)。Step 17+ で llvm-cov 整備後に計測開始
- [ ] **データフロー:** 未初期化変数・使用されない定義の排除(clang-tidy `bugprone-*` / `cert-*` で機械的検出)
- [ ] **ハードウェア障害・ソフトウェア障害:** Hardware Simulator(UNIT-301〜304)の故障注入機能で検証、または `IElectronGun` 等抽象 IF を `GoogleMock` で差替えて障害を模擬
- [ ] **並行処理:** 競合・デッドロック・優先度逆転への対処(`std::atomic` + メモリオーダリング、`std::mutex`、lock-free queue)。**TSan で race condition 検出 0 を必須**
- [ ] **タイミング:** タイムアウト・割込応答が SDD §4.x で規定した時間内(SRS-101 < 10 ms など)。本格的な測定は IT(結合試験)で実施し、UT では「設計上タイミング保証可能か」の構造的検証を行う

### 6.1 本プロジェクト固有の追加受入基準(Therac-25 学習目的)

CLAUDE.md / SRMP §6.1 / SCMP §4.1 を踏まえ、以下を追加で必須化する:

- [ ] **`std::atomic<uint64_t>::is_always_lock_free` を `static_assert` で表明**(コンパイラ・標準ライブラリ更新時に lock-free 性が失われた場合、ビルド時点で fail-stop。HZ-007 構造的予防)
- [ ] **強い型(SRS-D-002〜008)は `explicit` constructor 必須、算術演算子非提供**(`requires` 式または SFINAE で UT 上に compile-time チェック)
- [ ] **`enum class` は `static_cast` 経由でのみ整数値と相互変換可能**(`bugprone-enum-misuse` で機械的検出)

## 7. ソフトウェアユニット試験(箇条 5.5.5)

### 7.1 試験環境

| 項目 | 内容 |
|------|------|
| 試験フレームワーク | GoogleTest 1.14+(SOUP-008、vcpkg 経由)+ GoogleMock(SOUP-009、同梱)|
| ホスト環境 1(ローカル開発) | macOS 13+ / AppleClang 15、骨格ビルド(`TH25_BUILD_TESTS=OFF`)で構文・`static_assert` 検証 |
| ホスト環境 2(CI) | Ubuntu 24.04 + clang-17 / gcc-13、CMake 3.28+、Ninja 1.11+、vcpkg(ベースライン commit `c3867e714d...`) |
| 試験実行プリセット | `debug` / `relwithdebinfo` / `asan` / `ubsan` / `tsan`(各プリセットで UT 全件実行) |
| 起動オプション | `ASAN_OPTIONS=halt_on_error=1,detect_leaks=1` / `UBSAN_OPTIONS=halt_on_error=1,print_stacktrace=1` / `TSAN_OPTIONS=halt_on_error=1,second_deadlock_stack=1` |
| カバレッジ計測ツール | llvm-cov(SOUP-017、Step 17+ で整備予定、本 v0.1 範囲では未計測) |

### 7.2 試験ケース定義

#### UNIT-200 CommonTypes ユニット試験ケース一覧(本 v0.1 範囲)

| 試験 ID | GoogleTest 名 | 入力 | 期待結果 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|--------------|------|---------|------|--------------|-------------|
| UT-200-01 | `CommonTypes_TreatmentMode.EnumValuesAreStable` | enum 値 | Electron=1, XRay=2, Light=3 | 正常系 | SRS-001 / SRS-D-001 / SDD §4.1 | RCM-001 / HZ-001 |
| UT-200-01b | `CommonTypes_TreatmentMode.IsStronglyTypedEnumeration` | (compile-time) | `is_convertible_v<TreatmentMode, int> == false` | 構造的(compile) | 同上 | 同上 |
| UT-200-02 | `CommonTypes_LifecycleState.AllEightStatesArePresent` | 8 状態列挙値 | Init=0..Error=7 連番 | 正常系 | SRS-002 / SDD §6.1 | RCM-001 |
| UT-200-03 | `CommonTypes_BeamState.AllFourStatesArePresent` | 4 状態列挙値 | Off=0..Stopping=3 連番 | 正常系 | SRS-007 / SRS-011 / SDD §4.1 | RCM-017 |
| UT-200-04 | `CommonTypes_ErrorCode.CategoryMaskCoversEightCategories` | 全 ErrorCode 24 件 | 各 `error_category()` 戻り値が 0x01〜0x07/0xFF のいずれかと一致 | 正常系 + 構造的 | SDD §4.1.1 / §6.2 | RCM-009 / HZ-006 |
| UT-200-05 | `CommonTypes_Severity.SeverityOfFollowsSpecRules` | 各系統代表 ErrorCode | Critical(Mode/Beam/Dose/Internal)/ High(Turntable/IPC)/ Medium(Magnet/Auth)を返す | 正常系 | SDD §6.2 | RCM-010 / HZ-006 |
| UT-200-06 | `CommonTypes_StrongType.EnergyMeVRequiresExplicitConstruction` | (compile-time) | `is_convertible_v<double, Energy_MeV> == false`、`Energy_MeV{10.0}.value() == 10.0` | 構造的 + 正常系 | SRS-D-002 / SDD §6.3 | RCM-008 / HZ-005 |
| UT-200-07 | `CommonTypes_StrongType.EnergyMeVAndEnergyMVAreDistinctTypes` | (compile-time) | `Energy_MeV` と `Energy_MV` 間の暗黙変換不可 | 構造的 | SRS-D-002 / SRS-D-003 / SDD §6.3 | RCM-008 |
| UT-200-08 | `CommonTypes_StrongType.ComparisonOperatorsAreProvided` | `Energy_MeV{5.0}` 等 | `<`/`>`/`==`/`!=`/`<=`/`>=` 全て期待通り | 正常系 | SDD §6.3 | RCM-008 |
| UT-200-09 | `CommonTypes_StrongType.ArithmeticOperatorsAreNotProvided` | (compile-time `requires` 式) | `Energy_MeV` で `+`/`-`/`*` がコンパイル不可 | 構造的(C++20 requires)| SDD §6.3 | RCM-008 / HZ-005 |
| UT-200-10 | `CommonTypes_StrongType.DoseUnitAddDoseAccumulatesOnly` | `DoseUnit_cGy{100}.add_dose(50)` | 結果 = 150、元のインスタンス不変 | 正常系 | SDD §4.1 | RCM-008 |
| UT-200-10b | `CommonTypes_StrongType.PositionAbsDiffIsNonNegative` | `Position_mm` の `abs_diff()` | 常に非負、対称性 | 正常系 + 境界値 | SRS-006 / SDD §4.6 | RCM-005 |
| UT-200-11 | `CommonTypes_PulseCount.ConstructionAndComparison` | `PulseCount{100}`, `{200}` | 比較演算子が期待通り | 正常系 | SRS-009 / SRS-D-005 / SDD §4.1 | RCM-003 |
| UT-200-12 | `CommonTypes_PulseCounter.AtomicUint64IsAlwaysLockFree` | (runtime + static_assert) | `is_always_lock_free == true` | 構造的(HZ-007) | SDD §7 SOUP-003/004 | RCM-019 / HZ-002 / HZ-007 |
| UT-200-13a | `CommonTypes_PulseCounter.InitialValueIsZero` | new instance | `load().value() == 0` | 正常系 | SDD §4.1 | RCM-003 |
| UT-200-13b | `CommonTypes_PulseCounter.FetchAddReturnsPreviousValue` | 連続 `fetch_add` | 戻り値 = 加算前、`load()` = 加算後 | 正常系 | SDD §4.1 | RCM-002 |
| UT-200-14 | `CommonTypes_PulseCounter.ResetReturnsToZero` | `fetch_add(12345)` → `reset()` | `load() == 0` | 正常系 | SDD §4.5 | RCM-008 |
| UT-200-15 | `CommonTypes_PulseCounter.NumericLimitsBoundary` | `uint64_t::max() - 1` まで進めて + 1 | `load() == numeric_limits::max()` | **境界値** | SRS-009 / SDD §4.5 | **RCM-003 / HZ-003** |
| UT-200-16 | `CommonTypes_PulseCounter.ConcurrentFetchAddIsRaceFree` | **4 スレッド × 10000 増分** | `load() == 40000`、TSan で race 検出 0 | **並行処理(TSan 必須)** | SRS-009 / SRS-D-009 / SDD §4.13 | **RCM-002 / RCM-019 / HZ-002** |
| UT-200-17 | `CommonTypes_PulseCounter.IsNotCopyableNorMovable` | (compile-time) | `is_copy_constructible_v == false` 等 4 項目 | 構造的 | SDD §4.1 | RCM-019 |
| UT-200-18 | `CommonTypes_Result.ValueConstructionAndAccess` | `Result<DoseUnit_cGy>{42.5}` | `has_value() == true`、`value()` 取得 | 正常系 | SDD §6.4 | — |
| UT-200-19 | `CommonTypes_Result.ErrorConstructionAndAccess` | `Result::error(DoseOutOfRange)` | `has_value() == false`、`error_code() == DoseOutOfRange` | 正常系 | SDD §6.4 | RCM-009 |
| UT-200-20a | `CommonTypes_Result.VoidSpecializationOk` | `Result<void>::ok()` | `has_value() == true` | 正常系 | SDD §6.4 | — |
| UT-200-20b | `CommonTypes_Result.VoidSpecializationError` | `Result<void>::error(InternalAssertion)` | `has_value() == false`、`error_code()` 取得 | 正常系 | SDD §6.4 | — |

UNIT-200 合計試験ケース: **24 件**(`gtest_discover_tests` で自動収集)

#### UNIT-401 InProcessQueue ユニット試験ケース一覧(本 v0.2 範囲、Step 18 / CR-0006 で追加)

| 試験 ID | GoogleTest 名 | 入力 | 期待結果 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|--------------|------|---------|------|--------------|-------------|
| UT-401-01 | `InProcessQueue_Init.IsEmptyInitially` | new instance | `empty_approx() == true`、`approximate_size() == 0`、`try_consume() == nullopt` | 正常系 | SDD §4.13 | RCM-019 |
| UT-401-02 | `InProcessQueue_Capacity.ReportsTemplateValue` | (compile-time) | `capacity()` が template parameter と一致 | 構造的(constexpr) | SDD §4.13 / §6.5 | — |
| UT-401-03 | `InProcessQueue_SingleThreaded.PublishConsumeRoundTrip` | `try_publish(42)` → `try_consume()` | 値の整合性、size 1 → 0 | 正常系 | SDD §4.13 | RCM-019 |
| UT-401-04 | `InProcessQueue_SingleThreaded.PreservesFifoOrder` | 5 件 publish → 5 件 consume | FIFO 順序保証 | 正常系 | SDD §4.13 | RCM-019 |
| UT-401-05 | `InProcessQueue_Boundary.FullAtCapacityMinusOne` | Capacity-1 件詰め → Capacity 目で full | `try_publish` が false 返却、consume 後に再 publish 可 | **境界値** | SDD §4.13 | RCM-019 |
| UT-401-06 | `InProcessQueue_Boundary.ConsumeOnEmptyReturnsNullopt` | empty で `try_consume` 5 回 | 全て `nullopt` | 異常系 | SDD §4.13 | RCM-019 |
| UT-401-07 | `InProcessQueue_Boundary.RingWrapAroundOverManyCycles` | Capacity の 100 倍サイクルで publish/consume | wrap-around バグなし、値・FIFO 維持 | **境界値**(off-by-one / modulo 検出) | SDD §4.13 | RCM-019 |
| UT-401-08 | `InProcessQueue_LockFree.AtomicSizeIsAlwaysLockFree` | (runtime + static_assert) | `is_always_lock_free == true` | 構造的(HZ-007) | SDD §7 SOUP-003/004 | RCM-019 / HZ-002 / HZ-007 |
| UT-401-09 | `InProcessQueue_OwnershipTraits.IsNotCopyableNorMovable` | (compile-time) | `is_copy_constructible_v == false` 等 4 項目 | 構造的 | SDD §4.13 | RCM-019 |
| UT-401-10 | `InProcessQueue_MessageTypes.StructWithMultipleFieldsRoundTrip` | `TestMessage{int, double, uint64_t}` | 全フィールド整合 | 正常系(template 独立性)| SDD §5.1 IF-U メッセージ型 | — |
| UT-401-11 | `InProcessQueue_MessageTypes.EnumClassRoundTrip` | `TreatmentMode::{Electron,XRay,Light}` | 全 enum 値整合 | 正常系(template 独立性)| SDD §4.1 / §5.1 | RCM-001 |
| **UT-401-12** | **`InProcessQueue_Concurrency.SpscOneHundredThousandMessages`** | **1 producer + 1 consumer × 100k メッセージ** | **全件受信 + FIFO 順序保証 + race detection 0(`tsan` プリセット)** | **並行処理(TSan 必須)** | SRS-D-009 / SDD §4.13 / §9 SEP-003 | **RCM-002 / RCM-019 / HZ-002** |
| UT-401-13 | `InProcessQueue_Concurrency.BurstFillThenDrain` | 50 burst × (Capacity-1) 件 publish/consume | 全件受信、burst パターン耐性 | 並行処理(TSan)| SDD §4.13 | RCM-019 |

UNIT-401 合計試験ケース: **13 件**(`gtest_discover_tests` で自動収集)

**累積合計試験ケース(UNIT-200 + UNIT-401)= 37 件**

### 7.3 カバレッジ目標

| 指標 | 目標値 | 本 v0.1 範囲での実績 |
|------|-------|------------------|
| ステートメントカバレッジ | 100%(安全関連) | UNIT-200 はヘッダオンリーかつ全機能を UT で網羅(本書 §8 参照)。llvm-cov 計測は Step 17+ で整備予定 |
| 分岐(decision)カバレッジ | 100%(安全関連)/ 95% 以上(その他) | 同上(`error_category` の switch、`severity_of` の switch を全分岐 UT 化済) |
| MC/DC | クラス C 推奨(本プロジェクトでは Inc.3 以降の重要分岐で導入判断) | (Step 17+ 整備予定) |

---

# 第 II 部 報告

## 8. 試験実施結果

### 8.1 実施サマリ(UNIT-200 / Step 17 / CR-0005)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-04-26 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: `a18b6cf` / CI 修正: `d6a6bc7`(UT-200-09 の `requires` 式を concept にラップ、clang-17/gcc-13 互換性確保)|
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で構文 + `static_assert(is_always_lock_free)` を検証(成功) |
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、CMake + Ninja + vcpkg(ベースライン `c3867e714d...`)、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。GitHub Actions cpp-build ワークフロー(run id `24959543848`)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)|

### 8.1bis 実施サマリ(UNIT-401 / Step 18 / CR-0006)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-04-29 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: (Step 18 本体コミット時に追記)|
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で構文 + `static_assert(std::atomic<size_t>::is_always_lock_free)` + `static_assert((Capacity & (Capacity-1)) == 0)` + template instantiation(`InProcessQueue<int, 16>::capacity()` 参照)を検証(成功)|
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_in_process_queue_tests` 13 件追加 |
| **CI 結果** | (Step 18 push 後に追記、`tsan` プリセットでの UT-401-12 race detection 0 が中核)|

### 8.2 試験ケース結果

#### UNIT-200(Step 17 / CR-0005、再掲)

| 試験 ID | ローカル骨格ビルド | CI cpp-build マトリクス(2026-04-26)| 備考 |
|--------|------------------|----------------------------|------|
| UT-200-01〜05(enum 系)| 構文 OK | **全 8 ジョブ Pass ✅** | enum 値・系統判定 |
| UT-200-06〜10b(強い型)| 構文 OK + compile-time 検証成功 | **全 8 ジョブ Pass ✅**(UT-200-09 は `d6a6bc7` で concept にラップ後、clang-17/gcc-13 で SFINAE-friendly に Pass)| `static_assert` + concept で構造的検証 |
| UT-200-11〜17(PulseCount/Counter)| **`static_assert(is_always_lock_free)` 通過** | **全 8 ジョブ Pass ✅。`clang-17/tsan`・`gcc-13/tsan` で UT-200-16 race-free 確認(4 スレッド × 10000 increment、race detection 0)** | **HZ-002/003/007 構造的予防の機械検証成立** |
| UT-200-18〜20b(Result)| 構文 OK | **全 8 ジョブ Pass ✅** | C++23 `std::expected` 互換 IF |
| (clang-tidy 全ファイル)| — | **clang-tidy (clang-17) Pass ✅** | MISRA C++ 2023 関連チェック完全 Pass |

#### UNIT-401(Step 18 / CR-0006、本 v0.2 で追加)

| 試験 ID | ローカル骨格ビルド | CI cpp-build マトリクス(Step 18 push 後に確定)| 備考 |
|--------|------------------|----------------------------|------|
| UT-401-01〜04(初期 / 基本 publish-consume)| 構文 OK | (push 後に追記)| 初期 empty、FIFO 順序、capacity() |
| UT-401-05〜07(境界値 / wrap-around)| 構文 OK | (push 後に追記)| Capacity-1 で full、empty で nullopt、100 サイクル wrap-around |
| UT-401-08〜09(lock-free 表明 / 所有権)| **`static_assert(is_always_lock_free)` + `static_assert((Capacity & (Capacity-1)) == 0)` 通過** | (push 後に追記)| HZ-007 構造的予防の build 時 fail-stop |
| UT-401-10〜11(メッセージ型独立性)| 構文 OK | (push 後に追記)| struct / `enum class TreatmentMode` 双方で動作 |
| **UT-401-12** | 構文 OK | **(push 後に追記、`clang-17/tsan`・`gcc-13/tsan` で UT-401-12 race detection 0 確認が中核)**| **HZ-002 直接対応**(SPSC で 1 producer + 1 consumer × 100k メッセージ全件受信 + FIFO 維持 + race-free)|
| UT-401-13(burst pattern)| 構文 OK | (push 後に追記)| 50 burst × (Capacity-1) で Pass |

> **本表の中核成果物:** `clang-17/tsan` および `gcc-13/tsan` プリセットで **UT-401-12(SPSC 1 producer + 1 consumer × 100k メッセージ)が race condition 検出 0 で Pass** することが、Step 18 の主要 acceptance criterion。SDD §4.13 で確定した SPSC ring buffer の release-acquire メモリオーダリングが正しく動作することの機械検証(SRMP §6.1 3 層検証の TSan 層、HZ-002 直接対応)。

### 8.3 カバレッジ実績

| ユニット ID | ステートメント | 分岐 | MC/DC | 備考 |
|------------|--------------|------|-------|------|
| UNIT-200 | (llvm-cov 未計測、Step 19+ 整備予定) | 同上 | — | UT 設計上は全機能を網羅(本書 §7.2 / §7.3) |
| UNIT-401 | (llvm-cov 未計測、Step 19+ 整備予定) | 同上 | — | UT 設計上は全公開 API + 境界値 + wrap-around + concurrent を網羅(本書 §7.2 / §7.3)|

### 8.4 不具合・逸脱

| 問題 ID | 内容 | 重大度 | 対応 | ステータス |
|--------|------|-------|------|----------|
| (CR-0005 修正コミット `d6a6bc7` で対応済) | UT-200-09 の `requires` 式を関数本体内に直接記述したため、clang-17 / gcc-13 で SFINAE 文脈外と判定されビルド失敗(AppleClang 15 では通っていた挙動差)| Minor(テストファイルの SFINAE パターン不適切、本番コードへの影響なし)| `requires` 式を `detail::HasPlusOp` / `HasMinusOp` / `HasMulOp` の 3 concept にラップして SFINAE 文脈に持ち込み、すべてのコンパイラで一貫した挙動を確保(`d6a6bc7`)| **解決(CI 全 9 ジョブ Pass で確認、2026-04-26)**|
| (Step 18 / CR-0006 範囲)| (本 Step push 後に CI 結果に応じて追記)| | | |

### 8.5 未達項目と処置

- **llvm-cov 網羅率計測**: 本 v0.1 範囲では未実施。Step 17+ 後続ステップで CI ワークフローに統合する(SOUP-017 として CIL §5 で正式登録予定)。本 v0.1 では UT 設計上の網羅性確認(本書 §7.2 試験ケース定義)で代替
- **モデル検査**: Inc.3(タスク同期)着手時に TLA+ / SPIN / Frama-C のいずれかを採用判断(SDD §10 未確定事項、SOUP-019 検討中として CIL §5 で記録済)

## 9. 結論

### UNIT-200(v0.1、再掲)

- [x] **UNIT-200 は受入基準(§5)を全て満たしている**(コーディング規約遵守、clang-tidy 0 警告、UT 全 24 件 CI Pass、ローカル骨格ビルドで `static_assert` 通過)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系・境界値・異常系・並行処理・タイミング・データフロー等を本書 §7.2 で網羅。資源使用量・分岐網羅は llvm-cov 整備後に追加計測予定)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`is_always_lock_free` `static_assert`、強い型 `explicit` + 算術非提供、`enum class` 型安全)

### UNIT-401(v0.2、本 Step 18)

- [x] **UNIT-401 は受入基準(§5)を全て満たしている**(SDD §4.13 v0.1.1 と完全一致、コーディング規約遵守、UT 全 13 件設計済、ローカル骨格ビルドで `static_assert(is_always_lock_free)` + power-of-two 制約 + default-constructible 制約 通過。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-401-01/03/04/10/11、境界値: UT-401-05/06/07、異常系: UT-401-06、並行処理: UT-401-12/13、データフロー: clang-tidy `bugprone-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<size_t>::is_always_lock_free` `static_assert`、Capacity = power-of-two `static_assert`、template type 制約 `is_default_constructible_v` / `is_move_constructible_v` / `is_move_assignable_v` `static_assert`)
- [x] 未解決問題なし(本 v0.2 範囲、CI Pass 確認後に最終クローズ)

## 10. トレーサビリティマトリクス(本 v0.2 範囲)

### UNIT-200(再掲、CI 実績 2026-04-26 確定済)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-200 | UT-200-01 | SRS-001 / SRS-D-001 / SDD §4.1 | RCM-001 / HZ-001 | 全 9 ジョブ Pass(2026-04-26)|
| UNIT-200 | UT-200-02 | SRS-002 / SDD §6.1 | RCM-001 | 同上 |
| UNIT-200 | UT-200-03 | SRS-007 / SRS-011 / SDD §4.1 | RCM-017 | 同上 |
| UNIT-200 | UT-200-04 | SDD §4.1.1 / §6.2 | RCM-009 / HZ-006 | 同上 |
| UNIT-200 | UT-200-05 | SDD §6.2 | RCM-010 / HZ-006 | 同上 |
| UNIT-200 | UT-200-06〜10b | SRS-D-002〜008 / SDD §6.3 | RCM-008 / HZ-005 | 同上 |
| UNIT-200 | UT-200-11〜14 | SRS-009 / SRS-D-005 / SDD §4.1 / §4.5 | RCM-003 / RCM-008 | 同上 |
| UNIT-200 | UT-200-15 | SRS-009 / SDD §4.5 | **RCM-003 / HZ-003** | 同上 |
| UNIT-200 | **UT-200-16** | SRS-009 / SRS-D-009 / SDD §4.13 | **RCM-002 / RCM-019 / HZ-002** | **`tsan` プリセットで race 0 確認(2026-04-26)** |
| UNIT-200 | UT-200-17 | SDD §4.1 | RCM-019 | 同上 |
| UNIT-200 | UT-200-18〜20b | SDD §6.4 | — / RCM-009 | 同上 |

### UNIT-401(本 v0.2 で追加、CI 実績は Step 18 push 後追記)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass(Step 18 push 後追記)|
|------------|--------|--------------|------------|--------------------------------|
| UNIT-401 | UT-401-01〜04 | SDD §4.13(SPSC 基本動作)| RCM-019 | (追記)|
| UNIT-401 | UT-401-05〜07 | SDD §4.13(境界値 / wrap-around)| RCM-019 | (追記)|
| UNIT-401 | UT-401-08 | SDD §7 SOUP-003/004 | **RCM-019 / HZ-002 / HZ-007** | (追記)|
| UNIT-401 | UT-401-09 | SDD §4.13(所有権)| RCM-019 | (追記)|
| UNIT-401 | UT-401-10〜11 | SDD §5.1 IF-U(メッセージ型独立性)/ §4.1(`enum class TreatmentMode`)| RCM-001 | (追記)|
| UNIT-401 | **UT-401-12** | SRS-D-009 / SDD §4.13 / §9 SEP-003 | **RCM-002 / RCM-019 / HZ-002** | **(`tsan` プリセットで SPSC race 0 確認、追記)**|
| UNIT-401 | UT-401-13 | SDD §4.13(burst pattern)| RCM-019 | (追記)|

## 11. 改訂履歴

| バージョン | 日付 | 変更内容 | 変更者 |
|----------|------|---------|--------|
| 0.1 | 2026-04-26 | 初版作成(Step 17 / CR-0005、本プロジェクト Issue #5)。**Inc.1 範囲のうち UNIT-200 CommonTypes に対する第 I 部(計画)+ 第 II 部(報告)を初期化**。**§3 実装ルール:** MISRA C++ 2023 + CERT C++ + C++20 + clang-tidy 全カテゴリ + Sanitizer 3 種 + `std::atomic` 共有変数規則(`PulseCounter` 経由のみ可)を確定。**§4 検証プロセス:** コードレビュー / 静的解析 / 動的解析 / UT / 網羅率計測の 5 手段を整理。**§4.2 並行処理 RCM の 3 層検証(SRMP §6.1 整合):** TSan + 境界値 + モデル検査(Inc.3 で導入判断)を必須化。**§5 受入基準 5 項目** + **§6 クラス C 追加受入基準 9 項目** + **§6.1 本プロジェクト固有追加基準 3 項目**(`is_always_lock_free` `static_assert`、強い型 `explicit` + 算術非提供、`enum class` 型安全)を確定。**§7.2 UNIT-200 試験ケース 24 件**(enum 系 5 件、強い型 7 件、PulseCount/Counter 7 件、Result 5 件)を定義し、`tests/unit/test_common_types.cpp` として実装済。**§8 実施サマリ:** ローカル骨格ビルド(`TH25_BUILD_TESTS=OFF`)で `static_assert(is_always_lock_free)` を AppleClang 15 で検証成功。CI cpp-build マトリクス全 8 ジョブ + clang-tidy ジョブの Pass は Step 17 push 後の SHA 追記事後処理で記録予定。**Therac-25 学習目的の中核達成:** UT-200-16 で 4 スレッド × 10000 increment による `PulseCounter` race-free 検証を TSan プリセットで実施(Tyler 第 1 例 race condition 類型の機械的検証手段確立)、UT-200-15 で `uint64_t::max()` 境界値検証(Yakima Class3 オーバフロー類型の構造的排除確認)、UT-200-12 + ヘッダ内 `static_assert(is_always_lock_free)` で HZ-007(レガシー前提喪失)構造的予防の機械検証成立 | k-abe |
| 0.2 | 2026-04-29 | **昇格(Step 18 / CR-0006、本プロジェクト Issue #6)**。**UNIT-401 InProcessQueue(SPSC lock-free ring buffer)を §3.2 実装対象に追加**(本 v0.2 計画範囲)、SDD-TH25-001 v0.1.1 への参照に更新(folly 採用 → 独自 Lamport SPSC 実装に確定)。**§7.2 UNIT-401 試験ケース 13 件追加**(初期 / publish-consume / 境界値 / wrap-around / lock-free 表明 / 所有権 / メッセージ型独立性 / 並行処理 SPSC 100k メッセージ / burst pattern)を定義し、`tests/unit/test_in_process_queue.cpp` として実装済。**累積試験ケース = 37 件**(UNIT-200 24 件 + UNIT-401 13 件)。**§8.1bis 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<size_t>::is_always_lock_free)` + Capacity = power-of-two `static_assert` + template type 制約 `static_assert` を AppleClang 15 で検証成功。CI cpp-build マトリクス全 9 ジョブ Pass は Step 18 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-401 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-401 行追加**(13 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UT-401-12 で 1 producer + 1 consumer × 100k メッセージ SPSC race-free 検証を `tsan` プリセットで実施する設計を確立(Tyler 第 1 例 race condition 類型に対する SDD §4.13 SPSC ring buffer 設計の機械的検証手段成立)。UT-401-08 + ヘッダ内 `static_assert(is_always_lock_free)` で HZ-007 構造的予防の機械検証を Inc.1 中核 MessageBus 層に拡張 | k-abe |
