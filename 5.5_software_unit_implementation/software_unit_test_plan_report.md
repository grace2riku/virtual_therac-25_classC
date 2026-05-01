# ソフトウェアユニットテスト計画書/報告書(UTPR)

**ドキュメント ID:** UTPR-TH25-001
**バージョン:** 0.6
**作成日:** 2026-04-26(初版)/ 2026-04-29(v0.2)/ 2026-04-30(v0.3)/ 2026-04-30(v0.4)/ 2026-04-30(v0.5)/ 2026-05-01(v0.6 昇格)
**対象製品:** 仮想 Therac-25(Virtual Therac-25) / TH25-SIM-001
**対象ソフトウェアバージョン:** 0.2.0(Inc.1 完了で初回機能リリース予定)
**安全クラス:** C(IEC 62304)
**変更要求:** CR-0005〜0009 + CR-0010(GitHub Issue #10、v0.6 UNIT-204 追加、HZ-005 直接 + HZ-003 構造的排除の中核)

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
> **本 v0.6(Step 22 / CR-0010)では UNIT-204 DoseManager(HZ-005 直接 + HZ-003 構造的排除の中核)を追加**。UNIT-205〜210 / 301〜304 / 402 の試験計画・実績は後続 SDD-準拠ステップで追加する。

---

## 1. 目的と適用範囲

### 1.1 目的

本書は、IEC 62304 箇条 5.5(ソフトウェアユニットの実装)および §5.5.4(クラス C 追加受入基準)に基づき、TH25-CTRL の各ソフトウェアユニットの **実装ルール、検証プロセス、受入基準、試験ケース、試験実績** を一元的に記録する。本書は「Step 17 以降のコード実装すべての品質保証の根拠」として参照される。

### 1.2 適用範囲

- 対象: SDD-TH25-001 v0.1.1 で確定した全 22 ユニット(UNIT-101〜104 / 200 / 201〜210 / 301〜304 / 401〜402)
- 本 v0.1 で記録範囲を確定したユニット: **UNIT-200 CommonTypes**(Step 17 / CR-0005)
- 本 v0.2 で追加したユニット: **UNIT-401 InProcessQueue**(Step 18 / CR-0006)
- 本 v0.3 で追加したユニット: **UNIT-201 SafetyCoreOrchestrator**(Step 19 / CR-0007)
- 本 v0.4 で追加したユニット: **UNIT-202 TreatmentModeManager**(Step 20 / CR-0008、HZ-001 直接対応の中核)
- 本 v0.5 で追加したユニット: **UNIT-203 BeamController**(Step 21 / CR-0009、RCM-017 中核 / 許可フラグ消費型)
- 後続 Step 22+ で他ユニットを順次追加

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
| [11] | 構成アイテム一覧(CIL-TH25-001) | 0.13 |
| [12] | CCB 運用規程(CCB-TH25-001) | 0.1 |
| [13] | 変更要求台帳(CRR-TH25-001) | 0.11 |
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
| **UNIT-200** | **CommonTypes** | k-abe | `src/th25_ctrl/include/th25_ctrl/common_types.hpp` | **v0.1〜v0.5 で計画 + 実施記録維持** |
| **UNIT-401** | **InProcessQueue** | k-abe | `src/th25_ctrl/include/th25_ctrl/in_process_queue.hpp` | **v0.2〜v0.5 で計画 + 実施記録維持** |
| **UNIT-201** | **SafetyCoreOrchestrator** | k-abe | `src/th25_ctrl/include/th25_ctrl/safety_core_orchestrator.hpp` + `src/safety_core_orchestrator.cpp` | **v0.3〜v0.5 で計画 + 実施記録維持** |
| **UNIT-202** | **TreatmentModeManager** | k-abe | `src/th25_ctrl/include/th25_ctrl/treatment_mode_manager.hpp` + `src/treatment_mode_manager.cpp` | **v0.4〜v0.5 で計画 + 実施記録維持(HZ-001 直接対応の中核)** |
| **UNIT-203** | **BeamController** | k-abe | `src/th25_ctrl/include/th25_ctrl/beam_controller.hpp` + `src/beam_controller.cpp` | **v0.5〜v0.6 で計画 + 実施記録維持(Step 21 / CR-0009、RCM-017 中核 / 許可フラグ消費型)** |
| **UNIT-204** | **DoseManager** | k-abe | `src/th25_ctrl/include/th25_ctrl/dose_manager.hpp` + `src/dose_manager.cpp` | **本 v0.6 で計画 + 実施記録(Step 22 / CR-0010、HZ-005 直接 + HZ-003 構造的排除の中核)** |
| UNIT-101〜104 | Operator UI 群 | k-abe | `src/th25_ui/...`(Inc.4 で追加予定) | (Inc.4 SDD 改訂時に追加) |
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

#### UNIT-201 SafetyCoreOrchestrator ユニット試験ケース一覧(本 v0.3 範囲、Step 19 / CR-0007 で追加)

| 試験 ID | GoogleTest 名 | 入力 | 期待結果 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|--------------|------|---------|------|--------------|-------------|
| UT-201-01 | `SafetyCoreOrchestrator_Initial.IsInit` | new instance | `current_state() == LifecycleState::Init` | 正常系 | SDD §4.2 / §6.1 | RCM-001 |
| UT-201-02 | `SafetyCoreOrchestrator_InitSubsystems.TransitionsToSelfCheck` | `init_subsystems()` 1 回呼出 | `Result::has_value() == true`、`current_state() == SelfCheck` | 正常系 | SRS-012 / SDD §4.2 | RCM-001 / RCM-013 |
| UT-201-03 | `SafetyCoreOrchestrator_InitSubsystems.SecondCallReturnsInternalUnexpectedState` | `init_subsystems()` 2 回連続呼出 | 1 回目 OK、2 回目 `Result::error_code() == InternalUnexpectedState`、状態 SelfCheck 維持 | 異常系(idempotency)| SDD §4.2 事前条件 | RCM-001 |
| UT-201-04 | `SafetyCoreOrchestrator_LockFree.AtomicLifecycleStateIsAlwaysLockFree` | (runtime + static_assert) | `is_always_lock_free == true`(LifecycleState / bool 両方) | 構造的(HZ-007) | SDD §7 SOUP-003/004 | RCM-002 / **HZ-002 / HZ-007** |
| UT-201-05 | `SafetyCoreOrchestrator_Ownership.IsNotCopyableNorMovable` | (compile-time) | `is_copy_constructible_v == false` 等 4 項目 | 構造的 | SDD §4.2 | — |
| UT-201-06 | `SafetyCoreOrchestrator_Shutdown.SetsShutdownFlagWithoutChangingState` | `init_subsystems()` 後に `shutdown()` | `noexcept(shutdown())` 表明 + `current_state()` は SelfCheck のまま | 正常系(signal-safe)| SDD §4.2 | RCM-017 |
| UT-201-07 | `SafetyCoreOrchestrator_EventLoop.ReturnsZeroWhenShutdownFromIdle` | Idle 状態で `shutdown()` 後に `run_event_loop` | 戻り値 0(Halted/Error 以外) | 正常系 | SDD §4.2 戻り値仕様 | — |
| UT-201-08 | `SafetyCoreOrchestrator_Transition.SelfCheckPassedToIdle` | `next_state(SelfCheck, SelfCheckPassed)` | `Idle` | 正常系(状態機械) | SDD §6.1 | RCM-001 |
| UT-201-09 | `SafetyCoreOrchestrator_Transition.SelfCheckFailedToError` | `next_state(SelfCheck, SelfCheckFailed)` | `Error` | 正常系 | SDD §6.1 | RCM-001 / RCM-013 |
| UT-201-10 | `SafetyCoreOrchestrator_Transition.IdleToPrescriptionSet` | `next_state(Idle, PrescriptionReceived)` | `PrescriptionSet` | 正常系 | SDD §6.1 | RCM-001 |
| UT-201-11 | `SafetyCoreOrchestrator_Transition.PrescriptionSetToReady` | `next_state(PrescriptionSet, PrescriptionValidated)` | `Ready` | 正常系 | SDD §6.1 | RCM-001 |
| UT-201-12 | `SafetyCoreOrchestrator_Transition.PrescriptionSetToIdleByReset` | `next_state(PrescriptionSet, PrescriptionReset)` | `Idle` | 正常系 | SDD §6.1 | RCM-001 |
| UT-201-13 | `SafetyCoreOrchestrator_Transition.ReadyToBeamOn` | `next_state(Ready, BeamOnRequested)` | `BeamOn` | 正常系 | SDD §6.1 / SRS-007 | RCM-001 / RCM-017 |
| UT-201-14 | `SafetyCoreOrchestrator_Transition.ReadyToIdleByReset` | `next_state(Ready, PrescriptionReset)` | `Idle` | 正常系 | SDD §6.1 | RCM-001 |
| UT-201-15 | `SafetyCoreOrchestrator_Transition.BeamOnToIdleByBeamOff` | `next_state(BeamOn, BeamOffCompleted)` | `Idle` | 正常系 | SDD §6.1 / SRS-101 | RCM-017 |
| UT-201-16 | `SafetyCoreOrchestrator_Transition.BeamOnToIdleByDoseTarget` | `next_state(BeamOn, DoseTargetReached)` | `Idle` | 正常系 | SDD §6.1 / SRS-010 | RCM-017 |
| UT-201-17 | `SafetyCoreOrchestrator_Transition.BeamOnToHaltedByCriticalAlarm` | `next_state(BeamOn, CriticalAlarmRaised)` | `Halted` | 正常系(致死的)| SDD §6.1 / SRS-UX-002 | RCM-001 / RCM-010 |
| UT-201-18 | `SafetyCoreOrchestrator_Transition.IdleToBeamOnIsForbidden` | `next_state(Idle, BeamOnRequested)` | `nullopt`(不正遷移)| **異常系**(RCM-001 中核)| SDD §6.1 | **RCM-001 / HZ-001** |
| UT-201-19 | `SafetyCoreOrchestrator_Transition.HaltedIsTerminal` | `next_state(Halted, *)` 全 10 イベント(Shutdown 除く)| すべて `nullopt`、ShutdownRequested のみ Halted | 構造的(終端状態)| SDD §6.1 | RCM-001 |
| UT-201-20 | `SafetyCoreOrchestrator_Transition.ShutdownRequestedAlwaysGoesToHalted` | `next_state(*, ShutdownRequested)` 全 8 状態 | すべて `Halted` | 構造的(緊急停止)| SDD §4.2 | RCM-017 |
| UT-201-21 | `SafetyCoreOrchestrator_Transition.ExhaustiveTableMatchesSdd` | 8 状態 × 11 イベント = 88 組合せ | 許可 19 件(論理 11 + Shutdown 8)/ 拒否 69 件、`is_transition_allowed` ↔ `next_state` の整合確認 | **構造的網羅試験** | SDD §6.1 表 | **RCM-001** |
| UT-201-22 | `SafetyCoreOrchestrator_EventLoop.DispatchesQueuedEventsThroughStateMachine` | キューに SelfCheckPassed → PrescriptionReceived → ShutdownRequested を投入し `run_event_loop` 起動 | 戻り値 1、状態 Halted | 正常系(統合) | SDD §4.2 / §6.1 / §4.13 | RCM-001 / RCM-019 |
| UT-201-23 | `SafetyCoreOrchestrator_EventLoop.IllegalEventTransitionsToHaltedAndExits` | Idle 状態で BeamOnRequested を投入 | 戻り値 1、状態 Halted、ループ終了 | **異常系**(不正遷移は Halted 強制)| SDD §4.2 異常系 / §6.1 | **RCM-001 / HZ-001** |
| **UT-201-24** | **`SafetyCoreOrchestrator_Concurrency.SpscEventDeliveryIsRaceFree`** | **1 producer + 1 consumer × 200 サイクル × 6 イベント / サイクル** | **全イベント正常配送、最終 Halted、`tsan` プリセットで race detection 0** | **並行処理(TSan 必須)** | SRS-D-009 / SDD §9 SEP-003 / §4.13 | **RCM-002 / RCM-019 / HZ-002** |

UNIT-201 合計試験ケース: **24 件**(`gtest_discover_tests` で自動収集)

#### UNIT-202 TreatmentModeManager ユニット試験ケース一覧(本 v0.4 範囲、Step 20 / CR-0008 で追加、HZ-001 直接対応の中核)

| 試験 ID | GoogleTest 名 | 入力 | 期待結果 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|--------------|------|---------|------|--------------|-------------|
| UT-202-01 | `TreatmentModeManager_Initial.DefaultIsLight` | デフォルト構築 | `current_mode() == Light` | 正常系 | SDD §4.3 | RCM-001 |
| UT-202-02 | `TreatmentModeManager_Initial.ExplicitInitialMode` | `{Electron}` / `{XRay}` で構築 | 各モードに正しく初期化 | 正常系 | SDD §4.3 | RCM-001 |
| UT-202-03 | `TreatmentModeManager_Transition.LightToElectron` | Light + `request_mode_change(Electron, 10 MeV, Off)` | `Result::ok()`、Electron へ遷移 | 正常系 | SRS-002 / SDD §4.3 | RCM-001 |
| UT-202-04 | `TreatmentModeManager_Transition.LightToXRay` | Light + XRay 要求 | Ok、XRay へ遷移 | 正常系 | SRS-002 / SDD §4.3 | RCM-001 |
| UT-202-05 | `TreatmentModeManager_Transition.ElectronToXRay` | Electron → XRay 直接遷移(SDD §4.3 表) | Ok、XRay へ遷移 | 正常系 | SRS-002 / SDD §4.3 | RCM-001 |
| UT-202-06 | `TreatmentModeManager_Transition.XRayToElectron` | XRay → Electron 直接遷移 | Ok | 正常系 | SRS-002 / SDD §4.3 | RCM-001 |
| UT-202-07 | `TreatmentModeManager_Transition.AnyToLight` | Electron / XRay → Light(治療終了)| Ok、Light へ | 正常系 | SDD §4.3 | RCM-001 |
| UT-202-08 | `TreatmentModeManager_Transition.SameModeIsNoOpSuccess` | Electron + Electron 要求(no-op)| Ok | 正常系 | SDD §4.3 | RCM-001 |
| UT-202-09 | `TreatmentModeManager_BeamStateGuard.RejectsElectronWhenBeamOn` | BeamState ∈ {Arming, On, Stopping} で Electron 要求 | `ModeBeamOnNotAllowed`、状態不変 | **異常系**(RCM-001 中核)| SRS-002 / SDD §4.3 | **RCM-001 / HZ-001** |
| UT-202-10 | `TreatmentModeManager_BeamStateGuard.RejectsXRayWhenBeamOn` | BeamState::On で XRay 要求 | `ModeBeamOnNotAllowed` | 異常系 | SRS-002 / SDD §4.3 | RCM-001 / HZ-001 |
| UT-202-11 | `TreatmentModeManager_BeamStateGuard.RejectsLightWhenBeamOn` | BeamState::Arming で Light 要求 | `ModeBeamOnNotAllowed` | 異常系 | SDD §4.3 | RCM-001 |
| UT-202-12 | `TreatmentModeManager_EnergyRange.ElectronBelowMinIsRejected` | Electron + 0.99 MeV | `ModeInvalidTransition` | **境界値** | SRS-004 / SDD §4.3 | RCM-008 |
| UT-202-13 | `TreatmentModeManager_EnergyRange.ElectronAboveMaxIsRejected` | Electron + 25.01 MeV | `ModeInvalidTransition` | 境界値 | SRS-004 / SDD §4.3 | RCM-008 |
| UT-202-14 | `TreatmentModeManager_EnergyRange.XRayBelowMinIsRejected` | XRay + 4.99 MV | `ModeInvalidTransition` | 境界値 | SRS-004 / SDD §4.3 | RCM-008 |
| UT-202-15 | `TreatmentModeManager_EnergyRange.XRayAboveMaxIsRejected` | XRay + 25.01 MV | `ModeInvalidTransition` | 境界値 | SRS-004 / SDD §4.3 | RCM-008 |
| UT-202-16 | `TreatmentModeManager_EnergyRange.ElectronBoundaryIsAccepted` | Electron + 1.0 / 25.0 MeV(境界値)+ 範囲外 ±0.01 | 境界値受入、範囲外拒否 | 境界値網羅 | SRS-004 / SDD §4.3 | RCM-008 / HZ-005 |
| UT-202-17 | `TreatmentModeManager_EnergyRange.XRayBoundaryIsAccepted` | XRay + 5.0 / 25.0 MV(境界値) | 同上 | 境界値網羅 | SRS-004 / SDD §4.3 | RCM-008 / HZ-005 |
| **UT-202-18** | `TreatmentModeManager_ApiGuard.ElectronApiRejectsXRayMode` | Electron 用 API に `XRay` を渡す | `ModeInvalidTransition`(**Therac-25 Tyler 事故型「Electron モードで X 線エネルギー設定」を構造的拒否**)| **異常系**(HZ-001 直接対応)| SDD §4.3 | **RCM-001 / HZ-001** |
| UT-202-19 | `TreatmentModeManager_ApiGuard.XRayApiRejectsElectronMode` | XRay 用 API に Electron | `ModeInvalidTransition` | 異常系 | SDD §4.3 | RCM-001 / HZ-001 |
| UT-202-20 | `TreatmentModeManager_StrongTypes.EnergyMeVAndEnergyMVAreDistinct` | (compile-time) | Energy_MeV ↔ Energy_MV 暗黙変換不可、`double` からも不可 | 構造的(compile)| SRS-D-002/003 / SDD §6.3 | **RCM-008 / HZ-005** |
| **UT-202-21** | **`TreatmentModeManager_Concurrency.ConcurrentRequestsAreRaceFree`** | **2 writer(Electron / XRay)+ 1 reader × 1000 イテレーション** | 全 writer 要求成功、最終モードは Electron / XRay のいずれか、未定義値検出なし、`tsan` で race detection 0 | **並行処理(TSan 必須)** | SRS-D-009 / SDD §4.3 | **RCM-001 / RCM-002 / HZ-002** |
| UT-202-22 | `TreatmentModeManager_Ownership.IsNotCopyableNorMovable` | (compile-time) | `is_copy_constructible_v == false` 等 4 項目 | 構造的 | SDD §4.3 | — |
| UT-202-23 | `TreatmentModeManager_LockFree.AtomicTreatmentModeIsAlwaysLockFree` | (runtime + static_assert) | `is_always_lock_free == true` | 構造的(HZ-007)| SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** |
| UT-202-24 | `TreatmentModeManager_VerifyConsistency.MatchAndMismatch` | `verify_mode_consistency` 一致 / 不一致 | 一致 → Ok、不一致 → `ModePositionMismatch` | 正常系 + 異常系 | SRS-003 / SDD §4.3 | RCM-001 / RCM-005(構造的前提)|
| UT-202-25 | `TreatmentModeManager_TransitionTable.ExhaustiveAllPairsAllowed` | 3 状態 × 3 要求モード = 9 通り全件 | 全 9 通り許可(SDD §4.3 表との完全一致)| **構造的網羅試験** | SDD §4.3 表 | **RCM-001** |
| UT-202-26 | `TreatmentModeManager_TransitionTable.RejectsInvalidEnumValues` | キャスト経由の不正 enum 値 | 拒否(防御的) | 構造的(防御)| SDD §4.1 | RCM-001 |

UNIT-202 合計試験ケース: **26 件**(`gtest_discover_tests` で自動収集)

#### UNIT-203 BeamController ユニット試験ケース一覧(本 v0.5 範囲、Step 21 / CR-0009 で追加、RCM-017 中核)

| 試験 ID | GoogleTest 名 | 入力 | 期待結果 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|--------------|------|---------|------|--------------|-------------|
| UT-203-01 | `BeamController_Initial.IsOff` | デフォルト構築 | `current_state() == Off` | 正常系 | SDD §4.4 / §4.1 | RCM-017 |
| UT-203-02 | `BeamController_Initial.PermissionDefaultFalse` | デフォルト構築 | `is_beam_on_permitted() == false`(安全側既定値)| 正常系 | SDD §4.4 | RCM-017 |
| UT-203-03 | `BeamController_Permission.SetAndRead` | `set_beam_on_permission(true/false)` | 整合読取 | 正常系 | SDD §4.4 | RCM-017 |
| UT-203-04 | `BeamController_LifecycleGuard.RejectsBeamOnWhenNotReady` | Lifecycle ∈ {Init, SelfCheck, Idle, PrescriptionSet, BeamOn, Halted, Error}(7 状態)で beam_on | 全件 `BeamOnNotPermitted`、状態 Off / permission 維持 | **異常系**(RCM-017 中核)| SRS-007 / SDD §4.4 | **RCM-017 / HZ-001** |
| UT-203-05 | `BeamController_PermissionGuard.RejectsBeamOnWhenPermissionFalse` | permission false で beam_on | `BeamOnNotPermitted`、状態 Off | 異常系 | SDD §4.4 | RCM-017 / HZ-004 |
| UT-203-06 | `BeamController_BeamOn.SuccessTransitionsToOn` | Lifecycle = Ready + permission = true | `Result::ok()`、状態 On、permission 消費 | 正常系 | SRS-007 / SDD §4.4 | RCM-017 |
| **UT-203-07** | `BeamController_PermissionGuard.IsConsumedOnceAndRejectsSecond` | 連続 beam_on 2 回 | 1 回目成功、2 回目 `BeamOnNotPermitted`(**1 回限り消費**)| **異常系**(Therac-25 Tyler 事故型「ビーム二重照射」の構造的拒否)| SDD §4.4 | **RCM-017 / HZ-001 / HZ-004** |
| UT-203-08 | `BeamController_BeamOff.OnToOffViaStopping` | On 状態で beam_off | `Result::ok()`、状態 Off、permission false | 正常系 | SRS-101 / SDD §4.4 | RCM-017 |
| UT-203-09 | `BeamController_BeamOff.OffIsNoOp` | Off で beam_off | Off のまま、許容 | 正常系(フェイルセーフ)| SDD §4.4 | RCM-017 |
| UT-203-10 | `BeamController_PermissionLifecycle.ReBeamOnRequiresPermissionReset` | beam_on → beam_off → 再 beam_on(permission 再設定なし vs 再設定あり) | 再設定なしは拒否、再設定後は受入 | 正常系(RCM-017 二重照射防止) | SDD §4.4 | **RCM-017 / HZ-001** |
| UT-203-11 | `BeamController_Ownership.IsNotCopyableNorMovable` | (compile-time) | 4 trait 全て false | 構造的 | SDD §4.4 | — |
| UT-203-12 | `BeamController_LockFree.AtomicsAreAlwaysLockFree` | (runtime + static_assert) | `std::atomic<BeamState>` + `std::atomic<bool>` 両方 lock-free | 構造的(HZ-007)| SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** |
| **UT-203-13** | **`BeamController_Concurrency.BeamOnIsConsumedExactlyOnceUnderRace`** | **8 thread × 1000 セット並列 beam_on(各セット permission 再設定後)** | **各セットで唯一 1 thread のみ成功**、合計成功数 = 1000(`tsan` プリセットで race detection 0)| **並行処理(TSan 必須)** | SRS-D-009 / SDD §4.4 | **RCM-002 / RCM-017 / HZ-002 / HZ-004** |
| UT-203-14 | `BeamController_Concurrency.ParallelBeamOffIsRaceFree` | 8 thread 並列 beam_off | 全件 OK、最終状態 Off | 並行処理(TSan)| SDD §4.4 | RCM-002 / RCM-017 |
| UT-203-15 | `BeamController_BeamStateValues.AllFourStatesAreStable` | (compile-time) | Off=0/Arming=1/On=2/Stopping=3 | 構造的 | SDD §4.1 | — |
| UT-203-16 | `BeamController_LifecycleGuard.RejectsInvalidLifecycleEnumValue` | キャスト経由不正 LifecycleState | `BeamOnNotPermitted`(防御的) | 構造的(防御)| SDD §4.4 | RCM-017 |
| UT-203-17 | `BeamController_Concurrency.SetPermissionRacesWithBeamOnButConsumesOnce` | 1 setter + 4 reader × 500 イテレーション | 各イテレーションで唯一 1 reader 成功 | **並行処理(TSan)** | SRS-D-009 / SDD §4.4 | RCM-002 / RCM-017 / HZ-002 |
| UT-203-18 | `BeamController_StateGuard.RejectsBeamOnWhenAlreadyOn` | 既に On 状態で再 beam_on | `BeamOnNotPermitted`(state ガード、再エントラント防止)| 異常系 | SDD §4.4 | RCM-017 |
| UT-203-19 | `BeamController_BeamOff.AlwaysSucceedsRegardlessOfCurrentState` | 任意状態で beam_off | 常に成功(SDD §4.4 「事前条件: 任意」フェイルセーフ)| 正常系 | SDD §4.4 | RCM-017 |

UNIT-203 合計試験ケース: **19 件**(`gtest_discover_tests` で自動収集)

#### UNIT-204 試験ケース(本 v0.6 で追加、Step 22 / CR-0010、HZ-005 直接 + HZ-003 構造的排除の中核)

`tests/unit/test_dose_manager.cpp` として実装(本 Step 22 / CR-0010 でローカル骨格ビルド検証成功)。

| 試験 ID | テスト名 | 入力 | 期待される出力 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|---------|------|--------------|------|--------------|------------|
| UT-204-01 | `DoseManager_Initial.AllZeroAndUnset` | 初期 DoseManager(rate=1.0)| target=0、accumulated=0、reached=false、target_pulses=`max()`(未設定保護)| 正常系(初期状態)| SDD §4.5 / SRS-009 | RCM-003 |
| UT-204-02 | `DoseManager_Initial.ConstructWithDifferentRates` | rate=1.0 / 0.05 で初期化 | accumulated=0 両方 | 正常系 | SDD §4.5 / §6.5 | RCM-008 |
| UT-204-03 | `DoseManager_SetDoseTarget.Normal_Ready` | target=200 cGy、Ready | `Result::ok()`、target_pulses=200 | 正常系 | SRS-008 / SDD §4.5 | RCM-008 |
| UT-204-04 | `DoseManager_SetDoseTarget.Normal_PrescriptionSet` | target=500 cGy、PrescriptionSet | `Result::ok()`、target_pulses=500 | 正常系 | SRS-008 / SDD §4.5 | RCM-008 |
| UT-204-05 | `DoseManager_SetDoseTarget.BoundaryMin_Accept` | target=0.01 cGy、rate=0.05 | `Result::ok()`、target_pulses=0(切り捨て)| 境界値 | SRS-008 | **RCM-008 / HZ-005** |
| UT-204-06 | `DoseManager_SetDoseTarget.BoundaryMax_Accept` | target=10000 cGy | `Result::ok()`、target_pulses=10000 | 境界値 | SRS-008 | **RCM-008 / HZ-005** |
| UT-204-07 | `DoseManager_SetDoseTarget.BelowMin_Reject` | target=0.005 cGy(範囲下限以下)| `DoseOutOfRange`、内部状態不変 | 異常系(境界値外)| SRS-008 / SDD §4.5 | **RCM-008 / HZ-005** |
| UT-204-08 | `DoseManager_SetDoseTarget.AboveMax_Reject` | target=10001 cGy(範囲上限超)| `DoseOutOfRange` | 異常系(境界値外)| SRS-008 / SDD §4.5 | **RCM-008 / HZ-005** |
| UT-204-09 | `DoseManager_SetDoseTarget.Zero_Reject` | target=0 cGy | `DoseOutOfRange` | 異常系 | SRS-008 | **RCM-008 / HZ-005** |
| UT-204-10 | `DoseManager_SetDoseTarget.LifecycleIdle_Reject` | target=200 cGy、Idle | `InternalUnexpectedState`、内部状態不変 | 異常系(lifecycle 違反)| SDD §4.5 | RCM-001 |
| UT-204-11 | `DoseManager_SetDoseTarget.LifecycleBeamOn_Reject` | target=200 cGy、BeamOn | `InternalUnexpectedState` | 異常系(lifecycle 違反)| SDD §4.5 | RCM-001 |
| UT-204-12 | `DoseManager_SetDoseTarget.AllLifecycleStates` | 全 8 LifecycleState を順次 | PrescriptionSet/Ready のみ受入、他 6 状態は `InternalUnexpectedState` | 異常系(8 状態網羅)| SDD §4.5 | RCM-001 |
| UT-204-13 | `DoseManager_SetDoseTarget.ResetsAccumulator` | 50 パルス積算 → set_dose_target 再 | accumulated=0、reached=false | 正常系(累積リセット)| SDD §4.5 | RCM-003 |
| UT-204-14 | `DoseManager_OnDosePulse.SingleAdvance` | 1 パルス積算 | accumulated=1.0 cGy(rate=1.0)| 正常系 | SDD §4.5 / SRS-009 | RCM-003 / RCM-004 |
| UT-204-15 | `DoseManager_OnDosePulse.Monotonic` | 100 パルス積算 | 単調増加で 100.0 cGy | 正常系 | SDD §4.5 / SRS-009 | RCM-003 |
| UT-204-16 | `DoseManager_PulseToDose.ConversionMatchesRate` | rate=1.0/0.05 で複数値 | rate × pulses = 期待値 | 正常系(単位変換)| SDD §4.5 / §6.5 | **RCM-008 / HZ-005** |
| UT-204-17 | `DoseManager_TargetReached.JustBelow` | target=100、99 パルス | accumulated=99、reached=false | 境界値(未達)| SDD §4.5 / SRS-010 | RCM-008 / HZ-005 |
| UT-204-18 | `DoseManager_TargetReached.ExactReach` | target=100、100 パルス | accumulated=100、reached=true | 境界値(到達)| SRS-010 / SDD §4.5 | **RCM-008 / HZ-005** |
| UT-204-19 | `DoseManager_TargetReached.OverShoot` | target=100、101 パルス | accumulated=101、reached=true 維持 | 境界値(超過)| SDD §4.5 / SRS-ALM-002 | **RCM-008 / HZ-005** |
| UT-204-20 | `DoseManager_TargetReached.NoTargetSet_NoReachDetection` | target 未設定 + 1000 パルス | accumulated=1000、reached=false(未設定保護)| 異常系(防御)| SDD §4.5 | RCM-001 |
| **UT-204-21** | **`DoseManager_HZ003.LargeAccumulationStable`** | **10^9 パルス一度に積算** | **accumulated=1.0e9 cGy で破綻なし、uint64_t::max() 余裕で表現範囲内** | **境界値(HZ-003 構造的排除)** | SRS-009 / SDD §4.5 | **RCM-003 / HZ-003** |
| UT-204-22 | `DoseManager_SetDoseTarget.BoundaryValuesMechanicalCheck` | 6 ケース(0.005/0.01/10000/10000.001/10001/-1)| 受入 2 件・拒否 4 件で全件 ErrorCode=DoseOutOfRange | 境界値(SRS-008 機械検証)| SRS-008 | **RCM-008 / HZ-005** |
| UT-204-23 | `DoseManager_IsInRange.StaticPredicate` | 8 値で `is_dose_target_in_range` | 範囲内/外を完全網羅 | 静的純粋関数 | SRS-008 / SDD §4.5 | RCM-008 |
| UT-204-24 | `DoseManager_Reset.AllFieldsCleared` | 100 パルス到達 → reset() | accumulated=0、target=0、reached=false、target_pulses=`max()` | 正常系(全クリア)| SDD §4.5 | RCM-001 |
| **UT-204-25** | **`DoseManager_SetDoseTarget.MatrixCoverage`** | **8 状態 × 3 ドーズ値 = 24 組合せ** | **lifecycle_ok && in_range のみ受入、他は適切な ErrorCode で拒否** | **網羅試験(SDD §4.5 表との完全一致)** | SRS-008 / SDD §4.5 | RCM-001 / RCM-008 |
| UT-204-26 | `DoseManager_Ownership.NoCopyNoMove` | (compile-time) | コピー/ムーブ 4 trait 全て false | 構造的(所有権)| SDD §4.5 | — |
| UT-204-27 | `DoseManager_HZ007.AtomicsAreLockFree` | (compile-time `static_assert` 二重表明)| `std::atomic<uint64_t>` + `std::atomic<bool>` 両方 lock-free を build 時検証(GCC libstdc++ 14 で `is_lock_free()` runtime メンバが libatomic 動的リンクを要求するため `is_always_lock_free` のみ使用)| 構造的(HZ-007)| SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** |
| UT-204-28 | `DoseManager_StrongTypes.CompileTimeIsolation` | (compile-time) | `DoseRatePerPulse_cGy_per_pulse` / `DoseUnit_cGy` / `PulseCount` 相互の暗黙変換不可 | 構造的(強い型)| SRS-D-002〜005 / SDD §6.3 / §6.5 | **RCM-008 / HZ-005** |
| UT-204-29 | `DoseManager_SetDoseTarget.DoseOverflowOnInvalidRate` | rate=0/負/極小(target/rate が表現範囲外)| `DoseOverflow` | 異常系(防御)| SDD §4.5 / SDD §4.1.1 | RCM-003 / RCM-008 |
| **UT-204-30** | **`DoseManager_Concurrency.ProducerProducerReader`** | **4 producer × 10000 パルス + 1 reader 並行**(rate=0.05 cGy/pulse、target=10000 cGy = SRS-008 上限受入、target_pulses=200000)| **累積パルス=40000、ドーズ=2000.0 cGy、reached=false(40000 < 200000)、reader race-free に load**(`tsan` プリセットで race detection 0)| **並行処理(TSan 必須)** | SRS-D-009 / SDD §4.5 / §9 SEP-003 | **RCM-002 / RCM-004 / HZ-002** |
| UT-204-31 | `DoseManager_Concurrency.ProducerSetterRace` | 1 producer × 5000 パルス + 1 setter(target 切替・reset)| データレース無し(`tsan`)| 並行処理(TSan)| SRS-D-009 / SDD §4.5 | RCM-002 / RCM-004 / HZ-002 |
| UT-204-32 | `DoseManager_ErrorCodes.CategoryConsistency` | DoseOutOfRange / DoseOverflow / InternalUnexpectedState | category 上位 8bit + Severity が想定通り(全て Critical)| 構造的(ErrorCode 階層整合)| SDD §4.1.1 / §6.2 | RCM-009 |

UNIT-204 合計試験ケース: **32 件**(`gtest_discover_tests` で自動収集)

**累積合計試験ケース(UNIT-200 + UNIT-401 + UNIT-201 + UNIT-202 + UNIT-203 + UNIT-204)= 138 件**

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
| 対象ソフトウェアバージョン(コミット)| 本体: `2777378`|
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で構文 + `static_assert(std::atomic<size_t>::is_always_lock_free)` + `static_assert((Capacity & (Capacity-1)) == 0)` + template instantiation(`InProcessQueue<int, 16>::capacity()` 参照)を検証(成功)|
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_in_process_queue_tests` 13 件追加。GitHub Actions cpp-build ワークフロー(run id `25135657465`)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-401-12(SPSC 100k メッセージ並行)race detection 0 で Pass**、SDD §9 SEP-003 構造的予防の機械的検証成立 |

### 8.1ter 実施サマリ(UNIT-201 / Step 19 / CR-0007)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-04-30 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: `3fef49b` |
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で構文 + `static_assert(std::atomic<LifecycleState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` + template instantiation(`SafetyCoreOrchestrator::EventQueue::capacity()` 参照)を検証(成功、3 ターゲットの完全ビルド `[1/3]..[3/3] Linking CXX static library libth25_ctrl.a` 完了)|
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_safety_core_orchestrator_tests` 24 件追加。GitHub Actions cpp-build ワークフロー(run id `25138602852`)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-201-24(SPSC イベント配送 200 サイクル × 6 イベント並行)race detection 0 で Pass**、SDD §9 SEP-003 + RCM-019 のオーケストレータ層拡大の機械的検証成立。**UT-201-21 で SDD §6.1 表との完全一致を 88 組合せ網羅試験(許可 19 / 拒否 69)で機械的に検証成立** |

### 8.1quater 実施サマリ(UNIT-202 / Step 20 / CR-0008、HZ-001 直接対応の中核)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-04-30 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: `e506ea7` |
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で構文 + `static_assert(std::atomic<TreatmentMode>::is_always_lock_free)` + 4 ターゲットの完全ビルド `[1/4]..[4/4]` 完了(成功)|
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_treatment_mode_manager_tests` 26 件追加。GitHub Actions cpp-build ワークフロー(run id `25166736078`)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-202-21(2 writer + 1 reader × 1000 イテレーション並行)race detection 0 で Pass**、`std::atomic<TreatmentMode>` の release-acquire ordering の正しさが両コンパイラで機械的に検証成立。**UT-202-25 で SDD §4.3 表との完全一致を 9 組合せ網羅試験で機械的検証成立**(SDD ↔ 実装の構造的乖離防止メカニズムを 4 ユニット目に拡大)。**UT-202-18〜19 で Therac-25 Tyler 事故型「Electron API に XRay モード」「XRay API に Electron モード」の構造的拒否を機械検証成立**(HZ-001 直接対応の中核達成)|

### 8.1quinquies 実施サマリ(UNIT-203 / Step 21 / CR-0009、RCM-017 中核 / 許可フラグ消費型)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-04-30 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: `f7387b2` |
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で構文 + `static_assert(std::atomic<BeamState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明を AppleClang 15 で検証成功(`th25_ctrl` 静的ライブラリ完全ビルド成功)|
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_beam_controller_tests` 19 件追加。GitHub Actions cpp-build ワークフロー(run id `25169361882`)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-203-13(8 thread × 1000 セット並列 beam_on)が「各セットで唯一 1 thread のみ成功」+ race detection 0 で Pass**、`compare_exchange_strong` による消費型予防が両コンパイラで機械検証成立(Therac-25 Tyler 事故型「許可確認の race condition」「ビーム二重照射」の構造的予防達成)。**UT-203-07 で「ビーム二重照射」の構造的拒否を機械検証成立**、**UT-203-17 で setter/reader 並行(1 setter + 4 reader × 500 イテレーション)race-free 検証成立**(RCM-017 中核達成)|

### 8.1sexies 実施サマリ(UNIT-204 / Step 22 / CR-0010、HZ-005 直接 + HZ-003 構造的排除の中核)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-05-01 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: `3f18eee` / PRB-0001 修正(libatomic、UT-204-27 から runtime is_lock_free() 削除): `8032f61` / PRB-0002 修正(UT-204-30 SRS-008 範囲内化、rate=0.05 + target=10000): `b1bdae4` |
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で `static_assert(std::atomic<std::uint64_t>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明を AppleClang 15 で検証成功(`th25_ctrl` 静的ライブラリ `[1/4]..[4/4]` 完全ビルド成功)。`PulseCounter` を内包し target_pulses_(`std::atomic<uint64_t>`)/ target_reached_ / target_set_(`std::atomic<bool>`)の 4 atomic + DoseRatePerPulse_cGy_per_pulse 強い型をヘッダ内 SDD §6.5 から導入 |
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_dose_manager_tests` 32 件追加。GitHub Actions cpp-build ワークフロー(run id `25195589849`、2026-05-01、最終コミット `b1bdae4`)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-204-30(4 producer × 10000 パルス + 1 reader 並行、rate=0.05 cGy/pulse、target=10000 cGy = SRS-008 上限受入、target_pulses=200000)race detection 0 で Pass**(`std::atomic<uint64_t>::fetch_add(acq_rel)` + `std::atomic<bool>` の release-acquire ordering の正しさが両コンパイラで機械検証成立)。**UT-204-21 で 10^9 パルス積算でも uint64_t::max() 余裕(5,800 万年連続照射相当)を機械検証成立**(Yakima Class3 オーバフロー類型の構造的不可能化を実装層で完成)。**UT-204-25 で SDD §4.5 表との完全一致を 8 lifecycle × 3 ドーズ値 = 24 組合せ網羅機械検証成立**(SDD ↔ 実装の構造的乖離防止メカニズムを 5 ユニット目に拡大)。**VERIFICATION 段階で PRB-0001(#11、libatomic リンクエラー、`3f18eee`)+ PRB-0002(#12、UT-204-30 SRS-008 範囲外、`8032f61`)を SPRP §3.1 規定により遡及起票 → 修正(`8032f61` / `b1bdae4`)→ 検証(本 run id)完了。本プロジェクトで CI 失敗が初めて発生したケースであり、SPRP §3.1 PRB 起票プロセスの本格運用第 1 例として後続プロジェクトの教訓となる** |

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

| 試験 ID | ローカル骨格ビルド | CI cpp-build マトリクス(2026-04-29、run id `25135657465`)| 備考 |
|--------|------------------|----------------------------|------|
| UT-401-01〜04(初期 / 基本 publish-consume)| 構文 OK | **全 8 ジョブ Pass ✅** | 初期 empty、FIFO 順序、capacity() |
| UT-401-05〜07(境界値 / wrap-around)| 構文 OK | **全 8 ジョブ Pass ✅** | Capacity-1 で full、empty で nullopt、100 サイクル wrap-around |
| UT-401-08〜09(lock-free 表明 / 所有権)| **`static_assert(is_always_lock_free)` + `static_assert((Capacity & (Capacity-1)) == 0)` 通過** | **全 8 ジョブ Pass ✅** | HZ-007 構造的予防の build 時 fail-stop |
| UT-401-10〜11(メッセージ型独立性)| 構文 OK | **全 8 ジョブ Pass ✅** | struct / `enum class TreatmentMode` 双方で動作 |
| **UT-401-12** | 構文 OK | **全 8 ジョブ Pass ✅。`clang-17/tsan`・`gcc-13/tsan` で UT-401-12 race detection 0 確認(1 producer + 1 consumer × 100k メッセージ)** | **HZ-002 直接対応の機械検証成立**(SPSC ring buffer release-acquire の正しさ実証)|
| UT-401-13(burst pattern)| 構文 OK | **全 8 ジョブ Pass ✅** | 50 burst × (Capacity-1) で Pass |
| (clang-tidy 全ファイル)| — | **clang-tidy (clang-17) Pass ✅**(`bugprone-*` / `cert-*` / `concurrency-*` / `cppcoreguidelines-*` 全カテゴリ警告 0)| MISRA C++ 2023 関連チェック完全 Pass |

> **本表の中核成果物達成:** `clang-17/tsan` および `gcc-13/tsan` プリセットで **UT-401-12(SPSC 1 producer + 1 consumer × 100k メッセージ)が race condition 検出 0 で Pass** したこと。SDD §4.13 で確定した Lamport 1983 SPSC ring buffer の release-acquire メモリオーダリングが正しく動作することが、両コンパイラ × TSan で機械的に検証された(SRMP §6.1 3 層検証の TSan 層、HZ-002 直接対応の Inc.1 中核 MessageBus 層への拡大)。

#### UNIT-201(Step 19 / CR-0007、本 v0.3 で追加)

| 試験 ID | ローカル骨格ビルド | CI cpp-build マトリクス(2026-04-30、run id `25138602852`)| 備考 |
|--------|------------------|----------------------------|------|
| UT-201-01〜03(初期 / init_subsystems / idempotency)| 構文 OK | **全 9 ジョブ Pass ✅** | `compare_exchange_strong` で二重呼出を構造的に拒否 |
| UT-201-04(lock-free 表明)| **`static_assert(std::atomic<LifecycleState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 通過** | **全 9 ジョブ Pass ✅** | HZ-007 構造的予防の build 時 fail-stop |
| UT-201-05〜06(所有権 / shutdown signal-safe)| 構文 OK | **全 9 ジョブ Pass ✅** | コピー/ムーブ禁止、`shutdown` は state 変更せず flag のみ |
| UT-201-07(EventLoop 正常終了)| 構文 OK | **全 9 ジョブ Pass ✅** | Idle で shutdown → run_event_loop 戻り値 0 |
| UT-201-08〜17(状態遷移許可表 10 件、SDD §6.1 個別)| 構文 OK | **全 9 ジョブ Pass ✅** | 各論理遷移を `next_state` 単独で機械検証 |
| UT-201-18〜20(不正遷移 / 終端 / 緊急停止)| 構文 OK | **全 9 ジョブ Pass ✅** | **HZ-001 直接対応**(Idle→BeamOn 直接禁止)、Halted 終端、ShutdownRequested 任意状態許可 |
| **UT-201-21**(網羅試験 88 組合せ)| 構文 OK | **全 9 ジョブ Pass ✅** | **SDD §6.1 表との完全一致を機械検証**(許可 19 / 拒否 69)|
| UT-201-22〜23(EventLoop 統合 + 不正遷移時の Halted)| 構文 OK | **全 9 ジョブ Pass ✅** | キュー駆動の状態遷移、不正イベントは Halted + 戻り値 1 |
| **UT-201-24** | 構文 OK | **全 9 ジョブ Pass ✅。`clang-17/tsan` + `gcc-13/tsan` で UT-201-24 race detection 0 確認**(1 producer + 1 consumer × 200 サイクル × 6 イベント)| **HZ-002 直接対応の機械検証成立**(SDD §9 SEP-003 + RCM-019 のオーケストレータ層拡大)|
| (clang-tidy 全ファイル)| — | **clang-tidy (clang-17) Pass ✅**(`bugprone-*` / `cert-*` / `concurrency-*` / `cppcoreguidelines-*` 全カテゴリ警告 0)| MISRA C++ 2023 関連チェック完全 Pass |

> **本表の中核成果物達成:** `clang-17/tsan` および `gcc-13/tsan` プリセットで **UT-201-24(SPSC イベント配送 200 サイクル × 6 イベント / サイクル)が race condition 検出 0 で Pass** したこと。SDD §9 SEP-003 + RCM-019 で確定したメッセージパッシングによる race-free 設計が、UNIT-201 オーケストレータ層でも両コンパイラ × TSan で機械的に検証された(SRMP §6.1 3 層検証の TSan 層、Tyler 第 1 例 race condition 類型の機械的予防を Inc.1 オーケストレータ層に拡大)。**UT-201-21 で SDD §6.1 状態遷移許可表との完全一致を 88 組合せ網羅試験で機械的に検証成立**(SDD と実装の構造的乖離を防ぐ仕組み)。**UT-201-18 で HZ-001(電子モード+X 線ターゲット非挿入)の構造的不可能化を状態機械層で実体化**(RCM-001 中核)。

#### UNIT-202(Step 20 / CR-0008、本 v0.4 で追加、HZ-001 直接対応の中核)

| 試験 ID | ローカル骨格ビルド | CI cpp-build マトリクス(2026-04-30、run id `25166736078`)| 備考 |
|--------|------------------|----------------------------|------|
| UT-202-01〜02(初期状態 / 明示初期化)| 構文 OK | **全 9 ジョブ Pass ✅** | 既定 Light、明示 Electron/XRay |
| UT-202-03〜08(モード遷移正常系 6 件)| 構文 OK | **全 9 ジョブ Pass ✅** | Light↔Electron↔XRay の任意遷移 + Light 戻り + 同モード no-op |
| UT-202-09〜11(BeamState != Off で要求拒否)| 構文 OK | **全 9 ジョブ Pass ✅** | **HZ-001 直接対応**(Therac-25 Tyler 事故型「BeamState 確認漏れ」の構造的拒否)|
| UT-202-12〜15(エネルギー範囲外拒否 4 件)| 構文 OK | **全 9 ジョブ Pass ✅** | Electron(0.99/25.01 MeV)、XRay(4.99/25.01 MV)|
| UT-202-16〜17(エネルギー境界値受入)| 構文 OK | **全 9 ジョブ Pass ✅** | 境界値 ±0.01 まで網羅 |
| **UT-202-18〜19**(API ガード)| 構文 OK | **全 9 ジョブ Pass ✅** | **Electron API に XRay モード / XRay API に Electron モードを構造的拒否を機械検証成立**(Therac-25 Tyler 事故型「Electron モードで X 線エネルギー」発現経路の阻止) |
| UT-202-20(強い型 compile-time)| 構文 OK | **全 9 ジョブ Pass ✅** | Energy_MeV ↔ Energy_MV 暗黙変換不可 |
| **UT-202-21** | 構文 OK | **全 9 ジョブ Pass ✅。`clang-17/tsan` + `gcc-13/tsan` で 2 writer + 1 reader × 1000 イテレーション race detection 0 確認**| **HZ-002 機械検証成立**(`std::atomic<TreatmentMode>` の release-acquire ordering)|
| UT-202-22(コピー/ムーブ禁止)| 構文 OK | **全 9 ジョブ Pass ✅** | 4 trait compile-time |
| UT-202-23(lock-free 表明)| **`static_assert(std::atomic<TreatmentMode>::is_always_lock_free)` 通過** | **全 9 ジョブ Pass ✅** | HZ-007 構造的予防の build 時 fail-stop |
| UT-202-24(verify_mode_consistency)| 構文 OK | **全 9 ジョブ Pass ✅** | Step 21+ の TurntableManager 結線で 3 系統センサ依存判定に拡張予定 |
| **UT-202-25**(網羅試験 9 組合せ)| 構文 OK | **全 9 ジョブ Pass ✅** | **SDD §4.3 表との完全一致を機械検証成立**(全 9 通り許可)|
| UT-202-26(防御的 enum 値拒否)| 構文 OK | **全 9 ジョブ Pass ✅** | キャスト経由の不正値拒否 |
| (clang-tidy 全ファイル)| — | **clang-tidy (clang-17) Pass ✅**(`bugprone-*` / `cert-*` / `concurrency-*` / `cppcoreguidelines-*` 全カテゴリ警告 0)| MISRA C++ 2023 関連チェック完全 Pass |

> **本表の中核成果物達成:** `clang-17/tsan` および `gcc-13/tsan` プリセットで **UT-202-21(2 writer + 1 reader × 1000 イテレーション)が race condition 検出 0 で Pass** したこと。`std::atomic<TreatmentMode>` の release-acquire ordering が両コンパイラで機械的に検証された(SRMP §6.1 3 層検証の TSan 層、HZ-002 機械的予防を Inc.1 Manager 層に拡大)。**UT-202-25 で SDD §4.3 表との完全一致を 9 組合せ網羅試験で機械的に検証成立**(SDD ↔ 実装の構造的乖離防止メカニズムを 4 ユニット目に拡大)。**UT-202-09〜11 / UT-202-18〜19 で Therac-25 Tyler 事故型の三重エラー(Electron モードで X 線エネルギー / BeamState 確認漏れ / API オーバーロード分離による型安全)を構造的に拒否することを機械検証成立**(RCM-001 / HZ-001 直接対応の中核達成)。

### 8.3 カバレッジ実績

| ユニット ID | ステートメント | 分岐 | MC/DC | 備考 |
|------------|--------------|------|-------|------|
| UNIT-200 | (llvm-cov 未計測、Step 23+ 整備予定) | 同上 | — | UT 設計上は全機能を網羅(本書 §7.2 / §7.3) |
| UNIT-401 | (llvm-cov 未計測、Step 23+ 整備予定) | 同上 | — | UT 設計上は全公開 API + 境界値 + wrap-around + concurrent を網羅(本書 §7.2 / §7.3)|
| UNIT-201 | (llvm-cov 未計測、Step 23+ 整備予定) | 同上 | — | UT 設計上は公開 API 4 種 + 状態遷移 88 組合せ網羅 + イベントループ統合 + 並行処理を網羅(本書 §7.2 / §7.3) |
| UNIT-202 | (llvm-cov 未計測、Step 23+ 整備予定) | 同上 | — | UT 設計上は公開 API 5 種 + モード遷移 9 組合せ網羅 + BeamState/エネルギー範囲ガード + 強い型 compile-time + 並行処理を網羅(本書 §7.2 / §7.3) |
| UNIT-203 | (llvm-cov 未計測、Step 23+ 整備予定) | 同上 | — | UT 設計上は公開 API 5 種 + 許可フラグ消費型 + Lifecycle/state ガード + 並行処理(8 thread × 1000 セット)を網羅(本書 §7.2 / §7.3)。SDD §4.4「< 10 ms ビームオフ」の実測は IT-101(Inc.1 完了時)で実施 |
| UNIT-204 | (llvm-cov 未計測、Step 23+ 整備予定) | 同上 | — | UT 設計上は公開 API 3 種 + 補助 API 5 種 + 静的純粋関数 1 種 + lifecycle ガード 8 状態 × ドーズ値 3 通り = 24 組合せ網羅 + 大量積算 10^9 パルス + 並行処理(4 producer + reader / producer + setter)を網羅(本書 §7.2 / §7.3)。SDD §4.5「目標到達 → BeamOff < 1 ms」の実測は IT-101(Inc.1 完了時、UNIT-201/UNIT-203 結線後)で実施 |

### 8.4 不具合・逸脱

| 問題 ID | 内容 | 重大度 | 対応 | ステータス |
|--------|------|-------|------|----------|
| (CR-0005 修正コミット `d6a6bc7` で対応済) | UT-200-09 の `requires` 式を関数本体内に直接記述したため、clang-17 / gcc-13 で SFINAE 文脈外と判定されビルド失敗(AppleClang 15 では通っていた挙動差)| Minor(テストファイルの SFINAE パターン不適切、本番コードへの影響なし)| `requires` 式を `detail::HasPlusOp` / `HasMinusOp` / `HasMulOp` の 3 concept にラップして SFINAE 文脈に持ち込み、すべてのコンパイラで一貫した挙動を確保(`d6a6bc7`)| **解決(CI 全 9 ジョブ Pass で確認、2026-04-26)**|
| (Step 18 / CR-0006 範囲)| (CI 全 9 ジョブ Pass、不具合・逸脱なし、`24d77f2` で SHA 追記事後処理済)| Minor 以下 | — | **解決(2026-04-29)** |
| (Step 19 / CR-0007 範囲)| (CI 全 9 ジョブ Pass、不具合・逸脱なし、SHA 追記事後処理コミットで確定)| Minor 以下 | — | **解決(2026-04-30)** |
| (Step 20 / CR-0008 範囲)| (CI 全 9 ジョブ Pass、不具合・逸脱なし、SHA 追記事後処理コミットで確定)| Minor 以下 | — | **解決(2026-04-30)** |
| (Step 21 / CR-0009 範囲)| (CI 全 9 ジョブ Pass、不具合・逸脱なし、SHA 追記事後処理コミットで確定)| Minor 以下 | — | **解決(2026-04-30)** |
| **PRB-0001(#11)** | Step 22 (CR-0010) `3f18eee` push 後の CI で全 8 ジョブ linker error。UT-204-27 で `std::atomic<std::uint64_t>::is_lock_free()` メンバ関数(runtime)呼出が GCC 14 libstdc++ で `__atomic_is_lock_free` 未解決リンクエラー(libatomic 動的リンク要求)。Therac-25 主要因類型 F に部分該当(コンパイラ・標準ライブラリ更新時の挙動差は HZ-007 類型の現代版、ただし UT のみで本番コードへの影響なし)| **Major**(UT 自体の defect、本番コードへの影響なし、安全性影響なし)| `8032f61` で `EXPECT_TRUE(.is_lock_free())` 2 行を削除し `static_assert(is_always_lock_free)` 二重表明のみによる build 時検証に統一。UTPR §7.2 UT-204-27 行を整合更新(libatomic 回避理由を注記)| **解決(2026-05-01、CI run id `25195589849` 全 9 ジョブ Pass で確定、Issue #11 CLOSED)** |
| **PRB-0002(#12)** | Step 22 (CR-0010) `8032f61` push 後の CI で UT-204-30 (`DoseManager_Concurrency.ProducerProducerReader`) のみ ASSERT 失敗。`target=100000.0 cGy` が SRS-008 範囲 [0.01, 10000.0] cGy の 10 倍超で範囲外 → `set_dose_target` が `DoseOutOfRange` で正しく拒否したため `ASSERT_TRUE(...has_value())` が false で fail(本番コードの SRS-008 境界値検証が正しく機能していた証拠)。Therac-25 主要因類型該当なし(UT 実装上の数値ミス)| **Major**(UT 自体の数値ミス、本番コードへの影響なし、安全性影響なし、UT-204-22 / UT-204-25 が SRS-008 境界値検証で正しく機能していた)| `b1bdae4` で `rate` を `kRateOneCGy`(1.0) → `kRateRealistic`(0.05)に変更、`target` を 100000.0 → 10000.0 cGy(SRS-008 上限受入)に変更。target_pulses=200000、producer 4 × 10000 = 40000 パルスで reached=false 維持、並行 atomic 操作量 40000 を保持。UTPR §7.2 UT-204-30 行を整合更新 | **解決(2026-05-01、CI run id `25195589849` 全 9 ジョブ Pass で確定、Issue #12 CLOSED)** |
| (Step 22 / CR-0010 範囲、PRB-0001/0002 以外)| (ローカル骨格ビルド成功、CI 全 9 ジョブ Pass、`b1bdae4` で最終確定)| Minor 以下 | — | **解決(2026-05-01、run id `25195589849`)** |

### 8.5 未達項目と処置

- **llvm-cov 網羅率計測**: 本 v0.4 範囲では未実施。Step 21+ 後続ステップで CI ワークフローに統合する(SOUP-017 として CIL §5 で正式登録予定)。本 v0.4 では UT 設計上の網羅性確認(本書 §7.2 試験ケース定義 + UT-201-21 / UT-202-25 の網羅試験)で代替
- **モデル検査**: Inc.3(タスク同期)着手時に TLA+ / SPIN / Frama-C のいずれかを採用判断(SDD §10 未確定事項、SOUP-019 検討中として CIL §5 で記録済)
- **UNIT-202 と TurntableManager / BendingMagnetManager の結線**: 本 v0.4 では UNIT-202 単体のモード状態 + 遷移許可 + エネルギー範囲検証を実装。SDD §4.3 事後条件「TurntableManager::move_to + BendingMagnetManager::set_current のスケジュール」と `verify_mode_consistency` の「3 系統センサ依存判定」は Step 22+ で UNIT-205/206 実装時に結線・拡張する
- **UNIT-203 と ElectronGunSim / InterProcessChannel の結線**: 本 v0.5 では UNIT-203 単体の状態機械 + 許可フラグ消費型管理を実装。SDD §4.4「< 10 ms 電子銃電流 0 mA 確認」+「ウォッチドッグ」+「InterProcessChannel 経由 ElectronGunCurrent_mA 設定」は Step 23+ で UNIT-301/402 実装時に結線、Inc.1 完了時の **IT-101(< 10 ms 実測)** で SRS-101 の妥当性確認を行う
- **UNIT-204 と UNIT-201 / UNIT-203 の結線(observer pattern)**: 本 v0.6 では UNIT-204 単体のドーズ目標管理 + 積算 + 目標到達フラグまでを実装。SDD §4.5 サンプル「目標到達時 IF-U-002 経由で BeamOff 要求送信」は **`target_reached_` フラグまで** で構造を成立させ、UNIT-201 SafetyCoreOrchestrator の event_loop が `is_target_reached()` を観測して UNIT-203 BeamController の `request_beam_off()` を呼ぶ結線は Step 23+ の SafetyCoreOrchestrator dispatch 機構整備時に完成。SDD §4.5「目標到達 → BeamOff < 1 ms」の実測は Inc.1 完了時の **IT-101**(UNIT-204 → UNIT-201 → UNIT-203 連鎖の遅延測定)で SRS-010 の妥当性確認を行う

## 9. 結論

### UNIT-200(v0.1、再掲)

- [x] **UNIT-200 は受入基準(§5)を全て満たしている**(コーディング規約遵守、clang-tidy 0 警告、UT 全 24 件 CI Pass、ローカル骨格ビルドで `static_assert` 通過)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系・境界値・異常系・並行処理・タイミング・データフロー等を本書 §7.2 で網羅。資源使用量・分岐網羅は llvm-cov 整備後に追加計測予定)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`is_always_lock_free` `static_assert`、強い型 `explicit` + 算術非提供、`enum class` 型安全)

### UNIT-401(v0.2、Step 18)

- [x] **UNIT-401 は受入基準(§5)を全て満たしている**(SDD §4.13 v0.1.1 と完全一致、コーディング規約遵守、UT 全 13 件設計済、ローカル骨格ビルドで `static_assert(is_always_lock_free)` + power-of-two 制約 + default-constructible 制約 通過、CI 全 9 ジョブ Pass で確認済 `24d77f2`)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-401-01/03/04/10/11、境界値: UT-401-05/06/07、異常系: UT-401-06、並行処理: UT-401-12/13、データフロー: clang-tidy `bugprone-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<size_t>::is_always_lock_free` `static_assert`、Capacity = power-of-two `static_assert`、template type 制約 `is_default_constructible_v` / `is_move_constructible_v` / `is_move_assignable_v` `static_assert`)
- [x] 未解決問題なし(CI 全 9 ジョブ Pass で最終確定、2026-04-29)

### UNIT-201(v0.3、本 Step 19)

- [x] **UNIT-201 は受入基準(§5)を全て満たしている**(SDD §4.2 公開 API 4 種 + §6.1 状態遷移許可表と完全一致、コーディング規約遵守、UT 全 24 件設計済、ローカル骨格ビルドで `static_assert(std::atomic<LifecycleState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` + template instantiation(`SafetyCoreOrchestrator::EventQueue::capacity()`)通過。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-201-01〜17、境界値/網羅試験: UT-201-21 88 組合せ、異常系/不正遷移: UT-201-18/19/23、並行処理: UT-201-24 SPSC、データフロー: clang-tidy `bugprone-*`/`concurrency-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<LifecycleState>::is_always_lock_free` `static_assert` + `std::atomic<bool>::is_always_lock_free` `static_assert`、`enum class LifecycleState`/`LifecycleEventKind` 型安全(暗黙整数変換不可)、`shutdown()` の `noexcept` 表明)
- [x] 未解決問題なし(CI 全 9 ジョブ Pass で最終確定、2026-04-30、run id `25138602852`)

### UNIT-202(v0.4、本 Step 20、HZ-001 直接対応の中核)

- [x] **UNIT-202 は受入基準(§5)を全て満たしている**(SDD §4.3 公開 API 5 種 + モード遷移許可表と完全一致、コーディング規約遵守、UT 全 26 件設計済、ローカル骨格ビルドで `static_assert(std::atomic<TreatmentMode>::is_always_lock_free)` 通過 + 4 ターゲット完全ビルド成功。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-202-01〜08、境界値: UT-202-12〜17(エネルギー範囲)、異常系/不正条件: UT-202-09〜11(BeamState ガード)/ UT-202-18〜19(API ガード)/ UT-202-26(不正 enum)、並行処理: UT-202-21、データフロー: clang-tidy `bugprone-*`/`concurrency-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<TreatmentMode>::is_always_lock_free` `static_assert`、強い型 `Energy_MeV` / `Energy_MV` の compile-time 区別(暗黙変換不可)、`enum class TreatmentMode` 型安全)
- [x] 未解決問題なし(CI 全 9 ジョブ Pass で最終確定、2026-04-30、run id `25166736078`)

### UNIT-203(v0.5、Step 21、RCM-017 中核 / 許可フラグ消費型)

- [x] **UNIT-203 は受入基準(§5)を全て満たしている**(SDD §4.4 公開 API 3 種 + 補助 API 2 種 + 許可フラグ消費型設計と完全一致、コーディング規約遵守、UT 全 19 件設計済、ローカル骨格ビルドで `static_assert(std::atomic<BeamState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明通過。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-203-01〜03/06/08/09/10/19、異常系: UT-203-04(Lifecycle ガード 7 状態)/05(permission ガード)/07(二重照射拒否)/16(不正 enum)/18(state ガード)、並行処理: UT-203-13(8 thread × 1000 セット)/14(並列 beam_off)/17(setter+reader)、構造的: UT-203-11/12/15、データフロー: clang-tidy `bugprone-*`/`concurrency-*`)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<BeamState>::is_always_lock_free` + `std::atomic<bool>::is_always_lock_free` 二重 `static_assert`、`enum class BeamState` 型安全、`compare_exchange_strong` による消費型動作の機械検証)
- [x] 未解決問題なし(CI 全 9 ジョブ Pass で最終確定、2026-04-30、run id `25169361882`)。**SDD §4.4 < 10 ms ビームオフ実測は IT-101(Inc.1 完了時)で実施予定**

### UNIT-204(v0.6、本 Step 22、HZ-005 直接 + HZ-003 構造的排除の中核)

- [x] **UNIT-204 は受入基準(§5)を全て満たしている**(SDD §4.5 公開 API 3 種 + 補助 API 5 種 + 静的純粋関数 1 種 + 内部 4 atomic 設計と完全一致、コーディング規約遵守、UT 全 32 件設計済、ローカル骨格ビルドで `static_assert(std::atomic<std::uint64_t>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明 + `[1/4]..[4/4]` 完全ビルド通過。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-204-01〜04/13〜16/24、境界値: UT-204-05〜09/17〜19/22/23、異常系/不正条件: UT-204-07〜12/20/29、並行処理: UT-204-30(4 producer × 10000 + reader)/ UT-204-31(producer + setter)、構造的: UT-204-21(HZ-003 大量積算)/26〜28、SRS と SDD 表との網羅整合: UT-204-25(8 × 3 = 24 組合せ)、データフロー: clang-tidy `bugprone-*`/`concurrency-*`/`cppcoreguidelines-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<std::uint64_t>::is_always_lock_free` + `std::atomic<bool>::is_always_lock_free` 二重 `static_assert`、強い型 `DoseUnit_cGy` / `PulseCount` / `DoseRatePerPulse_cGy_per_pulse` の compile-time 区別(暗黙変換不可、UT-204-28 で機械検証)、`enum class LifecycleState` 型安全、`PulseCounter` 経由のみ accumulated_pulses_ にアクセス可能(SRS-D-009 整合))
- [x] 未解決問題なし予定(CI 全 9 ジョブ Pass は push 後 SHA 追記事後処理で最終確定)。**SDD §4.5「目標到達 → BeamOff < 1 ms」の実測は IT-101(Inc.1 完了時、UNIT-204 → UNIT-201 → UNIT-203 連鎖)で実施予定**

## 10. トレーサビリティマトリクス(本 v0.6 範囲)

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

### UNIT-401(Step 18 / CR-0006、CI 実績 2026-04-29 確定済)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-401 | UT-401-01〜04 | SDD §4.13(SPSC 基本動作)| RCM-019 | 全 9 ジョブ Pass(2026-04-29、run id 25135657465)|
| UNIT-401 | UT-401-05〜07 | SDD §4.13(境界値 / wrap-around)| RCM-019 | 同上 |
| UNIT-401 | UT-401-08 | SDD §7 SOUP-003/004 | **RCM-019 / HZ-002 / HZ-007** | 同上 |
| UNIT-401 | UT-401-09 | SDD §4.13(所有権)| RCM-019 | 同上 |
| UNIT-401 | UT-401-10〜11 | SDD §5.1 IF-U(メッセージ型独立性)/ §4.1(`enum class TreatmentMode`)| RCM-001 | 同上 |
| UNIT-401 | **UT-401-12** | SRS-D-009 / SDD §4.13 / §9 SEP-003 | **RCM-002 / RCM-019 / HZ-002** | **`tsan` プリセットで SPSC 100k race 0 確認(2026-04-29)** |
| UNIT-401 | UT-401-13 | SDD §4.13(burst pattern)| RCM-019 | 同上 |

### UNIT-201(Step 19 / CR-0007、CI 実績 2026-04-30 確定済)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-201 | UT-201-01 | SDD §4.2 / §6.1(初期状態)| RCM-001 | 全 9 ジョブ Pass(2026-04-30、run id `25138602852`)|
| UNIT-201 | UT-201-02〜03 | SRS-012 / SDD §4.2(自己診断)| RCM-001 / RCM-013 | 同上 |
| UNIT-201 | UT-201-04 | SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** | 同上 |
| UNIT-201 | UT-201-05 | SDD §4.2(所有権)| — | 同上 |
| UNIT-201 | UT-201-06〜07 | SDD §4.2(`shutdown()` signal-safe / `run_event_loop` 戻り値)| RCM-017 | 同上 |
| UNIT-201 | UT-201-08〜17 | SDD §6.1 状態遷移許可表(10 件論理遷移)| RCM-001 / RCM-017 | 同上 |
| UNIT-201 | UT-201-18 | SDD §6.1(Idle→BeamOn 不正遷移)| **RCM-001 / HZ-001** | 同上 |
| UNIT-201 | UT-201-19〜20 | SDD §6.1(終端 Halted / 緊急停止 ShutdownRequested)| RCM-001 / RCM-017 | 同上 |
| UNIT-201 | **UT-201-21** | **SDD §6.1 表 8 状態 × 11 イベント = 88 組合せ網羅** | **RCM-001** | 同上(SDD と実装の完全一致を機械検証成立)|
| UNIT-201 | UT-201-22〜23 | SDD §4.2 / §6.1 / §4.13(EventLoop 統合)| RCM-001 / RCM-019 | 同上 |
| UNIT-201 | **UT-201-24** | SRS-D-009 / SDD §9 SEP-003 / §4.13 | **RCM-002 / RCM-019 / HZ-002** | **`tsan` プリセットで SPSC イベント配送 race 0 確認(2026-04-30)** |

### UNIT-202(Step 20 / CR-0008、CI 実績 2026-04-30 確定済、HZ-001 直接対応の中核)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-202 | UT-202-01〜02 | SDD §4.3(初期状態)| RCM-001 | 全 9 ジョブ Pass(2026-04-30、run id `25166736078`)|
| UNIT-202 | UT-202-03〜08 | SRS-002 / SDD §4.3 表(モード遷移正常系 6 件)| RCM-001 | 同上 |
| UNIT-202 | UT-202-09〜11 | SRS-002 / SDD §4.3(BeamState 事前条件)| **RCM-001 / HZ-001** | 同上 |
| UNIT-202 | UT-202-12〜17 | SRS-004 / SDD §4.3(エネルギー範囲)| **RCM-008 / HZ-005** | 同上 |
| UNIT-202 | **UT-202-18〜19** | SDD §4.3(API ガード)| **RCM-001 / HZ-001** | 同上(**Therac-25 Tyler 事故型「Electron API に X 線」「XRay API に Electron」を構造的拒否を機械検証成立**)|
| UNIT-202 | UT-202-20 | SRS-D-002/003 / SDD §6.3(強い型 compile-time)| **RCM-008 / HZ-005** | 同上 |
| UNIT-202 | **UT-202-21** | SRS-D-009 / SDD §4.3 | **RCM-001 / RCM-002 / HZ-002** | **`tsan` プリセットで 2 writer + 1 reader race 0 確認(2026-04-30)** |
| UNIT-202 | UT-202-22 | SDD §4.3(所有権)| — | 同上 |
| UNIT-202 | UT-202-23 | SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** | 同上 |
| UNIT-202 | UT-202-24 | SRS-003 / SDD §4.3(verify_mode_consistency)| RCM-001 / RCM-005(構造的前提)| 同上 |
| UNIT-202 | **UT-202-25** | **SDD §4.3 表 3 状態 × 3 要求モード = 9 組合せ網羅** | **RCM-001** | 同上(SDD §4.3 表との完全一致を機械検証成立)|
| UNIT-202 | UT-202-26 | SDD §4.1(防御的 enum 値)| RCM-001 | 同上 |

### UNIT-203(Step 21 / CR-0009、CI 実績 2026-04-30 確定済、RCM-017 中核 / 許可フラグ消費型)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-203 | UT-203-01〜03 | SDD §4.4 / §4.1(初期状態 / permission)| RCM-017 | 全 9 ジョブ Pass(2026-04-30、run id `25169361882`)|
| UNIT-203 | UT-203-04 | SRS-007 / SDD §4.4(Lifecycle ガード 7 状態)| **RCM-017 / HZ-001** | 同上 |
| UNIT-203 | UT-203-05 | SDD §4.4(permission ガード)| RCM-017 / HZ-004 | 同上 |
| UNIT-203 | UT-203-06 | SRS-007 / SDD §4.4(成功遷移)| RCM-017 | 同上 |
| UNIT-203 | **UT-203-07** | SDD §4.4(1 回限り消費)| **RCM-017 / HZ-001 / HZ-004** | 同上(**Therac-25 Tyler 事故型「ビーム二重照射」の構造的拒否を機械検証成立**)|
| UNIT-203 | UT-203-08〜09 | SRS-101 / SDD §4.4(beam_off 正常系)| RCM-017 | 同上 |
| UNIT-203 | UT-203-10 | SDD §4.4(再 beam_on は permission 再設定必要)| **RCM-017 / HZ-001** | 同上 |
| UNIT-203 | UT-203-11 | SDD §4.4(所有権)| — | 同上 |
| UNIT-203 | UT-203-12 | SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** | 同上 |
| UNIT-203 | **UT-203-13** | SRS-D-009 / SDD §4.4 | **RCM-002 / RCM-017 / HZ-002 / HZ-004** | **`tsan` プリセットで 8 thread × 1000 セット race 0 + 各セット唯一 1 件成功 確認(2026-04-30)** |
| UNIT-203 | UT-203-14 | SRS-D-009 / SDD §4.4(並列 beam_off)| RCM-002 / RCM-017 | 同上 |
| UNIT-203 | UT-203-15 | SDD §4.1(BeamState 値安定性)| — | 同上 |
| UNIT-203 | UT-203-16 | SDD §4.4(防御的 Lifecycle 値)| RCM-017 | 同上 |
| UNIT-203 | UT-203-17 | SRS-D-009 / SDD §4.4(setter/reader 並行)| RCM-002 / RCM-017 / HZ-002 | 同上(`tsan` で 1 setter + 4 reader × 500 イテレーション race-free 確認)|
| UNIT-203 | UT-203-18 | SDD §4.4(state ガード、再エントラント防止)| RCM-017 | 同上 |
| UNIT-203 | UT-203-19 | SDD §4.4(beam_off フェイルセーフ性)| RCM-017 | 同上 |

### UNIT-204(本 Step 22 / CR-0010、HZ-005 直接 + HZ-003 構造的排除の中核、CI 実績 2026-05-01 確定済、run id `25195589849`、最終コミット `b1bdae4`)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-204 | UT-204-01〜02 | SDD §4.5 / §6.5(初期状態 / rate 注入)| RCM-003 / RCM-008 | 全 9 ジョブ Pass(2026-05-01、run id `25195589849`)|
| UNIT-204 | UT-204-03〜04 | SRS-008 / SDD §4.5(set_dose_target 正常系)| RCM-008 | 同上 |
| UNIT-204 | UT-204-05〜06 | SRS-008 / SDD §4.5(範囲境界受入)| **RCM-008 / HZ-005** | 同上 |
| UNIT-204 | UT-204-07〜09 | SRS-008 / SDD §4.5(範囲外拒否)| **RCM-008 / HZ-005** | 同上 |
| UNIT-204 | UT-204-10〜12 | SDD §4.5(lifecycle ガード 8 状態網羅)| RCM-001 | 同上 |
| UNIT-204 | UT-204-13 | SDD §4.5(累積リセット)| RCM-003 | 同上 |
| UNIT-204 | UT-204-14〜15 | SDD §4.5 / SRS-009(積算単調)| RCM-003 / RCM-004 | 同上 |
| UNIT-204 | UT-204-16 | SDD §4.5 / §6.5(単位変換 rate 整合)| **RCM-008 / HZ-005** | 同上 |
| UNIT-204 | UT-204-17〜19 | SRS-010 / SDD §4.5 / SRS-ALM-002(目標到達検知)| **RCM-008 / HZ-005** | 同上 |
| UNIT-204 | UT-204-20 | SDD §4.5(target 未設定保護)| RCM-001 | 同上 |
| UNIT-204 | **UT-204-21** | **SRS-009 / SDD §4.5(10^9 パルス積算余裕、5,800 万年連続照射相当)** | **RCM-003 / HZ-003** | 同上(**Yakima Class3 オーバフロー類型の構造的不可能化を実装層で機械検証成立**)|
| UNIT-204 | UT-204-22 | SRS-008(境界値 6 ケース機械検証)| **RCM-008 / HZ-005** | 同上 |
| UNIT-204 | UT-204-23 | SRS-008 / SDD §4.5(静的純粋関数)| RCM-008 | 同上 |
| UNIT-204 | UT-204-24 | SDD §4.5(reset() で全フィールドクリア)| RCM-001 | 同上 |
| UNIT-204 | **UT-204-25** | **SDD §4.5 表 8 lifecycle × 3 ドーズ値 = 24 組合せ網羅** | **RCM-001 / RCM-008** | 同上(SDD §4.5 表との完全一致を機械検証成立、5 ユニット目)|
| UNIT-204 | UT-204-26 | SDD §4.5(所有権)| — | 同上 |
| UNIT-204 | UT-204-27 | SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** | 同上(`static_assert(is_always_lock_free)` 二重表明、6 ユニット目)|
| UNIT-204 | UT-204-28 | SRS-D-002〜005 / SDD §6.3 / §6.5(強い型 compile-time)| **RCM-008 / HZ-005** | 同上 |
| UNIT-204 | UT-204-29 | SDD §4.1.1 / §4.5(DoseOverflow 検出)| RCM-003 / RCM-008 | 同上 |
| UNIT-204 | **UT-204-30** | SRS-D-009 / SDD §4.5 / §9 SEP-003(4 producer + 1 reader、rate=0.05 cGy/pulse、target=10000 cGy、target_pulses=200000) | **RCM-002 / RCM-004 / HZ-002** | **`tsan` プリセットで 4 producer × 10000 + 1 reader race 0 確認(2026-05-01、`b1bdae4` で SRS-008 範囲内化、PRB-0002 解決)** |
| UNIT-204 | UT-204-31 | SRS-D-009 / SDD §4.5(producer + setter)| RCM-002 / RCM-004 / HZ-002 | 同上(`tsan` で producer + setter race-free 確認)|
| UNIT-204 | UT-204-32 | SDD §4.1.1 / §6.2(ErrorCode 階層整合)| RCM-009 | 同上 |

## 11. 改訂履歴

| バージョン | 日付 | 変更内容 | 変更者 |
|----------|------|---------|--------|
| 0.1 | 2026-04-26 | 初版作成(Step 17 / CR-0005、本プロジェクト Issue #5)。**Inc.1 範囲のうち UNIT-200 CommonTypes に対する第 I 部(計画)+ 第 II 部(報告)を初期化**。**§3 実装ルール:** MISRA C++ 2023 + CERT C++ + C++20 + clang-tidy 全カテゴリ + Sanitizer 3 種 + `std::atomic` 共有変数規則(`PulseCounter` 経由のみ可)を確定。**§4 検証プロセス:** コードレビュー / 静的解析 / 動的解析 / UT / 網羅率計測の 5 手段を整理。**§4.2 並行処理 RCM の 3 層検証(SRMP §6.1 整合):** TSan + 境界値 + モデル検査(Inc.3 で導入判断)を必須化。**§5 受入基準 5 項目** + **§6 クラス C 追加受入基準 9 項目** + **§6.1 本プロジェクト固有追加基準 3 項目**(`is_always_lock_free` `static_assert`、強い型 `explicit` + 算術非提供、`enum class` 型安全)を確定。**§7.2 UNIT-200 試験ケース 24 件**(enum 系 5 件、強い型 7 件、PulseCount/Counter 7 件、Result 5 件)を定義し、`tests/unit/test_common_types.cpp` として実装済。**§8 実施サマリ:** ローカル骨格ビルド(`TH25_BUILD_TESTS=OFF`)で `static_assert(is_always_lock_free)` を AppleClang 15 で検証成功。CI cpp-build マトリクス全 8 ジョブ + clang-tidy ジョブの Pass は Step 17 push 後の SHA 追記事後処理で記録予定。**Therac-25 学習目的の中核達成:** UT-200-16 で 4 スレッド × 10000 increment による `PulseCounter` race-free 検証を TSan プリセットで実施(Tyler 第 1 例 race condition 類型の機械的検証手段確立)、UT-200-15 で `uint64_t::max()` 境界値検証(Yakima Class3 オーバフロー類型の構造的排除確認)、UT-200-12 + ヘッダ内 `static_assert(is_always_lock_free)` で HZ-007(レガシー前提喪失)構造的予防の機械検証成立 | k-abe |
| 0.2 | 2026-04-29 | **昇格(Step 18 / CR-0006、本プロジェクト Issue #6)**。**UNIT-401 InProcessQueue(SPSC lock-free ring buffer)を §3.2 実装対象に追加**(本 v0.2 計画範囲)、SDD-TH25-001 v0.1.1 への参照に更新(folly 採用 → 独自 Lamport SPSC 実装に確定)。**§7.2 UNIT-401 試験ケース 13 件追加**(初期 / publish-consume / 境界値 / wrap-around / lock-free 表明 / 所有権 / メッセージ型独立性 / 並行処理 SPSC 100k メッセージ / burst pattern)を定義し、`tests/unit/test_in_process_queue.cpp` として実装済。**累積試験ケース = 37 件**(UNIT-200 24 件 + UNIT-401 13 件)。**§8.1bis 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<size_t>::is_always_lock_free)` + Capacity = power-of-two `static_assert` + template type 制約 `static_assert` を AppleClang 15 で検証成功。CI cpp-build マトリクス全 9 ジョブ Pass は Step 18 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-401 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-401 行追加**(13 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UT-401-12 で 1 producer + 1 consumer × 100k メッセージ SPSC race-free 検証を `tsan` プリセットで実施する設計を確立(Tyler 第 1 例 race condition 類型に対する SDD §4.13 SPSC ring buffer 設計の機械的検証手段成立)。UT-401-08 + ヘッダ内 `static_assert(is_always_lock_free)` で HZ-007 構造的予防の機械検証を Inc.1 中核 MessageBus 層に拡張 | k-abe |
| 0.3 | 2026-04-30 | **昇格(Step 19 / CR-0007、本プロジェクト Issue #7)**。**UNIT-201 SafetyCoreOrchestrator(`LifecycleState` 8 状態機械 + イベントループ + 公開 API 4 種)を §3.2 実装対象に追加**(本 v0.3 計画範囲)。SDD-TH25-001 v0.1.1 §4.2 / §6.1 と完全一致する形で実装(`safety_core_orchestrator.hpp` + `.cpp`)。**§7.2 UNIT-201 試験ケース 24 件追加**(初期状態 / `init_subsystems` idempotency / lock-free 表明 / 所有権 / `shutdown` signal-safe / `run_event_loop` 戻り値 / 状態遷移許可表 10 件 / 不正遷移拒否 / 終端 Halted / 緊急停止 ShutdownRequested / **88 組合せ網羅試験 UT-201-21** / EventLoop 統合 2 件 / **SPSC イベント配送 concurrent 試験 UT-201-24**)を定義し、`tests/unit/test_safety_core_orchestrator.cpp` として実装済。**累積試験ケース = 61 件**(UNIT-200 24 件 + UNIT-401 13 件 + UNIT-201 24 件)。**§8.1ter 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<LifecycleState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` + template instantiation(`SafetyCoreOrchestrator::EventQueue::capacity()`)を AppleClang 15 で検証成功(`[1/3]..[3/3]` 完全ビルド)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 19 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-201 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-201 行追加**(24 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UT-201-21 の 88 組合せ網羅試験で SDD §6.1 状態遷移許可表との完全一致を機械検証(SDD と実装の構造的乖離を防ぐ仕組み)。UT-201-18(Idle→BeamOn 直接禁止)で HZ-001 の構造的不可能化を状態機械層で実体化(RCM-001 中核)。UT-201-24 で SPSC イベント配送 200 サイクル × 6 イベント/サイクル race-free 検証を `tsan` プリセットで実施する設計を確立(Tyler 第 1 例 race condition 類型に対する HZ-002 機械的予防を Inc.1 オーケストレータ層に拡大)。UT-201-04 + ヘッダ内 `static_assert(std::atomic<LifecycleState>::is_always_lock_free)` で HZ-007 構造的予防の機械検証を 3 ユニット目に拡大(UNIT-200/401 と同パターン継承)| k-abe |
| 0.5 | 2026-04-30 | **昇格(Step 21 / CR-0009、本プロジェクト Issue #9、RCM-017 中核 / 許可フラグ消費型)**。**UNIT-203 BeamController(ビームオン許可フラグの `compare_exchange_strong` による 1 回限り消費型管理 + BeamState 4 状態機械 + ビームオフ即時遷移)を §3.2 実装対象に追加**(本 v0.5 計画範囲)。SDD-TH25-001 v0.1.1 §4.4 と完全一致する形で実装(`beam_controller.hpp` + `.cpp`)。**§7.2 UNIT-203 試験ケース 19 件追加**(初期状態 / permission 操作 / Lifecycle ガード 7 状態 / permission ガード / 成功遷移 / **1 回限り消費(UT-203-07)** / beam_off 正常系 / 再 beam_on は permission 再設定必要 / 所有権 / lock-free 表明 / **8 thread × 1000 セット並列で各セット唯一 1 件成功(UT-203-13)** / 並列 beam_off / BeamState 値安定性 / 防御的 / setter/reader 並行 / state ガード / フェイルセーフ性)を定義し、`tests/unit/test_beam_controller.cpp` として実装済。**累積試験ケース = 106 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26 + UNIT-203 19)。**§8.1quinquies 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<BeamState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明を AppleClang 15 で検証成功。CI cpp-build マトリクス全 9 ジョブ Pass は Step 21 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-203 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-203 行追加**(19 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-203 は **RCM-017 中核ユニット**。UT-203-07(1 回限り消費)+ UT-203-13(8 thread × 1000 セット並列で各セット唯一 1 件成功)で、Therac-25 Tyler 事故型「許可確認とビームオン指令の race condition」「ビーム二重照射」の構造的予防(`compare_exchange_strong` による消費型設計)を機械検証する仕組みを確立(HZ-001/HZ-002/HZ-004 直接対応)。UT-203-12 + ヘッダ内 `static_assert` 二重表明で HZ-007 構造的予防を 5 ユニット目に拡大(UNIT-200/401/201/202/203)。SDD §4.4「< 10 ms ビームオフ保証」の実測は Inc.1 完了時の IT-101 で実施する旨を §8.5 に明記 | k-abe |
| 0.4 | 2026-04-30 | **昇格(Step 20 / CR-0008、本プロジェクト Issue #8、HZ-001 直接対応の中核)**。**UNIT-202 TreatmentModeManager(モード遷移許可表 + 強い型 Energy_MeV/MV エネルギー範囲検証 + BeamState 事前条件)を §3.2 実装対象に追加**(本 v0.4 計画範囲)。SDD-TH25-001 v0.1.1 §4.3 と完全一致する形で実装(`treatment_mode_manager.hpp` + `.cpp`)。**§7.2 UNIT-202 試験ケース 26 件追加**(初期状態 / モード遷移正常系 6 件 / BeamState != Off 拒否 3 件 / エネルギー範囲外拒否 4 件 / 境界値受入 2 件 / **API ガード 2 件(Therac-25 Tyler 事故型の構造的拒否)** / 強い型 compile-time / **並行処理 UT-202-21** / 所有権 / lock-free 表明 / verify_mode_consistency / **9 組合せ網羅試験 UT-202-25** / 防御的 enum 値拒否)を定義し、`tests/unit/test_treatment_mode_manager.cpp` として実装済。**累積試験ケース = 87 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26)。**§8.1quater 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<TreatmentMode>::is_always_lock_free)` を AppleClang 15 で検証成功(`[1/4]..[4/4]` 完全ビルド)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 20 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-202 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-202 行追加**(26 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-202 は **HZ-001 直接対応の中核ユニット**。UT-202-09〜11 / UT-202-18〜19 で Therac-25 Tyler 事故の三重エラー「Electron モードで X 線エネルギー」「BeamState 確認漏れ」「ターンテーブル位置不整合」の発現経路を構造的に拒否する設計を確立(RCM-001 / HZ-001 直接対応)。UT-202-25 の 9 組合せ網羅試験で SDD §4.3 表との完全一致を機械検証成立(SDD と実装の構造的乖離防止メカニズムを 4 ユニット目に拡大)。UT-202-12〜17 のエネルギー範囲境界値検証で SRS-004 範囲(Electron 1.0〜25.0 MeV / XRay 5.0〜25.0 MV)を機械的に保護(RCM-008、HZ-005 部分対応)。UT-202-21 の 2 writer + 1 reader 並行試験で `std::atomic<TreatmentMode>` の race-free 性を `tsan` プリセットで検証する設計を確立(HZ-002 機械的予防の継続)。UT-202-23 + ヘッダ内 `static_assert(std::atomic<TreatmentMode>::is_always_lock_free)` で HZ-007 構造的予防の機械検証を 4 ユニット目に拡大(UNIT-200/401/201 と同パターン継承)| k-abe |
| 0.6 | 2026-05-01 | **昇格(Step 22 / CR-0010、本プロジェクト Issue #10、HZ-005 直接 + HZ-003 構造的排除の中核)**。**UNIT-204 DoseManager(ドーズ目標管理 + ドーズ積算 + 目標到達検知 + 即時 BeamOff 指令フラグ)を §3.2 実装対象に追加**(本 v0.6 計画範囲)。SDD-TH25-001 v0.1.1 §4.5 と完全一致する形で実装(`dose_manager.hpp` + `.cpp`)。**SDD §6.5 で予告された強い型 `DoseRatePerPulse_cGy_per_pulse` を本 UNIT のヘッダで導入**(common_types.hpp は変更しない、`inc1-design-frozen` 維持)。**§7.2 UNIT-204 試験ケース 32 件追加**(初期状態 / 異なる rate / set_dose_target 正常系 + 範囲境界 + lifecycle ガード 8 状態網羅 + 累積リセット / on_dose_pulse 単発・単調・到達検知 / target 未設定保護 / **HZ-003 大量積算 10^9 パルス余裕(UT-204-21)** / 境界値 6 ケース機械検証 / 静的純粋関数 / reset / **24 組合せ網羅試験 UT-204-25** / 所有権 / lock-free 二重表明 / 強い型 compile-time / DoseOverflow 防御 / **並行処理 4 producer + reader(UT-204-30)+ producer + setter(UT-204-31)** / ErrorCode 階層整合)を定義し、`tests/unit/test_dose_manager.cpp` として実装済。**累積試験ケース = 138 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26 + UNIT-203 19 + UNIT-204 32)。**§8.1sexies 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<std::uint64_t>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明を AppleClang 15 で検証成功(`[1/4]..[4/4]` 完全ビルド)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 22 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-204 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-204 行追加**(32 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-204 は **HZ-005 直接対応 + HZ-003 構造的排除の中核ユニット**。UT-204-21 で 10^9 パルス積算でも uint64_t::max() 余裕(5,800 万年連続照射相当)を機械検証成立(Yakima Class3 8bit unsigned カウンタ 256 周期問題の構造的不可能化を実装層で完成、HZ-003 直接対応)。UT-204-25 の 24 組合せ網羅試験で SDD §4.5 表との完全一致を機械検証成立(SDD と実装の構造的乖離防止メカニズムを 5 ユニット目に拡大)。UT-204-22 のエネルギー範囲境界値検証で SRS-008 範囲(0.01〜10000.0 cGy)を機械的に保護(RCM-008、HZ-005 直接対応)。UT-204-30 の 4 producer + reader 並行試験で `std::atomic<uint64_t>::fetch_add(acq_rel)` + `std::atomic<bool>` release-acquire ordering の race-free 性を `tsan` プリセットで検証する設計を確立(HZ-002 機械的予防の継続)。UT-204-27 + ヘッダ内 `static_assert(std::atomic<std::uint64_t>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明で HZ-007 構造的予防の機械検証を 6 ユニット目に拡大(UNIT-200/401/201/202/203 と同パターン継承)。**UNIT-204 から UNIT-201/UNIT-203 への結線(observer pattern による BeamOff 指令)は Step 23+ で SafetyCoreOrchestrator dispatch 機構整備時に完成**、IT-101(Inc.1 完了時)で SRS-010「目標到達 → BeamOff < 1 ms」を実測する旨を §8.5 に明記 | k-abe |
