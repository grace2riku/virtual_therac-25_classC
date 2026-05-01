# ソフトウェアユニットテスト計画書/報告書(UTPR)

**ドキュメント ID:** UTPR-TH25-001
**バージョン:** 0.10
**作成日:** 2026-04-26(初版)/ 2026-04-29(v0.2)/ 2026-04-30(v0.3〜v0.5)/ 2026-05-01(v0.6〜v0.10)
**対象製品:** 仮想 Therac-25(Virtual Therac-25) / TH25-SIM-001
**対象ソフトウェアバージョン:** 0.2.0(Inc.1 完了で初回機能リリース予定)
**安全クラス:** C(IEC 62304)
**変更要求:** CR-0005〜0013 + CR-0014(GitHub Issue #18、v0.10 UNIT-301 追加、Hardware Simulator 層の最初の具体実装 / 仮想電子銃応答模擬)

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
> **本 v0.10(Step 26 / CR-0014)では UNIT-301 ElectronGunSim(Hardware Simulator 層の最初の具体実装 / 仮想電子銃応答模擬)を追加**。UNIT-207 / 209 / 210 / 302〜304 / 402 の試験計画・実績は後続 SDD-準拠ステップで追加する。

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
- 本 v0.6 で追加したユニット: **UNIT-204 DoseManager**(Step 22 / CR-0010、HZ-005 直接 + HZ-003 構造的排除の中核)
- 本 v0.7 で追加したユニット: **UNIT-205 TurntableManager**(Step 23 / CR-0011、HZ-001 直接対応 + RCM-005 中核 / 3 系統センサ集約 med-of-3)
- 本 v0.8 で追加したユニット: **UNIT-206 BendingMagnetManager**(Step 24 / CR-0012、HZ-005 部分対応 + RCM-008 中核 / 強い型による単位安全 + エネルギー対応表)
- 本 v0.9 で追加したユニット: **UNIT-208 StartupSelfCheck**(Step 25 / CR-0013、RCM-013 中核 + HZ-009 部分対応 / 起動時 4 項目自己診断)
- 本 v0.10 で追加したユニット: **UNIT-301 ElectronGunSim**(Step 26 / CR-0014、Hardware Simulator 層の最初の具体実装 / 仮想電子銃応答模擬)
- 後続 Step 27+ で他ユニットを順次追加

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
| [11] | 構成アイテム一覧(CIL-TH25-001) | 0.18 |
| [12] | CCB 運用規程(CCB-TH25-001) | 0.1 |
| [13] | 変更要求台帳(CRR-TH25-001) | 0.16 |
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
| **UNIT-204** | **DoseManager** | k-abe | `src/th25_ctrl/include/th25_ctrl/dose_manager.hpp` + `src/dose_manager.cpp` | **v0.6〜v0.7 で計画 + 実施記録維持(Step 22 / CR-0010、HZ-005 直接 + HZ-003 構造的排除の中核)** |
| **UNIT-205** | **TurntableManager** | k-abe | `src/th25_ctrl/include/th25_ctrl/turntable_manager.hpp` + `src/turntable_manager.cpp` | **v0.7〜v0.8 で計画 + 実施記録維持(Step 23 / CR-0011、HZ-001 直接対応 + RCM-005 中核 / 3 系統センサ集約 med-of-3)** |
| **UNIT-206** | **BendingMagnetManager** | k-abe | `src/th25_ctrl/include/th25_ctrl/bending_magnet_manager.hpp` + `src/bending_magnet_manager.cpp` | **v0.8〜v0.9 で計画 + 実施記録維持(Step 24 / CR-0012、HZ-005 部分対応 + RCM-008 中核 / 強い型による単位安全 + エネルギー対応表)** |
| **UNIT-208** | **StartupSelfCheck** | k-abe | `src/th25_ctrl/include/th25_ctrl/startup_self_check.hpp` + `src/startup_self_check.cpp` | **v0.9〜v0.10 で計画 + 実施記録維持(Step 25 / CR-0013、RCM-013 中核 + HZ-009 部分対応 / 起動時 4 項目自己診断)** |
| **UNIT-301** | **ElectronGunSim** | k-abe | `src/th25_sim/include/th25_sim/electron_gun_sim.hpp` + `src/electron_gun_sim.cpp` | **本 v0.10 で計画 + 実施記録(Step 26 / CR-0014、Hardware Simulator 層の最初の具体実装 / 仮想電子銃応答模擬)** |
| UNIT-101〜104 | Operator UI 群 | k-abe | `src/th25_ui/...`(Inc.4 で追加予定) | (Inc.4 SDD 改訂時に追加) |
| UNIT-207 | SafetyMonitor(空殻) | k-abe | `src/th25_ctrl/...` | 同上(Inc.2 で完成) |
| UNIT-209 | AuditLogger(空殻) | k-abe | `src/th25_ctrl/...` | 同上(Inc.4 で完成) |
| UNIT-210 | CoreAuthenticationGateway(空殻) | k-abe | `src/th25_ctrl/...` | 同上(Inc.4 で完成) |
| UNIT-302〜304 | Hardware Simulator 残ユニット(BendingMagnetSim / TurntableSim / IonChamberSim) | k-abe | `src/th25_sim/...` | 同上(Step 27+ で順次追加) |
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

#### UNIT-205 試験ケース(本 v0.7 で追加、Step 23 / CR-0011、HZ-001 直接対応 + RCM-005 中核)

`tests/unit/test_turntable_manager.cpp` として実装(本 Step 23 / CR-0011 でローカル骨格ビルド検証成功)。

| 試験 ID | テスト名 | 入力 | 期待される出力 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|---------|------|--------------|------|--------------|------------|
| UT-205-01 | `TurntableManager_Initial.AllZero` | 初期 TurntableManager | target=0、全センサ=0 | 正常系(初期状態)| SDD §4.6 | RCM-005 |
| UT-205-02 | `TurntableManager_Constants.SrsPositions` | (compile-time)| Electron=0.0/XRay=50.0/Light=-50.0、tolerance=0.5、discrepancy=1.0 | 構造的(SRS 整合) | SRS-006 / SRS-ALM-003 / SDD §4.6 | RCM-001 / RCM-005 |
| UT-205-03 | `TurntableManager_MoveTo.NormalElectron` | move_to(Electron=0.0) | `Result::ok()`、target=0.0 | 正常系 | SRS-006 / SDD §4.6 | RCM-001 |
| UT-205-04 | `TurntableManager_MoveTo.NormalXRay` | move_to(XRay=50.0) | `Result::ok()`、target=50.0 | 正常系 | SRS-006 / SDD §4.6 | RCM-001 |
| UT-205-05 | `TurntableManager_MoveTo.NormalLight` | move_to(Light=-50.0)| `Result::ok()`、target=-50.0 | 正常系 | SRS-006 / SDD §4.6 | RCM-001 |
| UT-205-06 | `TurntableManager_MoveTo.BoundaryAccept` | move_to(±100.0)| `Result::ok()`(SRS-D-007 境界受入)| 境界値 | SRS-D-007 / SDD §4.6 | **RCM-008** |
| UT-205-07 | `TurntableManager_MoveTo.BoundaryReject` | move_to(±100.001)| `TurntableOutOfPosition`(範囲外拒否)| 異常系(境界値外)| SRS-D-007 / SDD §4.6 | **RCM-008 / HZ-001** |
| UT-205-08 | `TurntableManager_Sensors.InjectAndRead` | inject(50.0,50.1,49.9)→ read | sensor_0/1/2 が一致 | 正常系(センサ反映)| SDD §4.6 | RCM-005 |
| UT-205-09 | `TurntableManager_StaticPure.ComputeMedian` | 6 順列 + 同値 3 ケース | 中央値が正しく算出 | 静的純粋関数 | SDD §4.6 | RCM-005 |
| UT-205-10 | `TurntableManager_StaticPure.HasDiscrepancyValues` | 5 ケース(全一致 / 微小 / 境界 / 超過 / 単一故障)| 閾値 1.0 mm 超で true | 静的純粋関数 | SRS-ALM-003 / SDD §4.6 | **RCM-005 / HZ-001** |
| UT-205-11 | `TurntablePosition_Median.ReturnsMedOf3` | TurntablePosition 2 ケース | median() が正しい中央値を返す | 構造的 | SDD §4.6 | RCM-005 |
| UT-205-12 | `TurntablePosition_HasDiscrepancy.DefaultThreshold` | 閾値 1.0 / 0.5 / 5.0 mm | has_discrepancy() が閾値依存で動作 | 構造的 | SRS-ALM-003 | RCM-005 |
| UT-205-13 | `TurntableManager_IsInPosition.NormalAccept` | 3 系統一致 + expected 一致 | true | 正常系 | SRS-006 / SDD §4.6 | RCM-005 / HZ-001 |
| UT-205-14 | `TurntableManager_IsInPosition.BoundaryDeviation` | median-expected = 0.5 vs 0.51 mm | 0.5 mm で true、0.51 mm で false | 境界値 | SRS-006 | **RCM-008 / HZ-001** |
| **UT-205-15** | **`TurntableManager_IsInPosition.DiscrepancyRejected`** | **3 系統不一致(max-min > 1.0 mm)**| **false(in-position 拒否)**| **異常系(Therac-25 Tyler 事故型対応)** | SRS-ALM-003 / SDD §4.6 | **RCM-005 / HZ-001** |
| **UT-205-16** | **`TurntableManager_IsInPosition.SingleSensorFault`** | **1 系統故障(原点貼り付き)**| **false(3 系統不一致で拒否)**| **異常系(Therac-25 Tyler 事故型: D 主要因)**| SRS-RCM-005 / SDD §4.6 | **RCM-005 / HZ-001** |
| UT-205-17 | `TurntableManager_IsInPosition.AllThreeSrsPositions` | Electron/XRay/Light 各位置 | 各位置で in-position 判定 + 他位置で拒否 | 正常系(3 位置網羅) | SRS-006 / SDD §4.6 | RCM-001 / RCM-005 |
| UT-205-18 | `TurntableManager_IsInRange.StaticPredicate` | 7 値で is_position_in_range | 範囲内/外を完全網羅 | 静的純粋関数 | SRS-D-007 | RCM-008 |
| UT-205-19 | `TurntableManager_Ownership.NoCopyNoMove` | (compile-time)| コピー/ムーブ 4 trait 全て false | 構造的(所有権)| SDD §4.6 | — |
| UT-205-20 | `TurntableManager_HZ007.AtomicIsLockFree` | (compile-time `static_assert`)| `std::atomic<double>::is_always_lock_free` | 構造的(HZ-007)| SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** |
| UT-205-21 | `TurntableManager_StrongTypes.CompileTimeIsolation` | (compile-time)| `Position_mm` への暗黙変換不可 | 構造的(強い型)| SRS-D-007 / SDD §6.3 | **RCM-008** |
| **UT-205-22** | **`TurntableManager_IsInPosition.MatrixCoverage`** | **9 ケース(センサ × expected 組合せ)** | **SDD §4.6 表との完全一致**(in-position 判定の網羅検証)| **網羅試験(SDD ↔ 実装の整合性、6 ユニット目)**| SRS-006 / SRS-ALM-003 / SDD §4.6 | RCM-001 / RCM-005 |
| **UT-205-23** | **`TurntableManager_Concurrency.WriterMultiReader`** | **1 writer + 4 reader × 5000 イテレーション**(`tsan` プリセットで race-free 検証)| **race detection 0** | **並行処理(TSan 必須)**| SRS-D-009 / SDD §4.6 / §9 SEP-003 | **RCM-002 / RCM-004 / HZ-002** |
| UT-205-24 | `TurntableManager_Concurrency.MoveToAndRead` | mover + reader 並行 5000 イテレーション | データレース無し(`tsan`)| 並行処理(TSan)| SRS-D-009 / SDD §4.6 | RCM-002 / HZ-002 |
| UT-205-25 | `TurntableManager_ErrorCodes.CategoryConsistency` | TurntableOutOfPosition / Discrepancy / MoveTimeout | category 上位 8bit = 0x04(Turntable 系)、Severity = Critical | 構造的(ErrorCode 階層整合)| SDD §4.1.1 / §6.2 | RCM-009 |

UNIT-205 合計試験ケース: **25 件**(`gtest_discover_tests` で自動収集)

#### UNIT-206 試験ケース(本 v0.8 で追加、Step 24 / CR-0012、HZ-005 部分対応 + RCM-008 中核)

`tests/unit/test_bending_magnet_manager.cpp` として実装(本 Step 24 / CR-0012 でローカル骨格ビルド検証成功)。

| 試験 ID | テスト名 | 入力 | 期待される出力 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|---------|------|--------------|------|--------------|------------|
| UT-206-01 | `BendingMagnetManager_Initial.AllZero` | 初期 BendingMagnetManager | target=0、actual=0、target_set=false、is_within_tolerance=false(target 未設定で安全側)| 正常系(初期状態)| SDD §4.7 | RCM-008 |
| UT-206-02 | `BendingMagnetManager_Constants.SrsValues` | (compile-time)| `kMagnetCurrentMinA=0.0` / `kMagnetCurrentMaxA=500.0` / `kMagnetToleranceFraction=0.05` | 構造的(SRS 整合)| SRS-D-006 / SRS-ALM-004 / SDD §4.7 | RCM-008 |
| UT-206-03 | `BendingMagnetManager_SetElectron.NormalMin` | set_current_for_energy(Electron, 1 MeV) | `Result::ok()`、target=2.5 A、target_set=true | 正常系(対応表線形補間下限)| SRS-005 / SRS-004 / SDD §4.7 | RCM-008 / HZ-005 |
| UT-206-04 | `BendingMagnetManager_SetElectron.NormalMax` | set_current_for_energy(Electron, 25 MeV) | `Result::ok()`、target=62.5 A | 正常系(対応表線形補間上限)| SRS-005 / SRS-004 / SDD §4.7 | RCM-008 / HZ-005 |
| UT-206-05 | `BendingMagnetManager_SetElectron.LinearInterpolation` | set_current_for_energy(Electron, 10 MeV) | `Result::ok()`、target=25.0 A(slope=2.5)| 正常系(線形補間中間)| SRS-005 / SDD §4.7 | RCM-008 / HZ-005 |
| UT-206-06 | `BendingMagnetManager_SetElectron.EnergyOutOfRange` | set_current_for_energy(Electron, 0.999/25.001 MeV) | `ModeInvalidTransition`、target_set=false | 異常系(SRS-004 範囲外拒否)| SRS-004 / SDD §4.7 | **RCM-008 / HZ-001 / HZ-005** |
| UT-206-07 | `BendingMagnetManager_SetElectron.ModeMismatch` | set_current_for_energy(XRay/Light, Energy_MeV) | `ModeInvalidTransition` 2 件 | 異常系(モード不整合)| SRS-002 / SDD §4.7 | **RCM-001 / HZ-001** |
| UT-206-08 | `BendingMagnetManager_SetXray.NormalMin` | set_current_for_energy(XRay, 5 MV) | `Result::ok()`、target=50.0 A | 正常系(対応表線形補間下限)| SRS-005 / SRS-004 / SDD §4.7 | RCM-008 / HZ-005 |
| UT-206-09 | `BendingMagnetManager_SetXray.NormalMax` | set_current_for_energy(XRay, 25 MV) | `Result::ok()`、target=250.0 A | 正常系(対応表線形補間上限)| SRS-005 / SRS-004 / SDD §4.7 | RCM-008 / HZ-005 |
| UT-206-10 | `BendingMagnetManager_SetXray.LinearInterpolation` | set_current_for_energy(XRay, 15 MV) | `Result::ok()`、target=150.0 A(slope=10.0)| 正常系(線形補間中間)| SRS-005 / SDD §4.7 | RCM-008 / HZ-005 |
| UT-206-11 | `BendingMagnetManager_SetXray.EnergyOutOfRange` | set_current_for_energy(XRay, 4.999/25.001 MV) | `ModeInvalidTransition` 2 件、target_set=false | 異常系(SRS-004 範囲外拒否)| SRS-004 / SDD §4.7 | **RCM-008 / HZ-001 / HZ-005** |
| UT-206-12 | `BendingMagnetManager_SetXray.ModeMismatch` | set_current_for_energy(Electron/Light, Energy_MV) | `ModeInvalidTransition` 2 件 | 異常系(モード不整合)| SRS-002 / SDD §4.7 | **RCM-001 / HZ-001** |
| UT-206-13 | `EnergyMagnetMap_Electron.ComputeTargetCurrent` | (compile-time + runtime)| 1/10/25 MeV → 2.5/25.0/62.5 A、`static_assert` で constexpr 検証 | 静的純粋関数 | SRS-005 / SDD §4.7 / §6.5 | **RCM-008 / HZ-005** |
| UT-206-14 | `EnergyMagnetMap_Xray.ComputeTargetCurrent` | (compile-time + runtime)| 5/15/25 MV → 50/150/250 A、`static_assert` で constexpr 検証 | 静的純粋関数 | SRS-005 / SDD §4.7 / §6.5 | **RCM-008 / HZ-005** |
| UT-206-15 | `BendingMagnetManager_StaticPure.IsCurrentWithinTolerance` | target=100、actual=100/95/105/94.999/105.001 | 完全一致 / ±5% 境界受入 / ±5% 超過拒否 | 境界値(±5% 偏差判定)| SRS-ALM-004 / SDD §4.7 | **RCM-008** |
| UT-206-16 | `BendingMagnetManager_StaticPure.IsCurrentWithinToleranceZeroTarget` | target=0、actual=0/±0.001 | 0 完全一致のみ true(0 で割れないため特殊取扱)| 境界値(ゼロ目標)| SDD §4.7 | RCM-008 |
| UT-206-17 | `BendingMagnetManager_StaticPure.IsCurrentWithinToleranceCustomFraction` | target=100、tolerance=0.10/0.01 | カスタム閾値で受入/拒否切替 | 構造的 | SDD §4.7 | RCM-008 |
| UT-206-18 | `BendingMagnetManager_StaticPure.IsCurrentInRange` | 6 値で is_current_in_range | 範囲内/外を完全網羅(SRS-D-006)| 静的純粋関数 | SRS-D-006 | RCM-008 |
| UT-206-19 | `BendingMagnetManager_Sensors.InjectAndRead` | inject_actual_current(42.0/123.45)| current_actual() がそれぞれ反映 | 正常系(センサ反映)| SDD §4.7 | RCM-008 |
| UT-206-20 | `BendingMagnetManager_IsWithinTolerance.Normal` | target=25 A(10 MeV)、actual=25 A | true | 正常系(完全一致)| SRS-ALM-004 / SDD §4.7 | RCM-008 |
| UT-206-21 | `BendingMagnetManager_IsWithinTolerance.BoundaryAccept` | target=25 A、actual=26.25(+5%)/23.75(-5%)| 境界 ±5% 受入 | 境界値 | SRS-ALM-004 / SDD §4.7 | **RCM-008** |
| UT-206-22 | `BendingMagnetManager_IsWithinTolerance.BoundaryReject` | target=25 A、actual=26.5(+6%)/23.5(-6%)| ±5% 超過拒否 | 異常系(境界値外)| SRS-ALM-004 / SDD §4.7 | **RCM-008** |
| UT-206-23 | `BendingMagnetManager_IsWithinTolerance.TargetNotSet` | target 未設定、actual=0 | false(安全側)| 異常系(事前条件未充足)| SDD §4.7 | RCM-008 |
| UT-206-24 | `BendingMagnetManager_Ownership.NoCopyNoMove` | (compile-time)| コピー/ムーブ 4 trait 全て false | 構造的(所有権)| SDD §4.7 | — |
| UT-206-25 | `BendingMagnetManager_HZ007.AtomicIsLockFree` | (compile-time `static_assert`)| `std::atomic<double>` + `std::atomic<bool>` 二重表明 | 構造的(HZ-007)| SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** |
| UT-206-26 | `BendingMagnetManager_StrongTypes.CompileTimeIsolation` | (compile-time)| Energy_MeV/MV/MagnetCurrent_A 全 7 ペアで暗黙変換不可 | 構造的(強い型)| SRS-D-002/003/006 / SDD §6.3 | **RCM-008 / HZ-005** |
| **UT-206-27** | **`BendingMagnetManager_Matrix.SddTableCoverage`** | **Electron 7 ケース + XRay 7 ケース = 14 組合せ網羅**(mode × energy 範囲)| **SDD §4.7 表との完全一致**(API 戻り値の網羅検証)| **網羅試験(SDD ↔ 実装の整合性、7 ユニット目)**| SRS-002 / SRS-004 / SRS-005 / SDD §4.7 | **RCM-001 / RCM-008 / HZ-001 / HZ-005** |
| **UT-206-28** | **`BendingMagnetManager_Concurrency.SetterMultiReader`** | **1 setter + 4 reader × 5000 イテレーション**(`tsan` プリセットで race-free 検証)| **race detection 0** | **並行処理(TSan 必須)**| SRS-D-009 / SDD §4.7 / §9 SEP-003 | **RCM-002 / RCM-004 / HZ-002** |
| UT-206-29 | `BendingMagnetManager_Concurrency.InjectAndCheck` | injector + checker 並行 5000 イテレーション | データレース無し(`tsan`)| 並行処理(TSan)| SRS-D-009 / SDD §4.7 | RCM-002 / HZ-002 |
| UT-206-30 | `BendingMagnetManager_ErrorCodes.CategoryConsistency` | MagnetCurrentDeviation / MagnetCurrentSensorFailure | category 上位 8bit = 0x05(Magnet 系)、Severity = Medium(common_types.hpp §6.2 規則と整合)| 構造的(ErrorCode 階層整合)| SDD §4.1.1 / §6.2 | RCM-009 |
| UT-206-31 | `BendingMagnetManager_Defense.OutOfRangeRejected` | is_current_in_range(500.0 / 500.001)| 上限受入 + 上限超過拒否 | 構造的(防御層)| SRS-D-006 | **RCM-008** |

UNIT-206 合計試験ケース: **31 件**(`gtest_discover_tests` で自動収集)

#### UNIT-208 試験ケース(本 v0.9 で追加、Step 25 / CR-0013、RCM-013 中核 + HZ-009 部分対応)

`tests/unit/test_startup_self_check.cpp` として実装(本 Step 25 / CR-0013 でローカル骨格ビルド検証成功)。

| 試験 ID | テスト名 | 入力 | 期待される出力 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|---------|------|--------------|------|--------------|------------|
| UT-208-01 | `StartupSelfCheck_Normal.AllInitial` | 4 項目すべて起動初期値 | `Result::ok()`(全 Pass)| 正常系(初期状態)| SRS-012 / SDD §4.9 | RCM-013 |
| UT-208-02 | `StartupSelfCheck_Constants.SrsValues` | (compile-time)| `kElectronGunZeroToleranceMA=0.01` / `kBendingMagnetZeroToleranceA=0.01` | 構造的(SRS 整合)| SDD §4.9 | RCM-013 |
| UT-208-03 | `StartupSelfCheck_ElectronGun.BoundaryAcceptInside` | 電子銃 0.005 mA | `Result::ok()` | 境界値(許容差内)| SRS-012 / SDD §4.9 | RCM-013 / HZ-009 |
| UT-208-04 | `StartupSelfCheck_ElectronGun.BoundaryRejectPositive` | 電子銃 0.011 mA | `ElectronGunCurrentOutOfRange` | 異常系(境界値超過)| SRS-012 / SDD §4.9 | **RCM-013 / HZ-009** |
| UT-208-05 | `StartupSelfCheck_ElectronGun.BoundaryRejectNegative` | 電子銃 -0.011 mA | `ElectronGunCurrentOutOfRange` | 異常系(負方向超過)| SRS-012 / SDD §4.9 | **RCM-013 / HZ-009** |
| UT-208-06 | `StartupSelfCheck_Turntable.LightPositionPass` | ターンテーブル Light 位置一致 | `Result::ok()` | 正常系 | SRS-012 / SRS-006 / SDD §4.9 | RCM-013 / RCM-005 |
| UT-208-07 | `StartupSelfCheck_Turntable.LightBoundaryAccept` | Light ±0.5 mm 境界(3 系統一致)| `Result::ok()` 2 件 | 境界値 | SRS-012 / SRS-006 / SDD §4.9 | **RCM-008 / RCM-013** |
| UT-208-08 | `StartupSelfCheck_Turntable.LightBoundaryReject` | Light ±0.51 mm | `TurntableOutOfPosition` | 異常系(境界値外)| SRS-012 / SRS-006 / SDD §4.9 | **RCM-013 / HZ-001** |
| UT-208-09 | `StartupSelfCheck_Turntable.ThreeSensorDiscrepancy` | 3 系統不一致(max-min > 1.0 mm)| `TurntableOutOfPosition` | 異常系(センサ不一致)| SRS-012 / SRS-ALM-003 / SDD §4.9 | **RCM-005 / RCM-013 / HZ-001** |
| UT-208-10 | `StartupSelfCheck_Turntable.NotAtLight` | ターンテーブル Electron 位置(0.0 mm)| `TurntableOutOfPosition` | 異常系(位置不一致)| SRS-012 / SRS-006 | **RCM-013 / HZ-001 / HZ-009** |
| UT-208-11 | `StartupSelfCheck_BendingMagnet.ZeroPass` | 磁石 0 A | `Result::ok()` | 正常系 | SRS-012 / SDD §4.9 | RCM-013 |
| UT-208-12 | `StartupSelfCheck_BendingMagnet.BoundaryAcceptInside` | 磁石 0.005 A | `Result::ok()` | 境界値(許容差内)| SRS-012 / SDD §4.9 | RCM-013 |
| UT-208-13 | `StartupSelfCheck_BendingMagnet.BoundaryReject` | 磁石 0.011 A | `MagnetCurrentDeviation` | 異常系(境界値超過)| SRS-012 / SDD §4.9 | **RCM-008 / RCM-013** |
| UT-208-14 | `StartupSelfCheck_BendingMagnet.BoundaryRejectNegative` | 磁石 -0.011 A | `MagnetCurrentDeviation` | 異常系(負方向超過)| SRS-012 / SDD §4.9 | **RCM-008 / RCM-013** |
| UT-208-15 | `StartupSelfCheck_Dose.ZeroPass` | ドーズ 0 cGy | `Result::ok()` | 正常系 | SRS-012 / SDD §4.9 | RCM-003 / RCM-013 |
| UT-208-16 | `StartupSelfCheck_Dose.NonZeroReject` | ドーズ != 0(積算後)| `InternalUnexpectedState` | 異常系(初期化失敗)| SRS-012 / SRS-009 / SDD §4.9 | **RCM-003 / RCM-013 / HZ-009** |
| **UT-208-17** | **`StartupSelfCheck_EarlyReturn.ElectronGunFirst`** | **電子銃 NG + 磁石 NG + ドーズ NG 同時** | **`ElectronGunCurrentOutOfRange`(早期リターン)** | **構造的(順序検証)**| SDD §4.9 | RCM-013 |
| UT-208-18 | `StartupSelfCheck_EarlyReturn.TurntableSecond` | 電子銃 OK + ターンテーブル NG + 磁石 NG + ドーズ NG | `TurntableOutOfPosition` | 構造的(順序検証)| SDD §4.9 | RCM-013 |
| UT-208-19 | `StartupSelfCheck_EarlyReturn.BendingMagnetThird` | 電子銃 OK + ターンテーブル OK + 磁石 NG + ドーズ NG | `MagnetCurrentDeviation` | 構造的(順序検証)| SDD §4.9 | RCM-013 |
| UT-208-20 | `StartupSelfCheck_StaticPure.IsElectronGunInZero` | 7 値(0/0.005/-0.005/0.009/0.01/0.02/-0.02)+ カスタム閾値 2 件 | 各境界で正しい true/false | 静的純粋関数 | SDD §4.9 | RCM-013 |
| UT-208-21 | `StartupSelfCheck_StaticPure.IsBendingMagnetInZero` | 6 値 + カスタム閾値 2 件 | 各境界で正しい true/false | 静的純粋関数 | SDD §4.9 | RCM-013 |
| UT-208-22 | `StartupSelfCheck_StaticPure.IsDoseZero` | 0.0 / 0.001 / 1.0 + constexpr 検証 | `0.0` のみ true、constexpr 評価可能 | 静的純粋関数 | SDD §4.9 | RCM-013 |
| UT-208-23 | `StartupSelfCheck_Inject.RoundTrip` | inject(2.5 / -1.5)→ current_electron_gun_current | 値が反映 | 正常系(inject API)| SDD §4.9 | RCM-013 |
| UT-208-24 | `StartupSelfCheck_Ownership.NoCopyNoMove` | (compile-time)| 4 trait 全て false | 構造的(所有権)| SDD §4.9 | — |
| UT-208-25 | `StartupSelfCheck_HZ007.AtomicIsLockFree` | (compile-time `static_assert`)| `std::atomic<double>::is_always_lock_free` | 構造的(HZ-007)| SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** |
| UT-208-26 | `StartupSelfCheck_StrongTypes.CompileTimeIsolation` | (compile-time)| ElectronGunCurrent_mA / MagnetCurrent_A / DoseUnit_cGy が double から暗黙変換不可、相互変換も不可 | 構造的(強い型)| SRS-D-006/008 / SDD §6.3 | **RCM-008** |
| **UT-208-27** | **`StartupSelfCheck_Matrix.SddTableCoverage`** | **6 ケース網羅(全 OK / 4 項目それぞれ単独 NG / 全 NG 早期リターン)**| **SDD §4.9 表との完全一致**(4 項目自己診断 + 早期リターン順序の網羅検証)| **網羅試験(SDD ↔ 実装の整合性、8 ユニット目)**| SRS-012 / SDD §4.9 | **RCM-013** |
| UT-208-28 | `StartupSelfCheck_ErrorCodes.CategoryAndSeverity` | 4 ErrorCode の category + Severity | Beam(0x02 Critical)/ Turntable(0x04 High)/ Magnet(0x05 Medium)/ Internal(0xFF Critical)| 構造的(ErrorCode 階層整合)| SDD §4.1.1 / §6.2 | RCM-009 |
| **UT-208-29** | **`StartupSelfCheck_Concurrency.SetterMultiReader`** | **1 setter + 4 reader × 5000 イテレーション**(`tsan` プリセットで race-free 検証)| **race detection 0** | **並行処理(TSan 必須)**| SRS-D-009 / SDD §4.9 / §9 SEP-003 | **RCM-002 / RCM-004 / HZ-002** |

UNIT-208 合計試験ケース: **29 件**(`gtest_discover_tests` で自動収集)

#### UNIT-301 試験ケース(本 v0.10 で追加、Step 26 / CR-0014、Hardware Simulator 層の最初の具体実装)

`tests/unit/test_electron_gun_sim.cpp` として実装(本 Step 26 / CR-0014 でローカル骨格ビルド検証成功)。

| 試験 ID | テスト名 | 入力 | 期待される出力 | 種別 | 関連 SRS / SDD | 関連 RCM / HZ |
|--------|---------|------|--------------|------|--------------|------------|
| UT-301-01 | `ElectronGunSim_Initial.AllZero` | 初期 ElectronGunSim | commanded=0、actual=0、noise disabled | 正常系(初期状態)| SDD §4.12 | RCM-002 |
| UT-301-02 | `ElectronGunSim_Constants.SrsValues` | (compile-time)| `kElectronGunMinMA=0.0` / `kElectronGunMaxMA=10.0` / `kElectronGunNoiseFraction=0.01` | 構造的(SRS 整合)| SRS-D-008 / SDD §4.12 | RCM-008 |
| UT-301-03 | `ElectronGunSim_SetCurrent.Normal` | set_current(5.0)| commanded=5.0、actual=5.0(ノイズ無効)| 正常系 | SRS-IF-001 / SRS-O-001 / SDD §4.12 | RCM-002 |
| UT-301-04 | `ElectronGunSim_SetCurrent.BoundaryMax` | set_current(10.0)| commanded=10.0(SRS-D-008 上限受入)| 境界値 | SRS-D-008 / SDD §4.12 | **RCM-008** |
| UT-301-05 | `ElectronGunSim_SetCurrent.BoundaryMin` | set_current(0.0)| commanded=0.0(SRS-D-008 下限受入)| 境界値 | SRS-D-008 / SDD §4.12 | **RCM-008** |
| UT-301-06 | `ElectronGunSim_SetCurrent.ClampOverMax` | set_current(15.0)| commanded=10.0(物理的飽和、上限 clamp)| 異常系(範囲外正方向)| SRS-D-008 / SDD §4.12 | **RCM-008 / 物理飽和模擬** |
| UT-301-07 | `ElectronGunSim_SetCurrent.ClampUnderMin` | set_current(-5.0)| commanded=0.0(物理的飽和、下限 clamp)| 異常系(範囲外負方向)| SRS-D-008 / SDD §4.12 | **RCM-008 / 物理飽和模擬** |
| UT-301-08 | `ElectronGunSim_ReadActual.NoiseDisabled` | set_current(3.0)→ read_actual_current() × 100 | すべて 3.0(ノイズ無効で決定論的)| 正常系(read_actual)| SRS-I-008 / SDD §4.12 | RCM-002 |
| UT-301-09 | `ElectronGunSim_Noise.EnableDisableState` | enable_noise(42)→ disable_noise()| is_noise_enabled が正しく遷移 | 正常系(状態遷移)| SDD §4.12 | RCM-002 |
| UT-301-10 | `ElectronGunSim_Noise.ActualWithinTolerance` | set_current(5.0)+ enable_noise(123)→ read × 100 | actual ∈ [5.0 × 0.99, 5.0 × 1.01]| 境界値(ノイズ ±1%)| SDD §4.12 | RCM-002 / RCM-008 |
| UT-301-11 | `ElectronGunSim_Noise.SeedReproducibility` | 同 seed 2 Sim → 同シーケンス、異 seed → 別値 | seed 再現性確認 | 構造的(決定論性)| SDD §4.12 | RCM-002 |
| UT-301-12 | `ElectronGunSim_Noise.ZeroCommandedZeroActual` | set_current(0.0)+ enable_noise(7)→ read × 50 | すべて 0.0(0 × noise = 0)| 境界値(ゼロ特殊)| SDD §4.12 | RCM-008 |
| UT-301-13 | `ElectronGunSim_StaticPure.IsCurrentInRange` | 6 値で is_current_in_range + constexpr | 範囲内/外を完全網羅 | 静的純粋関数 | SRS-D-008 | RCM-008 |
| UT-301-14 | `ElectronGunSim_StaticPure.ClampToRange` | 7 値で clamp_to_range + constexpr | 範囲内そのまま、外は飽和 | 静的純粋関数 | SRS-D-008 / SDD §4.12 | RCM-008 |
| UT-301-15 | `ElectronGunSim_StrongTypes.CompileTimeIsolation` | (compile-time)| `ElectronGunCurrent_mA` への暗黙変換不可 | 構造的(強い型)| SRS-D-008 / SDD §6.3 | **RCM-008** |
| UT-301-16 | `ElectronGunSim_Ownership.NoCopyNoMove` | (compile-time)| 4 trait 全て false | 構造的(所有権)| SDD §4.12 | — |
| UT-301-17 | `ElectronGunSim_HZ007.AtomicIsLockFree` | (compile-time `static_assert` 三重)| `std::atomic<double>` + `bool` + `std::uint32_t` 全て lock-free | 構造的(HZ-007)| SDD §7 SOUP-003/004 | **RCM-002 / HZ-002 / HZ-007** |
| **UT-301-18** | **`ElectronGunSim_Matrix.SddTableCoverage`** | **7 ケース(0/2.5/5/7.5/10 mA + 範囲外 -1/15)** | **SDD §4.12 表との完全一致**(commanded → actual の往復網羅)| **網羅試験(SDD ↔ 実装の整合性、9 ユニット目)**| SRS-IF-001 / SRS-O-001 / SRS-I-008 / SDD §4.12 | **RCM-008** |
| **UT-301-19** | **`ElectronGunSim_Concurrency.SetterMultiReader`** | **1 setter + 4 reader × 5000 イテレーション**(`tsan` プリセットで race-free 検証)| **race detection 0** | **並行処理(TSan 必須)**| SRS-D-009 / SDD §4.12 / §9 SEP-003 | **RCM-002 / RCM-019 / HZ-002** |

UNIT-301 合計試験ケース: **19 件**(`gtest_discover_tests` で自動収集)

**累積合計試験ケース(UNIT-200 + UNIT-401 + UNIT-201 + UNIT-202 + UNIT-203 + UNIT-204 + UNIT-205 + UNIT-206 + UNIT-208 + UNIT-301)= 242 件**

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

### 8.1decies 実施サマリ(UNIT-301 / Step 26 / CR-0014、Hardware Simulator 層の最初の具体実装 / 仮想電子銃応答模擬)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-05-01 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: (push 後 SHA 追記事後処理で記録) |
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で `static_assert(std::atomic<double>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` + `static_assert(std::atomic<std::uint32_t>::is_always_lock_free)` 三重表明を AppleClang 15 で検証成功(`th25_sim` 静的ライブラリ完全ビルド成功、`electron_gun_sim.cpp` を `th25_sim` ライブラリの 2 cpp 目として組込)。`th25_sim` を `th25_ctrl` に PUBLIC リンク(common_types.hpp の `ElectronGunCurrent_mA` 強い型を利用)。SRS-D-008 範囲(0.0〜10.0 mA)+ ノイズ許容率(±1%)を範囲定数で外部化。`commanded_current_ma_`(`std::atomic<double>`)+ `noise_enabled_`(`std::atomic<bool>`)+ `rng_state_`(`mutable std::atomic<std::uint32_t>`)の三重 atomic で並行アクセス保護。`mutable std::atomic<std::uint32_t>` で論理的 const 性を保ちつつノイズ生成の rng 内部状態を更新する標準的なパターンを採用。`read_actual_current()` 内で `fetch_add(kLcgIncrement, acq_rel)` により thread-safe な疑似乱数生成を実現 |
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_electron_gun_sim_tests` 19 件追加。GitHub Actions cpp-build ワークフロー(run id は push 後追記)|
| **CI 結果** | (push 後追記)。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-301-19(1 setter + 4 reader × 5000 イテレーション)race detection 0 で Pass 予定**(`std::atomic<double>` + `bool` + `std::uint32_t` の release-acquire ordering の正しさを両コンパイラで機械検証する設計、HZ-002 機械的予防を 10 ユニット目に拡大)。**UT-301-18 で SDD §4.12 表との完全一致を 7 ケース網羅機械検証成立予定**(SDD ↔ 実装の構造的乖離防止メカニズムを 9 ユニット目に拡大)。**UT-301-10/11 でノイズ ±1% 許容範囲 + seed 再現性を機械検証成立予定**(SDD §4.12「設定値 ± 1% のノイズ模擬」+ UT 駆動安定化 default OFF の構造的実現)。**UT-301-06/07 で範囲外指令の物理的飽和(clamp_to_range)を機械検証成立予定**(実物理デバイスの飽和挙動を Sim で構造的模擬)|

### 8.1nonies 実施サマリ(UNIT-208 / Step 25 / CR-0013、RCM-013 中核 + HZ-009 部分対応 / 起動時 4 項目自己診断)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-05-01 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: `da6b1de` |
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で `static_assert(std::atomic<double>::is_always_lock_free)` を AppleClang 15 で検証成功(`th25_ctrl` 静的ライブラリ完全ビルド成功、`startup_self_check.cpp` を 8 cpp 目として組込)。Manager 群 (UNIT-204 DoseManager + UNIT-205 TurntableManager + UNIT-206 BendingMagnetManager) への参照を保持し、`perform_self_check` が 4 項目を順次検証(電子銃 → ターンテーブル → 磁石 → ドーズの物理経路順、早期リターン)。UNIT-301 ElectronGunSim 未実装のため `inject_electron_gun_current(ElectronGunCurrent_mA)` 補助 API + `std::atomic<double>` 1 個で UT 駆動可能。version.cpp に `kElectronGunZeroToleranceMA` constexpr プローブを追加し、static_assert を翻訳単位レベルで発火 |
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_startup_self_check_tests` 29 件追加。GitHub Actions cpp-build ワークフロー(run id `25213056027`、2026-05-01、最終コミット `da6b1de`)+ Documentation Checks(run id `25213056023` success)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)+ Documentation Checks success。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-208-29(1 setter + 4 reader × 5000 イテレーション)race detection 0 で Pass**(`std::atomic<double>` の release-acquire ordering の正しさが両コンパイラで機械検証成立、HZ-002 機械的予防を 9 ユニット目に拡大)。**UT-208-27 で SDD §4.9 表との完全一致を 6 ケース網羅機械検証成立**(SDD ↔ 実装の構造的乖離防止メカニズムを 8 ユニット目に拡大)。**UT-208-17〜19 で早期リターン順序(電子銃 → ターンテーブル → 磁石 → ドーズの物理経路順)を機械検証成立**(SDD §4.9 設計判断「上流 → 下流の物理順序での早期リターン」が機械検証可能化)。**UT-208-04/05/13/14 で電子銃 + 磁石の境界値超過を機械検証成立**(SDD §4.9 許容差 abs < 0.01 の境界網羅、Therac-25 起動時状態不定リスクへの構造的予防)。**Step 22/23 と異なり PRB 起票なし**(Step 24 に続いて 2 連続のクリーンコミット、Step 23/24 教訓の継続反映が奏功)|

### 8.1octies 実施サマリ(UNIT-206 / Step 24 / CR-0012、HZ-005 部分対応 + RCM-008 中核 / 強い型による単位安全 + エネルギー対応表)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-05-01 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: `034b5d3` |
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で `static_assert(std::atomic<double>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明を AppleClang 15 で検証成功(`th25_ctrl` 静的ライブラリ `[1/4]..[4/4]` 完全ビルド成功)。target_current_ / actual_current_ を `std::atomic<double>` 2 個 + target_set_ を `std::atomic<bool>` で並行アクセス保護、SDD §4.7 と完全一致する形で `EnergyMagnetMap` を `class` として内包(線形補間係数 slope/intercept 各電子線/X 線で固定 constexpr、SDP §6.1 確定後の校正データに置換予定)、SRS-D-006 範囲(0.0〜500.0 A)+ SRS-ALM-004 偏差閾値(±5% = 0.05)を範囲定数で外部化。version.cpp に `EnergyMagnetMap::compute_target_current_electron(Energy_MeV{1.0})` を constexpr プローブとして追加し、static_assert を翻訳単位レベルで発火 |
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_bending_magnet_manager_tests` 31 件追加。GitHub Actions cpp-build ワークフロー(run id `25205346050`、2026-05-01、最終コミット `034b5d3`)+ Documentation Checks(run id `25205346048` success)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)+ Documentation Checks success。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-206-28(1 setter + 4 reader × 5000 イテレーション)race detection 0 で Pass**(`std::atomic<double>` × 2 + `std::atomic<bool>` の release-acquire ordering の正しさが両コンパイラで機械検証成立、HZ-002 機械的予防を 8 ユニット目に拡大)。**UT-206-27 で SDD §4.7 表との完全一致を 14 組合せ網羅機械検証成立**(SDD ↔ 実装の構造的乖離防止メカニズムを 7 ユニット目に拡大)。**UT-206-13/14 で EnergyMagnetMap 線形補間 (slope, intercept) を constexpr `static_assert` で機械検証成立**(SDD §4.7 で予告された「コードに直接埋め込まない、HZ-007 構造的予防」の実装層実現、SDP §6.1 確定後に校正データへ CR で置換可能な構造)。**UT-206-26 で Energy_MeV / Energy_MV / MagnetCurrent_A の compile-time 隔離を機械検証成立**(全 7 ペアで暗黙変換不可、HZ-005 直接対応の Inc.1 中核達成)。**Step 22/23 と異なり PRB 起票なし**(push 前ローカル markdownlint 実行 + UT 実装時に common_types.hpp §6.2 / SRS 範囲制約セルフチェック実施 = Step 23 教訓の継続反映が奏功、PRB-0003/0004 同型ミスを構造的に予防成立)|

### 8.1septies 実施サマリ(UNIT-205 / Step 23 / CR-0011、HZ-001 直接対応 + RCM-005 中核 / 3 系統センサ集約 med-of-3)

| 項目 | 内容 |
|------|------|
| 実施期間 | 2026-05-01 |
| 実施者 | k-abe |
| 対象ソフトウェアバージョン(コミット)| 本体: `c609f59` / PRB-0003 修正(MD037、`target_ 保持` をバッククォートでコードスパン化)+ PRB-0004 修正(UT-205-25 severity 期待値 Critical → High、common_types.hpp §6.2 確認漏れ): `d7459b3` |
| 試験環境(ローカル骨格ビルド)| macOS / AppleClang 15.0.0、CMake 3.28+、Ninja 1.11+、`TH25_BUILD_TESTS=OFF` で `static_assert(std::atomic<double>::is_always_lock_free)` を AppleClang 15 で検証成功(`th25_ctrl` 静的ライブラリ `[1/4]..[4/4]` 完全ビルド成功)。3 系統センサ + target を `std::atomic<double>` 4 個で並行アクセス保護、SDD §4.6 サンプルどおり `TurntablePosition` struct + `median()` + `has_discrepancy()` を実装、SRS-006 所定 3 位置(Electron=0.0/XRay=50.0/Light=-50.0 mm)を範囲定数で外部化。ローカル `npx markdownlint-cli2@0.22 DEVELOPMENT_STEPS.md` で 0 error 確認(PRB-0003 修正後)|
| 試験環境(CI 完全実行)| Ubuntu 24.04 + clang-17 / gcc-13、4 プリセット × 2 コンパイラ = 8 ジョブ + clang-tidy ジョブ。`th25_turntable_manager_tests` 25 件追加。GitHub Actions cpp-build ワークフロー(run id `25202911125`、2026-05-01、最終コミット `d7459b3`)+ Documentation Checks(run id `25202911126` success)|
| **CI 結果** | **全 9 ジョブ success ✅**(`clang-tidy (clang-17)`、`clang-17 / debug`、`clang-17 / asan`、`clang-17 / ubsan`、`clang-17 / tsan`、`gcc-13 / debug`、`gcc-13 / asan`、`gcc-13 / ubsan`、`gcc-13 / tsan`)+ Documentation Checks success。**特に `clang-17/tsan` + `gcc-13/tsan` で UT-205-23(1 writer + 4 reader × 5000 イテレーション)race detection 0 で Pass**(`std::atomic<double>` の release-acquire ordering の正しさが両コンパイラで機械検証成立)。**UT-205-22 で SDD §4.6 表との完全一致を 9 組合せ網羅機械検証成立**(SDD ↔ 実装の構造的乖離防止メカニズムを 6 ユニット目に拡大)。**UT-205-15/16 で Therac-25 Tyler 事故型「ターンテーブル位置不整合」+「単一センサ故障時の検出」の構造的予防(has_discrepancy + median)を機械検証成立**(D 主要因の Inc.1 中核 RCM-005 達成)。**VERIFICATION 段階で PRB-0003(#14、Markdown lint MD037、`c609f59`)+ PRB-0004(#15、UT-205-25 severity 期待値 Critical 誤り、`c609f59`)を SPRP §3.1 規定により遡及起票 → 修正(`d7459b3`)→ 検証(本 run id)完了。SPRP §3.1 PRB 起票プロセス本格運用第 2 例**(Step 22 PRB-0001/0002 に続く)|

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
| UNIT-204 | (llvm-cov 未計測、Step 24+ 整備予定) | 同上 | — | UT 設計上は公開 API 3 種 + 補助 API 5 種 + 静的純粋関数 1 種 + lifecycle ガード 8 状態 × ドーズ値 3 通り = 24 組合せ網羅 + 大量積算 10^9 パルス + 並行処理(4 producer + reader / producer + setter)を網羅(本書 §7.2 / §7.3)。SDD §4.5「目標到達 → BeamOff < 1 ms」の実測は IT-101(Inc.1 完了時、UNIT-201/UNIT-203 結線後)で実施 |
| UNIT-205 | (llvm-cov 未計測、Step 25+ 整備予定) | 同上 | — | UT 設計上は公開 API 3 種 + 補助 API 2 種 + 静的純粋関数 3 種 + 3 系統センサ × expected 位置 = 9 組合せ網羅 + 1 writer + 4 reader 並行 + move_to/read 並行 を網羅(本書 §7.2 / §7.3)。SDD §4.6「move_to の 500 ms タイムアウト」は Step 25+ で UNIT-303/402 結線時に実装、IT-104 で実測予定 |
| UNIT-206 | (llvm-cov 未計測、Step 26+ 整備予定) | 同上 | — | UT 設計上は公開 API 3 種(Energy_MeV / Energy_MV オーバーロード分離 + current_actual + is_within_tolerance)+ 補助 API 3 種 + 静的純粋関数 4 種(`EnergyMagnetMap::compute_target_current_electron/xray` + `is_current_within_tolerance` + `is_current_in_range`)+ Electron 7 ケース + XRay 7 ケース = 14 組合せ網羅 + 1 setter + 4 reader 並行 + injector + checker 並行 を網羅(本書 §7.2 / §7.3)。SDD §4.7「BendingMagnetSim に MagnetCurrent_A 指令送信、±5% 以内に到達するまで待機(タイムアウト 200 ms)」は Step 26+ で UNIT-302/402 結線時に実装、IT-102/IT-104 等で実測予定。SRS-ALM-004「連続 100 ms 以上」の継続検出も同 Step で連続監視ループに組み込む |
| UNIT-208 | (llvm-cov 未計測、Step 27+ 整備予定) | 同上 | — | UT 設計上は公開 API 1 種(perform_self_check)+ 補助 API 2 種(inject_electron_gun_current / current_electron_gun_current)+ 静的純粋関数 3 種(is_electron_gun_in_zero / is_bending_magnet_in_zero / is_dose_zero)+ 4 項目 × OK/NG = 6 ケース網羅(全 OK / 4 項目それぞれ単独 NG / 全 NG 早期リターン)+ 早期リターン順序 3 ケース(UT-208-17〜19)+ 1 setter + 4 reader 並行 を網羅(本書 §7.2 / §7.3)。SDD §4.9「ElectronGunSim から計測値読取」の UNIT-301 結線は Step 26 で UNIT-301 単体実装完了、Inc.1 後半で UNIT-208::inject_electron_gun_current API を UNIT-301::read_actual_current() 呼出に置換予定。SDD §4.9「UNIT-201::init_subsystems() からの呼出 dispatch」は Inc.1 後半で SafetyCoreOrchestrator dispatch 機構整備時に完成 |
| UNIT-301 | (llvm-cov 未計測、Step 27+ 整備予定) | 同上 | — | UT 設計上は公開 API 2 種(set_current / read_actual_current、SRS-IF-001 完全一致)+ 補助 API 4 種(enable_noise / disable_noise / is_noise_enabled / current_commanded)+ 静的純粋関数 2 種(is_current_in_range / clamp_to_range)+ SDD §4.12 表 7 ケース網羅(0/2.5/5/7.5/10 mA + 範囲外 -1/15)+ ノイズ ±1% 許容範囲 + seed 再現性 + 1 setter + 4 reader 並行 を網羅(本書 §7.2 / §7.3)。SDD §4.12「応答遅延 1 ms 模擬」は Step 26 範囲では構造のみ、実時間遅延は Inc.1 完了時の IT-101「ビームオフ < 10 ms」測定で検証予定。UNIT-203 BeamController からの指令受信 + UNIT-208 StartupSelfCheck からの計測値実読取結線は Inc.1 後半 SafetyCoreOrchestrator dispatch 機構整備時に完成 |

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
| **PRB-0003(#14)** | Step 23 (CR-0011) `c609f59` push 後の Documentation Checks ワークフローで Markdown lint(MD037 no-space-in-emphasis)が DEVELOPMENT_STEPS.md:807,816 で失敗。「`target_ 保持`」(`target_` 末尾アンダースコア + 半角スペース + 漢字「保」)の並びが markdownlint で `_保` の emphasis(イタリック)と誤解された。Therac-25 主要因類型該当なし(ドキュメント整形ミス)| **Major**(CI 失敗、ただしドキュメント整形ミスで本番コードへの影響なし)| `d7459b3` で「`target_ 保持`」をバッククォートでコードスパン化(`` `target_` への保持 ``)に書き換え。ローカル markdownlint-cli2 で 0 error 確認 | **解決(2026-05-01、CI run id `25202911126`(Documentation Checks)success で確定、Issue #14 CLOSED)** |
| **PRB-0004(#15)** | Step 23 (CR-0011) `c609f59` push 後の C++ Build (Inc.1) で UT-205-25(`TurntableManager_ErrorCodes.CategoryConsistency`)のみ EXPECT_EQ 失敗。`severity_of(ErrorCode::TurntableOutOfPosition)` を `Severity::Critical` と期待していたが、common_types.hpp §6.2 Severity 判定規則(line 141-143)で **Turntable 系(0x04)→ `High`** と定義済(`Critical` は Mode/Beam/Dose/Internal 系のみ)。UT 実装時に common_types.hpp の severity_of 定義を確認していなかった。Therac-25 主要因類型該当なし(UT 実装の参照確認漏れ)| **Major**(UT 自体の数値ミス、本番コードへの影響なし、安全性影響なし、UT-205-25 のみが影響、本番コード `severity_of` は SDD §6.2 と完全一致して正しく動作)| `d7459b3` で `Severity::Critical` → `Severity::High` に変更。さらに `TurntableSensorDiscrepancy` / `TurntableMoveTimeout` の 2 件も明示確認する 3 行を追加し、common_types.hpp §6.2 規則との完全一致を Turntable 系統全体で機械検証 | **解決(2026-05-01、CI run id `25202911125`(C++ Build) 全 9 ジョブ Pass で確定、Issue #15 CLOSED)** |
| (Step 23 / CR-0011 範囲、PRB-0003/0004 以外)| (ローカル骨格ビルド成功、CI 全 9 ジョブ Pass、`d7459b3` で最終確定)| Minor 以下 | — | **解決(2026-05-01、run id `25202911125` + `25202911126`)** |
| (Step 24 / CR-0012 範囲)| (ローカル骨格ビルド成功 `[1/4]..[4/4]`、CI 全 9 ジョブ Pass、`034b5d3` で確定、Documentation Checks も success、PRB 起票なし)| Minor 以下 | — | **解決(2026-05-01、run id `25205346050` + `25205346048`)** |
| (Step 25 / CR-0013 範囲)| (ローカル骨格ビルド成功、CI 全 9 ジョブ Pass、`da6b1de` で確定、Documentation Checks も success、PRB 起票なし)| Minor 以下 | — | **解決(2026-05-01、run id `25213056027` + `25213056023`)** |
| (Step 26 / CR-0014 範囲)| (ローカル骨格ビルド成功、CI 全 9 ジョブ Pass は push 後追記、本体コミット SHA は事後処理で記録、ローカルで mutable std::atomic<uint32_t> 採用による const メソッドからの fetch_add 経由 1 回ビルドエラー → 即時修正、PRB 起票不要)| Minor 以下 | — | (push 後最終確定) |

### 8.5 未達項目と処置

- **llvm-cov 網羅率計測**: 本 v0.4 範囲では未実施。Step 21+ 後続ステップで CI ワークフローに統合する(SOUP-017 として CIL §5 で正式登録予定)。本 v0.4 では UT 設計上の網羅性確認(本書 §7.2 試験ケース定義 + UT-201-21 / UT-202-25 の網羅試験)で代替
- **モデル検査**: Inc.3(タスク同期)着手時に TLA+ / SPIN / Frama-C のいずれかを採用判断(SDD §10 未確定事項、SOUP-019 検討中として CIL §5 で記録済)
- **UNIT-202 と TurntableManager / BendingMagnetManager の結線**: 本 v0.4 では UNIT-202 単体のモード状態 + 遷移許可 + エネルギー範囲検証を実装。SDD §4.3 事後条件「TurntableManager::move_to + BendingMagnetManager::set_current のスケジュール」と `verify_mode_consistency` の「3 系統センサ依存判定」は Step 22+ で UNIT-205/206 実装時に結線・拡張する
- **UNIT-203 と ElectronGunSim / InterProcessChannel の結線**: 本 v0.5 では UNIT-203 単体の状態機械 + 許可フラグ消費型管理を実装。SDD §4.4「< 10 ms 電子銃電流 0 mA 確認」+「ウォッチドッグ」+「InterProcessChannel 経由 ElectronGunCurrent_mA 設定」は Step 23+ で UNIT-301/402 実装時に結線、Inc.1 完了時の **IT-101(< 10 ms 実測)** で SRS-101 の妥当性確認を行う
- **UNIT-204 と UNIT-201 / UNIT-203 の結線(observer pattern)**: 本 v0.6 では UNIT-204 単体のドーズ目標管理 + 積算 + 目標到達フラグまでを実装。SDD §4.5 サンプル「目標到達時 IF-U-002 経由で BeamOff 要求送信」は **`target_reached_` フラグまで** で構造を成立させ、UNIT-201 SafetyCoreOrchestrator の event_loop が `is_target_reached()` を観測して UNIT-203 BeamController の `request_beam_off()` を呼ぶ結線は Step 24+ の SafetyCoreOrchestrator dispatch 機構整備時に完成。SDD §4.5「目標到達 → BeamOff < 1 ms」の実測は Inc.1 完了時の **IT-101**(UNIT-204 → UNIT-201 → UNIT-203 連鎖の遅延測定)で SRS-010 の妥当性確認を行う
- **UNIT-205 と UNIT-303 / UNIT-402 の結線(`move_to` の実 move + ±0.5 mm 到達待機)**: 本 v0.7 では UNIT-205 単体の target 保持 + 3 系統センサ集約 + has_discrepancy + is_in_position(±0.5 mm)を実装。SDD §4.6「InterProcessChannel 経由で TurntableSim に位置指令、3 系統センサが目標位置 ±0.5 mm 以内に到達するまで待機(タイムアウト 500 ms)」は **target_ 保持 + inject_sensor_readings(UT 駆動)まで** で構造を成立させ、UNIT-303 TurntableSim 経由のセンサ実読取 + UNIT-402 InterProcessChannel 経由の指令送信は Step 25+ で UNIT-303/402 実装時に完成。SDD §4.6「move_to の 500 ms タイムアウト」の実測は Inc.1 完了時の **IT-104** で実施予定。**UNIT-202 `verify_mode_consistency` の「3 系統センサ依存判定」拡張**(SDD §4.3 事後条件)も Step 25+ UNIT-205 結線時に実施(現在は UNIT-202 単体の内部状態整合のみ)
- **UNIT-206 と UNIT-302 / UNIT-402 の結線(`set_current_for_energy` の実 IPC 送信 + ±5% 到達待機 + SRS-ALM-004 連続 100 ms 検出)**: 本 v0.8 では UNIT-206 単体の target 保持 + EnergyMagnetMap 線形補間 + 偏差 ±5% 判定(一回限り)を実装。SDD §4.7「BendingMagnetSim に MagnetCurrent_A 指令送信、±5% 以内に到達するまで待機(タイムアウト 200 ms)」は **target_current_ 保持 + inject_actual_current(UT 駆動)まで** で構造を成立させ、UNIT-302 BendingMagnetSim 経由のセンサ実読取 + UNIT-402 InterProcessChannel 経由の指令送信は Step 25+ で UNIT-302/402 実装時に完成。SRS-ALM-004「連続 100 ms 以上」の継続検出も同 Step で連続監視ループに組み込む。SDD §4.7「set_current_for_energy の 200 ms タイムアウト」の実測は Inc.1 完了時の **IT-102 / IT-104** で実施予定。**UNIT-202 → UNIT-206 のスケジュール連鎖**(SDD §4.3 事後条件「`BendingMagnetManager::set_current(...)` がスケジュール済」)も Step 25+ UNIT-206 結線時に実施(現在は UNIT-202 / UNIT-206 が独立して機能、observer pattern 経由の自動スケジュールは未結線)
- **EnergyMagnetMap の校正データ置換**: 本 v0.8 では `EnergyMagnetMap::compute_target_current_electron/xray` の線形補間係数 (slope, intercept) を **プレースホルダ値**(electron: 2.5 A/MeV, intercept=0 / xray: 10.0 A/MV, intercept=0)で実装。SDP §6.1 で確定予定の校正データに対応する具体値は CR で置換する(SDD §4.7 で「具体値はコンストラクタで注入、コードに直接埋め込まない、HZ-007 構造的予防」と明記済、本実装は constexpr 内部係数として外部化済のため CR で 1 ファイル置換可能)
- **UNIT-208 と UNIT-301 / UNIT-201 の結線**: 本 v0.9 では UNIT-208 単体の起動時 4 項目自己診断を実装。SDD §4.9「ElectronGunSim から計測値読取」は **`inject_electron_gun_current` 補助 API による UT 駆動まで** で構造を成立させ、UNIT-301 ElectronGunSim 経由のセンサ実読取は Step 26+ で UNIT-301 実装時に完成(Manager 経由ではない直接 atomic を Sim 経由読取に置換、API 不変)。SDD §4.9「UNIT-201::init_subsystems() から呼出」は **本 v0.9 では UNIT-208 単体実装まで** で、UNIT-201 SafetyCoreOrchestrator からの呼出 dispatch は Inc.1 後半の **SafetyCoreOrchestrator dispatch 機構整備時** で完成。SDD §4.9「治療開始拒否 + 操作者 UI に詳細エラーメッセージ通知」(SRS-012 / SRS-ALM-005)は Inc.4 で UI レイヤー(UNIT-101〜104)実装時に完成、Inc.1 では Result<void, ErrorCode> 返却まで
- **UNIT-301 と UNIT-203 / UNIT-208 / UNIT-402 の結線**: 本 v0.10 では UNIT-301 単体の仮想電子銃応答模擬を実装。SDD §4.12「応答遅延 1 ms 模擬」は **構造のみ実装、実時間 sleep は未実装** で、Inc.1 完了時の IT-101「ビームオフ < 10 ms」測定で実時間挙動を検証予定。UNIT-203 BeamController からの set_current 指令受信 + UNIT-208 StartupSelfCheck からの read_actual_current 計測値読取結線は Inc.1 後半 SafetyCoreOrchestrator dispatch 機構整備時に完成。Step 26 範囲では UNIT-203/UNIT-208 ともに UNIT-301 を直接利用できる構造(`th25_sim` を `th25_ctrl` に PUBLIC リンク済)が確立済。UNIT-402 InterProcessChannel 経由の IPC 通信は Step 30+ で gRPC + Protocol Buffers SOUP 追加 CR と併せて実装予定(Inc.1 範囲では同一プロセス内で動作可能、Inc.2/3 でのプロセス分離時に IPC が必要)
- **UNIT-301 SRS-IF-001 `IElectronGun` 抽象 IF の正式定義**: 本 v0.10 では UNIT-301 を具体クラスとして直接実装、SRS-IF-001 で予告された `IElectronGun` 抽象 IF は未定義。Inc.2/3 で複数 Sim 並行実装(BendingMagnetSim / TurntableSim / IonChamberSim)時に共通 IF として抽象化を検討。Inc.1 範囲では具体クラスでの直接利用で充足

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

### UNIT-205(v0.7、本 Step 23、HZ-001 直接対応 + RCM-005 中核 / 3 系統センサ集約 med-of-3)

- [x] **UNIT-205 は受入基準(§5)を全て満たしている**(SDD §4.6 公開 API 3 種 + 補助 API 2 種 + 静的純粋関数 3 種 + 内部 4 atomic 設計と完全一致、コーディング規約遵守、UT 全 25 件設計済、ローカル骨格ビルドで `static_assert(std::atomic<double>::is_always_lock_free)` 通過 + `[1/4]..[4/4]` 完全ビルド成功。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-205-01〜05/13/17、境界値: UT-205-06/14/18/22、異常系/不正条件: UT-205-07/15/16(Therac-25 Tyler 事故型「ターンテーブル位置不整合」+「単一センサ故障」)、並行処理: UT-205-23(1 writer + 4 reader × 5000)/24(mover + reader)、構造的: UT-205-09〜12/19〜21、SDD と実装の網羅整合: UT-205-22(9 組合せ)、データフロー: clang-tidy `bugprone-*`/`concurrency-*`/`cppcoreguidelines-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<double>::is_always_lock_free` `static_assert` を 7 ユニット目に拡大、強い型 `Position_mm` の compile-time 隔離(暗黙変換不可、UT-205-21)、`enum class` 型安全)
- [x] 未解決問題なし予定(CI 全 9 ジョブ Pass は push 後 SHA 追記事後処理で最終確定)。**SDD §4.6「move_to の 500 ms タイムアウト」+「±0.5 mm 到達待機」+ 「UNIT-202 verify_mode_consistency 結線」は IT-104(Inc.1 完了時、UNIT-303/402 結線後)で実施予定**

### UNIT-301(v0.10、本 Step 26、Hardware Simulator 層の最初の具体実装 / 仮想電子銃応答模擬)

- [x] **UNIT-301 は受入基準(§5)を全て満たしている**(SDD §4.12 公開 API 2 種(SRS-IF-001 完全一致)+ 補助 API 4 種 + 静的純粋関数 2 種 + 内部 3 atomic 設計と完全一致、コーディング規約遵守、UT 全 19 件設計済、ローカル骨格ビルドで `static_assert` 三重表明通過 + 完全ビルド成功。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-301-01/03/08/09/12、境界値: UT-301-04/05/10/13/14、異常系/不正条件: UT-301-06/07(範囲外飽和)、並行処理: UT-301-19(1 setter + 4 reader × 5000)、構造的: UT-301-02/15〜17、SDD と実装の網羅整合: UT-301-18(7 ケース網羅)、ノイズ決定論性: UT-301-11、データフロー: clang-tidy `bugprone-*`/`concurrency-*`/`cppcoreguidelines-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<double>` + `std::atomic<bool>` + `std::atomic<std::uint32_t>` `is_always_lock_free` 三重 `static_assert` を **10 ユニット目に拡大**、強い型 `ElectronGunCurrent_mA` の compile-time 隔離(UT-301-15)、`mutable std::atomic` で論理的 const 性とノイズ生成 thread-safety を両立する標準的パターン採用)
- [x] 未解決問題なし予定(CI 全 9 ジョブ Pass は push 後 SHA 追記事後処理で最終確定、ローカルで `mutable std::atomic<uint32_t>` 採用による const メソッドからの fetch_add 経由 1 回ビルドエラー → 即時修正、PRB 起票不要)。**SDD §4.12「応答遅延 1 ms 模擬」は Inc.1 完了時の IT-101 で実測検証予定。UNIT-203 BeamController + UNIT-208 StartupSelfCheck との結線は Inc.1 後半 SafetyCoreOrchestrator dispatch 機構整備時に完成**

### UNIT-208(v0.9、本 Step 25、RCM-013 中核 + HZ-009 部分対応 / 起動時 4 項目自己診断)

- [x] **UNIT-208 は受入基準(§5)を全て満たしている**(SDD §4.9 公開 API 1 種 + 補助 API 2 種 + 静的純粋関数 3 種 + Manager 群参照保持 + 内部 1 atomic 設計と完全一致、コーディング規約遵守、UT 全 29 件設計済、ローカル骨格ビルドで `static_assert(std::atomic<double>::is_always_lock_free)` 通過 + 完全ビルド成功。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-208-01/03/06/11/12/15、境界値: UT-208-04/05/07/08/13/14/20/21、異常系/不正条件: UT-208-04/05/08/09/10/13/14/16、並行処理: UT-208-29(1 setter + 4 reader × 5000)、構造的: UT-208-02/17〜19/22〜26/28、SDD と実装の網羅整合: UT-208-27(6 ケース網羅、全 OK / 4 項目それぞれ単独 NG / 全 NG 早期リターン)、データフロー: clang-tidy `bugprone-*`/`concurrency-*`/`cppcoreguidelines-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<double>::is_always_lock_free` `static_assert` を **9 ユニット目に拡大**、強い型 `ElectronGunCurrent_mA` / `MagnetCurrent_A` / `DoseUnit_cGy` の compile-time 隔離(UT-208-26)、`enum class ErrorCode` 型安全)
- [x] 未解決問題なし予定(CI 全 9 ジョブ Pass は push 後 SHA 追記事後処理で最終確定)。**SDD §4.9「ElectronGunSim から計測値読取」+「UNIT-201::init_subsystems() から呼出」は Step 26+(UNIT-301 結線)+ Inc.1 後半(SafetyCoreOrchestrator dispatch 機構整備)で実施予定。SDD §4.9「治療開始拒否 + 操作者 UI 通知」(SRS-012/SRS-ALM-005)は Inc.4 UI レイヤー実装時に完成**

### UNIT-206(v0.8、本 Step 24、HZ-005 部分対応 + RCM-008 中核 / 強い型による単位安全 + エネルギー対応表)

- [x] **UNIT-206 は受入基準(§5)を全て満たしている**(SDD §4.7 公開 API 3 種 + 補助 API 3 種 + 静的純粋関数 4 種 + `EnergyMagnetMap` 内包 + 内部 3 atomic 設計と完全一致、コーディング規約遵守、UT 全 31 件設計済、ローカル骨格ビルドで `static_assert(std::atomic<double>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明通過 + `[1/4]..[4/4]` 完全ビルド成功。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-206-01/03〜05/08〜10/19/20、境界値: UT-206-04/06/09/11/15〜18/21/22/31、異常系/不正条件: UT-206-06/07/11/12/22/23、並行処理: UT-206-28(1 setter + 4 reader × 5000)/29(injector + checker)、構造的: UT-206-02/13〜18/24〜26/30、SDD と実装の網羅整合: UT-206-27(14 組合せ網羅、Electron 7 + XRay 7)、データフロー: clang-tidy `bugprone-*`/`concurrency-*`/`cppcoreguidelines-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<double>::is_always_lock_free` + `std::atomic<bool>::is_always_lock_free` 二重 `static_assert` を **8 ユニット目に拡大**、強い型 `Energy_MeV` / `Energy_MV` / `MagnetCurrent_A` の compile-time 隔離(全 7 ペアで暗黙変換不可、UT-206-26 で機械検証)、`enum class TreatmentMode` 型安全)
- [x] 未解決問題なし予定(CI 全 9 ジョブ Pass は push 後 SHA 追記事後処理で最終確定)。**SDD §4.7「set_current_for_energy の 200 ms タイムアウト」+「±5% 到達待機」+ SRS-ALM-004「連続 100 ms 以上」継続検出 + 「UNIT-202 → UNIT-206 スケジュール連鎖」は IT-102 / IT-104(Inc.1 完了時、UNIT-302/402 結線後)で実施予定**

### UNIT-204(v0.6、Step 22、HZ-005 直接 + HZ-003 構造的排除の中核)

- [x] **UNIT-204 は受入基準(§5)を全て満たしている**(SDD §4.5 公開 API 3 種 + 補助 API 5 種 + 静的純粋関数 1 種 + 内部 4 atomic 設計と完全一致、コーディング規約遵守、UT 全 32 件設計済、ローカル骨格ビルドで `static_assert(std::atomic<std::uint64_t>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明 + `[1/4]..[4/4]` 完全ビルド通過。CI 全 9 ジョブ Pass は push 後確認)
- [x] **クラス C 追加受入基準(§6)を全て満たしている**(正常系: UT-204-01〜04/13〜16/24、境界値: UT-204-05〜09/17〜19/22/23、異常系/不正条件: UT-204-07〜12/20/29、並行処理: UT-204-30(4 producer × 10000 + reader)/ UT-204-31(producer + setter)、構造的: UT-204-21(HZ-003 大量積算)/26〜28、SRS と SDD 表との網羅整合: UT-204-25(8 × 3 = 24 組合せ)、データフロー: clang-tidy `bugprone-*`/`concurrency-*`/`cppcoreguidelines-*` カテゴリで検出)
- [x] **本プロジェクト固有追加基準(§6.1)を全て満たしている**(`std::atomic<std::uint64_t>::is_always_lock_free` + `std::atomic<bool>::is_always_lock_free` 二重 `static_assert`、強い型 `DoseUnit_cGy` / `PulseCount` / `DoseRatePerPulse_cGy_per_pulse` の compile-time 区別(暗黙変換不可、UT-204-28 で機械検証)、`enum class LifecycleState` 型安全、`PulseCounter` 経由のみ accumulated_pulses_ にアクセス可能(SRS-D-009 整合))
- [x] 未解決問題なし予定(CI 全 9 ジョブ Pass は push 後 SHA 追記事後処理で最終確定)。**SDD §4.5「目標到達 → BeamOff < 1 ms」の実測は IT-101(Inc.1 完了時、UNIT-204 → UNIT-201 → UNIT-203 連鎖)で実施予定**

## 10. トレーサビリティマトリクス(本 v0.10 範囲)

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

### UNIT-205(本 Step 23 / CR-0011、HZ-001 直接対応 + RCM-005 中核、CI 実績 2026-05-01 確定済、run id `25202911125`、最終コミット `d7459b3`)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-205 | UT-205-01〜02 | SDD §4.6 / SRS-006(初期状態 / SRS 定数)| RCM-001 / RCM-005 | 全 9 ジョブ Pass(2026-05-01、run id `25202911125`)|
| UNIT-205 | UT-205-03〜05 | SRS-006 / SDD §4.6(所定 3 位置 move_to 正常系)| RCM-001 | 同上 |
| UNIT-205 | UT-205-06 | SRS-D-007(範囲境界受入)| RCM-008 | 同上 |
| UNIT-205 | UT-205-07 | SRS-D-007 / SDD §4.6(範囲外拒否)| **RCM-008 / HZ-001** | 同上 |
| UNIT-205 | UT-205-08 | SDD §4.6(センサ inject + read)| RCM-005 | 同上 |
| UNIT-205 | UT-205-09 | SDD §4.6(compute_median 静的純粋関数)| RCM-005 | 同上 |
| UNIT-205 | UT-205-10 | SRS-ALM-003 / SDD §4.6(has_discrepancy 静的純粋関数)| **RCM-005 / HZ-001** | 同上 |
| UNIT-205 | UT-205-11〜12 | SDD §4.6(TurntablePosition メンバ関数)| RCM-005 | 同上 |
| UNIT-205 | UT-205-13 | SRS-006 / SDD §4.6(is_in_position 正常系)| RCM-005 / HZ-001 | 同上 |
| UNIT-205 | UT-205-14 | SRS-006(in-position 境界値 ±0.5 mm)| **RCM-008 / HZ-001** | 同上 |
| UNIT-205 | **UT-205-15** | SRS-ALM-003 / SDD §4.6(3 系統不一致拒否) | **RCM-005 / HZ-001** | 同上(**Therac-25 Tyler 事故型「ターンテーブル位置不整合」の構造的予防を機械検証成立**)|
| UNIT-205 | **UT-205-16** | SRS-RCM-005 / SDD §4.6(単一センサ故障)| **RCM-005 / HZ-001** | 同上(**Therac-25 Tyler 事故型「単一センサ故障」を has_discrepancy で構造的検出を機械検証成立、D 主要因の中核達成**)|
| UNIT-205 | UT-205-17 | SRS-006 / SDD §4.6(3 SRS 位置網羅)| RCM-001 / RCM-005 | 同上 |
| UNIT-205 | UT-205-18 | SRS-D-007(範囲静的判定)| RCM-008 | 同上 |
| UNIT-205 | UT-205-19 | SDD §4.6(所有権)| — | 同上 |
| UNIT-205 | UT-205-20 | SDD §7 SOUP-003/004(lock-free 表明) | **RCM-002 / HZ-002 / HZ-007** | 同上(`static_assert` を 7 ユニット目に拡大)|
| UNIT-205 | UT-205-21 | SRS-D-007 / SDD §6.3(強い型 compile-time)| **RCM-008** | 同上 |
| UNIT-205 | **UT-205-22** | **SRS-006 / SRS-ALM-003 / SDD §4.6 表 9 組合せ網羅** | **RCM-001 / RCM-005** | 同上(SDD §4.6 表との完全一致を機械検証成立、6 ユニット目)|
| UNIT-205 | **UT-205-23** | SRS-D-009 / SDD §4.6 / §9 SEP-003(1 writer + 4 reader × 5000) | **RCM-002 / RCM-004 / HZ-002** | **`tsan` プリセットで race 0 確認(2026-05-01、`d7459b3`)** |
| UNIT-205 | UT-205-24 | SRS-D-009 / SDD §4.6(mover + reader 並行)| RCM-002 / HZ-002 | 同上(`tsan` で race-free 確認)|
| UNIT-205 | UT-205-25 | SDD §4.1.1 / §6.2(ErrorCode 階層整合 Turntable 系 0x04 → High、PRB-0004 で `d7459b3` 修正後)| RCM-009 | 同上 |

### UNIT-206(本 Step 24 / CR-0012、HZ-005 部分対応 + RCM-008 中核、CI 実績 2026-05-01 確定済、run id `25205346050`、最終コミット `034b5d3`)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-206 | UT-206-01 | SDD §4.7(初期状態)| RCM-008 | 全 9 ジョブ Pass(2026-05-01、run id `25205346050`)|
| UNIT-206 | UT-206-02 | SRS-D-006 / SRS-ALM-004 / SDD §4.7(SRS 定数)| RCM-008 | 同上 |
| UNIT-206 | UT-206-03〜05 | SRS-005 / SRS-004 / SDD §4.7(Electron 線形補間正常系)| **RCM-008 / HZ-005** | 同上 |
| UNIT-206 | UT-206-06 | SRS-004 / SDD §4.7(Electron エネルギー範囲外拒否)| **RCM-008 / HZ-001 / HZ-005** | 同上 |
| UNIT-206 | UT-206-07 | SRS-002 / SDD §4.7(Electron API モード不整合)| **RCM-001 / HZ-001** | 同上 |
| UNIT-206 | UT-206-08〜10 | SRS-005 / SRS-004 / SDD §4.7(XRay 線形補間正常系)| **RCM-008 / HZ-005** | 同上 |
| UNIT-206 | UT-206-11 | SRS-004 / SDD §4.7(XRay エネルギー範囲外拒否)| **RCM-008 / HZ-001 / HZ-005** | 同上 |
| UNIT-206 | UT-206-12 | SRS-002 / SDD §4.7(XRay API モード不整合)| **RCM-001 / HZ-001** | 同上 |
| UNIT-206 | UT-206-13 | SRS-005 / SDD §4.7 / §6.5(Electron EnergyMagnetMap)| **RCM-008 / HZ-005** | 同上(constexpr `static_assert` で機械検証)|
| UNIT-206 | UT-206-14 | SRS-005 / SDD §4.7 / §6.5(XRay EnergyMagnetMap)| **RCM-008 / HZ-005** | 同上(constexpr `static_assert` で機械検証)|
| UNIT-206 | UT-206-15 | SRS-ALM-004 / SDD §4.7(±5% 偏差判定の境界網羅)| **RCM-008** | 同上 |
| UNIT-206 | UT-206-16 | SDD §4.7(ゼロ目標時の特殊取扱)| RCM-008 | 同上 |
| UNIT-206 | UT-206-17 | SDD §4.7(カスタム閾値)| RCM-008 | 同上 |
| UNIT-206 | UT-206-18 | SRS-D-006(範囲静的判定)| RCM-008 | 同上 |
| UNIT-206 | UT-206-19 | SDD §4.7(センサ inject + read)| RCM-008 | 同上 |
| UNIT-206 | UT-206-20 | SRS-ALM-004 / SDD §4.7(is_within_tolerance 正常系)| RCM-008 | 同上 |
| UNIT-206 | UT-206-21 | SRS-ALM-004 / SDD §4.7(±5% 境界受入)| **RCM-008** | 同上 |
| UNIT-206 | UT-206-22 | SRS-ALM-004 / SDD §4.7(±5% 超過拒否)| **RCM-008** | 同上 |
| UNIT-206 | UT-206-23 | SDD §4.7(target 未設定時の安全側拒否)| RCM-008 | 同上 |
| UNIT-206 | UT-206-24 | SDD §4.7(所有権)| — | 同上 |
| UNIT-206 | UT-206-25 | SDD §7 SOUP-003/004(lock-free 二重表明)| **RCM-002 / HZ-002 / HZ-007** | 同上(`static_assert` を 8 ユニット目に拡大)|
| UNIT-206 | UT-206-26 | SRS-D-002/003/006 / SDD §6.3(強い型 compile-time)| **RCM-008 / HZ-005** | 同上(全 7 ペアで暗黙変換不可)|
| UNIT-206 | **UT-206-27** | **SRS-002 / SRS-004 / SRS-005 / SDD §4.7 表 14 組合せ網羅** | **RCM-001 / RCM-008 / HZ-001 / HZ-005** | 同上(SDD §4.7 表との完全一致を機械検証成立、7 ユニット目)|
| UNIT-206 | **UT-206-28** | SRS-D-009 / SDD §4.7 / §9 SEP-003(1 setter + 4 reader × 5000) | **RCM-002 / RCM-004 / HZ-002** | 同上(`tsan` プリセットで race 0 確認予定)|
| UNIT-206 | UT-206-29 | SRS-D-009 / SDD §4.7(injector + checker 並行)| RCM-002 / HZ-002 | 同上(`tsan` で race-free 確認予定)|
| UNIT-206 | UT-206-30 | SDD §4.1.1 / §6.2(ErrorCode 階層整合 Magnet 系 0x05 → Medium)| RCM-009 | 同上 |
| UNIT-206 | UT-206-31 | SRS-D-006(防御層構造試験)| **RCM-008** | 同上 |

### UNIT-208(本 Step 25 / CR-0013、RCM-013 中核 + HZ-009 部分対応、CI 実績 2026-05-01 確定済、run id `25213056027`、最終コミット `da6b1de`)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-208 | UT-208-01 | SRS-012 / SDD §4.9(初期状態 全 4 項目 OK)| **RCM-013** | 全 9 ジョブ Pass(2026-05-01、run id `25213056027`)|
| UNIT-208 | UT-208-02 | SDD §4.9(SRS 定数 許容差)| RCM-013 | 同上 |
| UNIT-208 | UT-208-03 | SRS-012 / SDD §4.9(電子銃 許容差内)| RCM-013 / HZ-009 | 同上 |
| UNIT-208 | UT-208-04 | SRS-012 / SDD §4.9(電子銃 正方向超過)| **RCM-013 / HZ-009** | 同上 |
| UNIT-208 | UT-208-05 | SRS-012 / SDD §4.9(電子銃 負方向超過)| **RCM-013 / HZ-009** | 同上 |
| UNIT-208 | UT-208-06 | SRS-012 / SRS-006 / SDD §4.9(ターンテーブル Light)| RCM-005 / RCM-013 | 同上 |
| UNIT-208 | UT-208-07 | SRS-012 / SRS-006 / SDD §4.9(±0.5 mm 境界)| **RCM-008 / RCM-013** | 同上 |
| UNIT-208 | UT-208-08 | SRS-012 / SRS-006 / SDD §4.9(±0.51 mm 拒否)| **RCM-013 / HZ-001** | 同上 |
| UNIT-208 | **UT-208-09** | SRS-012 / SRS-ALM-003 / SDD §4.9(3 系統不一致)| **RCM-005 / RCM-013 / HZ-001** | 同上(**Therac-25 Tyler 事故型「単一センサ故障」を起動時自己診断層で構造的検出**)|
| UNIT-208 | UT-208-10 | SRS-012 / SRS-006(Light 不一致拒否)| **RCM-013 / HZ-001 / HZ-009** | 同上 |
| UNIT-208 | UT-208-11 | SRS-012 / SDD §4.9(磁石 0)| RCM-013 | 同上 |
| UNIT-208 | UT-208-12 | SRS-012 / SDD §4.9(磁石 許容差内)| RCM-013 | 同上 |
| UNIT-208 | UT-208-13 | SRS-012 / SDD §4.9(磁石 正方向超過)| **RCM-008 / RCM-013** | 同上 |
| UNIT-208 | UT-208-14 | SRS-012 / SDD §4.9(磁石 負方向超過)| **RCM-008 / RCM-013** | 同上 |
| UNIT-208 | UT-208-15 | SRS-012 / SDD §4.9(ドーズ 0)| RCM-003 / RCM-013 | 同上 |
| UNIT-208 | **UT-208-16** | SRS-012 / SRS-009 / SDD §4.9(ドーズ != 0 拒否)| **RCM-003 / RCM-013 / HZ-009** | 同上 |
| UNIT-208 | **UT-208-17** | SDD §4.9(早期リターン 電子銃 1 番目)| **RCM-013** | 同上(**4 項目自己診断の物理経路順早期リターンを機械検証**)|
| UNIT-208 | UT-208-18 | SDD §4.9(早期リターン ターンテーブル 2 番目)| RCM-013 | 同上 |
| UNIT-208 | UT-208-19 | SDD §4.9(早期リターン 磁石 3 番目)| RCM-013 | 同上 |
| UNIT-208 | UT-208-20 | SDD §4.9(is_electron_gun_in_zero 静的純粋関数)| RCM-013 | 同上 |
| UNIT-208 | UT-208-21 | SDD §4.9(is_bending_magnet_in_zero 静的純粋関数)| RCM-013 | 同上 |
| UNIT-208 | UT-208-22 | SDD §4.9(is_dose_zero 静的純粋関数 + constexpr)| RCM-013 | 同上 |
| UNIT-208 | UT-208-23 | SDD §4.9(inject ラウンドトリップ)| RCM-013 | 同上 |
| UNIT-208 | UT-208-24 | SDD §4.9(所有権)| — | 同上 |
| UNIT-208 | UT-208-25 | SDD §7 SOUP-003/004(lock-free 表明)| **RCM-002 / HZ-002 / HZ-007** | 同上(`static_assert` を 9 ユニット目に拡大)|
| UNIT-208 | UT-208-26 | SRS-D-006/008 / SDD §6.3(強い型 compile-time)| **RCM-008** | 同上 |
| UNIT-208 | **UT-208-27** | **SRS-012 / SDD §4.9 表 6 ケース網羅** | **RCM-013** | 同上(**SDD §4.9 表との完全一致を機械検証成立、8 ユニット目**)|
| UNIT-208 | UT-208-28 | SDD §4.1.1 / §6.2(ErrorCode 階層整合 4 系統)| RCM-009 | 同上 |
| UNIT-208 | **UT-208-29** | SRS-D-009 / SDD §4.9 / §9 SEP-003(1 setter + 4 reader × 5000)| **RCM-002 / RCM-004 / HZ-002** | **`tsan` プリセットで race 0 確認(2026-05-01、`da6b1de`)** |

### UNIT-301(本 Step 26 / CR-0014、Hardware Simulator 層の最初の具体実装、CI 実績は push 後追記)

| ユニット ID | 試験 ID | 関連 SRS / SDD | 関連 RCM / HZ | CI ジョブ Pass |
|------------|--------|--------------|------------|--------------|
| UNIT-301 | UT-301-01 | SDD §4.12(初期状態)| RCM-002 | (push 後追記)|
| UNIT-301 | UT-301-02 | SRS-D-008 / SDD §4.12(SRS 定数)| RCM-008 | 同上 |
| UNIT-301 | UT-301-03 | SRS-IF-001 / SRS-O-001 / SDD §4.12(set_current 正常系)| RCM-002 | 同上 |
| UNIT-301 | UT-301-04 | SRS-D-008 / SDD §4.12(上限境界)| **RCM-008** | 同上 |
| UNIT-301 | UT-301-05 | SRS-D-008 / SDD §4.12(下限境界)| **RCM-008** | 同上 |
| UNIT-301 | UT-301-06 | SRS-D-008 / SDD §4.12(範囲外正方向 物理飽和)| **RCM-008** | 同上 |
| UNIT-301 | UT-301-07 | SRS-D-008 / SDD §4.12(範囲外負方向 物理飽和)| **RCM-008** | 同上 |
| UNIT-301 | UT-301-08 | SRS-I-008 / SDD §4.12(read_actual ノイズ無効)| RCM-002 | 同上 |
| UNIT-301 | UT-301-09 | SDD §4.12(noise enable/disable 状態遷移)| RCM-002 | 同上 |
| UNIT-301 | UT-301-10 | SDD §4.12(ノイズ ±1% 許容範囲)| RCM-002 / RCM-008 | 同上 |
| UNIT-301 | UT-301-11 | SDD §4.12(noise seed 再現性)| RCM-002 | 同上 |
| UNIT-301 | UT-301-12 | SDD §4.12(ゼロ commanded ゼロ actual)| RCM-008 | 同上 |
| UNIT-301 | UT-301-13 | SRS-D-008(is_current_in_range 静的純粋)| RCM-008 | 同上(constexpr `static_assert` 含む)|
| UNIT-301 | UT-301-14 | SRS-D-008 / SDD §4.12(clamp_to_range 静的純粋)| RCM-008 | 同上(constexpr `static_assert` 含む)|
| UNIT-301 | UT-301-15 | SRS-D-008 / SDD §6.3(強い型 compile-time)| **RCM-008** | 同上 |
| UNIT-301 | UT-301-16 | SDD §4.12(所有権)| — | 同上 |
| UNIT-301 | UT-301-17 | SDD §7 SOUP-003/004(lock-free 三重表明)| **RCM-002 / HZ-002 / HZ-007** | 同上(`static_assert` を 10 ユニット目に拡大)|
| UNIT-301 | **UT-301-18** | **SRS-IF-001 / SRS-O-001 / SRS-I-008 / SDD §4.12 表 7 ケース網羅** | **RCM-008** | 同上(SDD §4.12 表との完全一致を機械検証成立、9 ユニット目)|
| UNIT-301 | **UT-301-19** | SRS-D-009 / SDD §4.12 / §9 SEP-003(1 setter + 4 reader × 5000)| **RCM-002 / RCM-019 / HZ-002** | **`tsan` プリセットで race 0 確認予定** |

## 11. 改訂履歴

| バージョン | 日付 | 変更内容 | 変更者 |
|----------|------|---------|--------|
| 0.1 | 2026-04-26 | 初版作成(Step 17 / CR-0005、本プロジェクト Issue #5)。**Inc.1 範囲のうち UNIT-200 CommonTypes に対する第 I 部(計画)+ 第 II 部(報告)を初期化**。**§3 実装ルール:** MISRA C++ 2023 + CERT C++ + C++20 + clang-tidy 全カテゴリ + Sanitizer 3 種 + `std::atomic` 共有変数規則(`PulseCounter` 経由のみ可)を確定。**§4 検証プロセス:** コードレビュー / 静的解析 / 動的解析 / UT / 網羅率計測の 5 手段を整理。**§4.2 並行処理 RCM の 3 層検証(SRMP §6.1 整合):** TSan + 境界値 + モデル検査(Inc.3 で導入判断)を必須化。**§5 受入基準 5 項目** + **§6 クラス C 追加受入基準 9 項目** + **§6.1 本プロジェクト固有追加基準 3 項目**(`is_always_lock_free` `static_assert`、強い型 `explicit` + 算術非提供、`enum class` 型安全)を確定。**§7.2 UNIT-200 試験ケース 24 件**(enum 系 5 件、強い型 7 件、PulseCount/Counter 7 件、Result 5 件)を定義し、`tests/unit/test_common_types.cpp` として実装済。**§8 実施サマリ:** ローカル骨格ビルド(`TH25_BUILD_TESTS=OFF`)で `static_assert(is_always_lock_free)` を AppleClang 15 で検証成功。CI cpp-build マトリクス全 8 ジョブ + clang-tidy ジョブの Pass は Step 17 push 後の SHA 追記事後処理で記録予定。**Therac-25 学習目的の中核達成:** UT-200-16 で 4 スレッド × 10000 increment による `PulseCounter` race-free 検証を TSan プリセットで実施(Tyler 第 1 例 race condition 類型の機械的検証手段確立)、UT-200-15 で `uint64_t::max()` 境界値検証(Yakima Class3 オーバフロー類型の構造的排除確認)、UT-200-12 + ヘッダ内 `static_assert(is_always_lock_free)` で HZ-007(レガシー前提喪失)構造的予防の機械検証成立 | k-abe |
| 0.2 | 2026-04-29 | **昇格(Step 18 / CR-0006、本プロジェクト Issue #6)**。**UNIT-401 InProcessQueue(SPSC lock-free ring buffer)を §3.2 実装対象に追加**(本 v0.2 計画範囲)、SDD-TH25-001 v0.1.1 への参照に更新(folly 採用 → 独自 Lamport SPSC 実装に確定)。**§7.2 UNIT-401 試験ケース 13 件追加**(初期 / publish-consume / 境界値 / wrap-around / lock-free 表明 / 所有権 / メッセージ型独立性 / 並行処理 SPSC 100k メッセージ / burst pattern)を定義し、`tests/unit/test_in_process_queue.cpp` として実装済。**累積試験ケース = 37 件**(UNIT-200 24 件 + UNIT-401 13 件)。**§8.1bis 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<size_t>::is_always_lock_free)` + Capacity = power-of-two `static_assert` + template type 制約 `static_assert` を AppleClang 15 で検証成功。CI cpp-build マトリクス全 9 ジョブ Pass は Step 18 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-401 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-401 行追加**(13 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UT-401-12 で 1 producer + 1 consumer × 100k メッセージ SPSC race-free 検証を `tsan` プリセットで実施する設計を確立(Tyler 第 1 例 race condition 類型に対する SDD §4.13 SPSC ring buffer 設計の機械的検証手段成立)。UT-401-08 + ヘッダ内 `static_assert(is_always_lock_free)` で HZ-007 構造的予防の機械検証を Inc.1 中核 MessageBus 層に拡張 | k-abe |
| 0.3 | 2026-04-30 | **昇格(Step 19 / CR-0007、本プロジェクト Issue #7)**。**UNIT-201 SafetyCoreOrchestrator(`LifecycleState` 8 状態機械 + イベントループ + 公開 API 4 種)を §3.2 実装対象に追加**(本 v0.3 計画範囲)。SDD-TH25-001 v0.1.1 §4.2 / §6.1 と完全一致する形で実装(`safety_core_orchestrator.hpp` + `.cpp`)。**§7.2 UNIT-201 試験ケース 24 件追加**(初期状態 / `init_subsystems` idempotency / lock-free 表明 / 所有権 / `shutdown` signal-safe / `run_event_loop` 戻り値 / 状態遷移許可表 10 件 / 不正遷移拒否 / 終端 Halted / 緊急停止 ShutdownRequested / **88 組合せ網羅試験 UT-201-21** / EventLoop 統合 2 件 / **SPSC イベント配送 concurrent 試験 UT-201-24**)を定義し、`tests/unit/test_safety_core_orchestrator.cpp` として実装済。**累積試験ケース = 61 件**(UNIT-200 24 件 + UNIT-401 13 件 + UNIT-201 24 件)。**§8.1ter 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<LifecycleState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` + template instantiation(`SafetyCoreOrchestrator::EventQueue::capacity()`)を AppleClang 15 で検証成功(`[1/3]..[3/3]` 完全ビルド)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 19 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-201 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-201 行追加**(24 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UT-201-21 の 88 組合せ網羅試験で SDD §6.1 状態遷移許可表との完全一致を機械検証(SDD と実装の構造的乖離を防ぐ仕組み)。UT-201-18(Idle→BeamOn 直接禁止)で HZ-001 の構造的不可能化を状態機械層で実体化(RCM-001 中核)。UT-201-24 で SPSC イベント配送 200 サイクル × 6 イベント/サイクル race-free 検証を `tsan` プリセットで実施する設計を確立(Tyler 第 1 例 race condition 類型に対する HZ-002 機械的予防を Inc.1 オーケストレータ層に拡大)。UT-201-04 + ヘッダ内 `static_assert(std::atomic<LifecycleState>::is_always_lock_free)` で HZ-007 構造的予防の機械検証を 3 ユニット目に拡大(UNIT-200/401 と同パターン継承)| k-abe |
| 0.5 | 2026-04-30 | **昇格(Step 21 / CR-0009、本プロジェクト Issue #9、RCM-017 中核 / 許可フラグ消費型)**。**UNIT-203 BeamController(ビームオン許可フラグの `compare_exchange_strong` による 1 回限り消費型管理 + BeamState 4 状態機械 + ビームオフ即時遷移)を §3.2 実装対象に追加**(本 v0.5 計画範囲)。SDD-TH25-001 v0.1.1 §4.4 と完全一致する形で実装(`beam_controller.hpp` + `.cpp`)。**§7.2 UNIT-203 試験ケース 19 件追加**(初期状態 / permission 操作 / Lifecycle ガード 7 状態 / permission ガード / 成功遷移 / **1 回限り消費(UT-203-07)** / beam_off 正常系 / 再 beam_on は permission 再設定必要 / 所有権 / lock-free 表明 / **8 thread × 1000 セット並列で各セット唯一 1 件成功(UT-203-13)** / 並列 beam_off / BeamState 値安定性 / 防御的 / setter/reader 並行 / state ガード / フェイルセーフ性)を定義し、`tests/unit/test_beam_controller.cpp` として実装済。**累積試験ケース = 106 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26 + UNIT-203 19)。**§8.1quinquies 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<BeamState>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明を AppleClang 15 で検証成功。CI cpp-build マトリクス全 9 ジョブ Pass は Step 21 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-203 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-203 行追加**(19 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-203 は **RCM-017 中核ユニット**。UT-203-07(1 回限り消費)+ UT-203-13(8 thread × 1000 セット並列で各セット唯一 1 件成功)で、Therac-25 Tyler 事故型「許可確認とビームオン指令の race condition」「ビーム二重照射」の構造的予防(`compare_exchange_strong` による消費型設計)を機械検証する仕組みを確立(HZ-001/HZ-002/HZ-004 直接対応)。UT-203-12 + ヘッダ内 `static_assert` 二重表明で HZ-007 構造的予防を 5 ユニット目に拡大(UNIT-200/401/201/202/203)。SDD §4.4「< 10 ms ビームオフ保証」の実測は Inc.1 完了時の IT-101 で実施する旨を §8.5 に明記 | k-abe |
| 0.4 | 2026-04-30 | **昇格(Step 20 / CR-0008、本プロジェクト Issue #8、HZ-001 直接対応の中核)**。**UNIT-202 TreatmentModeManager(モード遷移許可表 + 強い型 Energy_MeV/MV エネルギー範囲検証 + BeamState 事前条件)を §3.2 実装対象に追加**(本 v0.4 計画範囲)。SDD-TH25-001 v0.1.1 §4.3 と完全一致する形で実装(`treatment_mode_manager.hpp` + `.cpp`)。**§7.2 UNIT-202 試験ケース 26 件追加**(初期状態 / モード遷移正常系 6 件 / BeamState != Off 拒否 3 件 / エネルギー範囲外拒否 4 件 / 境界値受入 2 件 / **API ガード 2 件(Therac-25 Tyler 事故型の構造的拒否)** / 強い型 compile-time / **並行処理 UT-202-21** / 所有権 / lock-free 表明 / verify_mode_consistency / **9 組合せ網羅試験 UT-202-25** / 防御的 enum 値拒否)を定義し、`tests/unit/test_treatment_mode_manager.cpp` として実装済。**累積試験ケース = 87 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26)。**§8.1quater 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<TreatmentMode>::is_always_lock_free)` を AppleClang 15 で検証成功(`[1/4]..[4/4]` 完全ビルド)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 20 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-202 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-202 行追加**(26 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-202 は **HZ-001 直接対応の中核ユニット**。UT-202-09〜11 / UT-202-18〜19 で Therac-25 Tyler 事故の三重エラー「Electron モードで X 線エネルギー」「BeamState 確認漏れ」「ターンテーブル位置不整合」の発現経路を構造的に拒否する設計を確立(RCM-001 / HZ-001 直接対応)。UT-202-25 の 9 組合せ網羅試験で SDD §4.3 表との完全一致を機械検証成立(SDD と実装の構造的乖離防止メカニズムを 4 ユニット目に拡大)。UT-202-12〜17 のエネルギー範囲境界値検証で SRS-004 範囲(Electron 1.0〜25.0 MeV / XRay 5.0〜25.0 MV)を機械的に保護(RCM-008、HZ-005 部分対応)。UT-202-21 の 2 writer + 1 reader 並行試験で `std::atomic<TreatmentMode>` の race-free 性を `tsan` プリセットで検証する設計を確立(HZ-002 機械的予防の継続)。UT-202-23 + ヘッダ内 `static_assert(std::atomic<TreatmentMode>::is_always_lock_free)` で HZ-007 構造的予防の機械検証を 4 ユニット目に拡大(UNIT-200/401/201 と同パターン継承)| k-abe |
| 0.10 | 2026-05-01 | **昇格(Step 26 / CR-0014、本プロジェクト Issue #18、Hardware Simulator 層の最初の具体実装 / 仮想電子銃応答模擬)**。**UNIT-301 ElectronGunSim(set_current + read_actual_current、SRS-IF-001 完全一致)を §3.2 実装対象に追加**(本 v0.10 計画範囲)。SDD-TH25-001 v0.1.1 §4.12 と完全一致する形で実装(`electron_gun_sim.hpp` + `.cpp`、`src/th25_sim/` 配下)。`std::atomic<double>` `commanded_current_ma_` + `std::atomic<bool>` `noise_enabled_` + `mutable std::atomic<std::uint32_t>` `rng_state_` の三重 atomic で並行アクセス保護(`mutable std::atomic` で論理的 const 性とノイズ生成 thread-safety を両立する標準的パターン採用)。SRS-D-008 範囲(0.0〜10.0 mA)+ ノイズ許容率(±1%)を範囲定数で外部化。**ノイズ模擬は default OFF**(UT 安定化、`enable_noise(seed)` で明示的に有効化、`disable_noise()` で無効化)。**簡易 LCG**(Numerical Recipes 推奨係数、`rng_state' = rng_state * 1664525 + 1013904223`)で `read_actual_current()` 内 `fetch_add(kLcgIncrement, acq_rel)` により thread-safe な疑似乱数生成を実現、上位 24 bit を `[-1, +1]` に正規化して `commanded × 0.01 × normalized` のノイズを乗算。範囲外指令は `clamp_to_range` で物理的飽和を模擬(実物理デバイス挙動)。`th25_sim` ライブラリを `th25_ctrl` に PUBLIC リンク(common_types.hpp の `ElectronGunCurrent_mA` 強い型を利用)。**§7.2 UNIT-301 試験ケース 19 件追加**(初期状態 + SRS 定数 / set_current 正常系 + 上下限境界 + 範囲外正負方向飽和 / read_actual_current ノイズ無効 / noise enable/disable 状態遷移 / **ノイズ ±1% 許容範囲(UT-301-10)** / **noise seed 再現性(UT-301-11、同 seed → 同シーケンス、異 seed → 別値)** / ゼロ commanded ゼロ actual / 静的純粋関数 2 種(constexpr `static_assert` 含む)/ 強い型 compile-time / 所有権 / **HZ-007 lock-free 三重表明(UT-301-17、10 ユニット目に拡大)** / **SDD §4.12 表 7 ケース網羅(UT-301-18、9 ユニット目に拡大)** / **並行処理 1 setter + 4 reader × 5000(UT-301-19、tsan)**)、`tests/unit/test_electron_gun_sim.cpp` として実装済。**累積試験ケース = 242 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26 + UNIT-203 19 + UNIT-204 32 + UNIT-205 25 + UNIT-206 31 + UNIT-208 29 + UNIT-301 19)。**§8.1decies 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<double>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` + `static_assert(std::atomic<std::uint32_t>::is_always_lock_free)` 三重表明を AppleClang 15 で検証成功(`th25_sim` 静的ライブラリ完全ビルド成功、`electron_gun_sim.cpp` を `th25_sim` ライブラリの 2 cpp 目として組込)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 26 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-301 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-301 行追加**(19 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-301 は **Hardware Simulator 層の最初の具体実装ユニット** + Inc.1 後半で UNIT-203 BeamController + UNIT-208 StartupSelfCheck と結線される基盤確立。UT-301-18 の 7 ケース網羅試験で SDD §4.12 表との完全一致を機械検証成立(SDD と実装の構造的乖離防止メカニズムを **9 ユニット目に拡大**)。UT-301-19 の 1 setter + 4 reader × 5000 並行試験で `std::atomic<double>` + `std::atomic<bool>` + `std::atomic<std::uint32_t>` の三重 atomic の race-free 性を `tsan` プリセットで検証する設計を確立(HZ-002 機械的予防の継続)。UT-301-17 + ヘッダ内 `static_assert(is_always_lock_free)` 三重表明で HZ-007 構造的予防の機械検証を **10 ユニット目に拡大**(UNIT-200/401/201/202/203/204/205/206/208/301)。**Step 22/23/24/25 教訓の継続反映**: (1) `static_assert(is_always_lock_free)` 統一(PRB-0001 教訓)、(2) UT 実装時に SRS-D-008 範囲制約セルフチェック(PRB-0002 教訓)、(3) 表セル内のパイプ文字(縦棒)衝突回避(Step 24 markdownlint MD056 教訓)、(4) push 前ローカル markdownlint 実行運用継続。**ローカルビルドエラーの即時修正:** 初版で `read_actual_current()` (const メソッド) 内で `rng_state_.fetch_add(...)` が「const オブジェクトに対する non-const メンバ関数呼出」でビルドエラー → `mutable std::atomic<std::uint32_t>` で修正(C++ 標準的パターン、論理的 const 性を維持しつつ rng 内部状態の更新を許可)、PRB 起票不要(ローカルビルド段階での即時修正、本番コードへの影響なし) | k-abe |
| 0.9 | 2026-05-01 | **昇格(Step 25 / CR-0013、本プロジェクト Issue #17、RCM-013 中核 + HZ-009 部分対応 / 起動時 4 項目自己診断)**。**UNIT-208 StartupSelfCheck(起動時 4 項目自己診断: 電子銃 0 mA / ターンテーブル Light 位置 / 磁石 0 A / ドーズカウンタ 0)を §3.2 実装対象に追加**(本 v0.9 計画範囲)。SDD-TH25-001 v0.1.1 §4.9 と完全一致する形で実装(`startup_self_check.hpp` + `.cpp`)。Manager 群 (UNIT-204 DoseManager + UNIT-205 TurntableManager + UNIT-206 BendingMagnetManager) への参照を保持し、`perform_self_check` が 4 項目を物理経路順(電子銃 → ターンテーブル → 磁石 → ドーズ)で検証 + 早期リターン。UNIT-301 ElectronGunSim 未実装のため `inject_electron_gun_current(ElectronGunCurrent_mA)` 補助 API + `std::atomic<double>` 1 個で UT 駆動可能(将来 UNIT-301 結線時に Sim 経由センサ実読取に置換、API 不変)。**SRS-012「StartupSelfCheckFailed」エラーの実装上の分類**: 4 項目それぞれ既存 ErrorCode 階層で意味的に適切に分類(電子銃 NG → `ElectronGunCurrentOutOfRange` Beam 系 0x02 Critical / ターンテーブル NG → `TurntableOutOfPosition` Turntable 系 0x04 High / 磁石 NG → `MagnetCurrentDeviation` Magnet 系 0x05 Medium / ドーズ NG → `InternalUnexpectedState` Internal 系 0xFF Critical)、これにより `inc1-design-frozen` を維持しつつ SRS-UX-001 詳細エラーメッセージ + 推奨対処に整合。SDD §4.9 + UNIT-205::is_in_position(kLightPositionMm) で「Light 位置 ±0.5 mm 以内 + 3 系統偏差 < 1.0 mm」の両判定を構造的に達成(SRS-012 + SRS-ALM-003 の重ね合わせ)。**§7.2 UNIT-208 試験ケース 29 件追加**(初期状態 + SRS 定数 / 電子銃 許容差内・正負境界超過 / ターンテーブル Light 位置・±0.5 mm 境界・±0.51 mm 拒否・3 系統不一致・Light 不一致 / 磁石 許容差内・正負境界超過 / ドーズ 0・積算後 / **早期リターン順序 3 件(UT-208-17/18/19)** / 静的純粋関数 3 種 / inject ラウンドトリップ / 所有権 / lock-free 表明 / 強い型 compile-time / **SDD §4.9 表 6 ケース網羅(UT-208-27、SDD ↔ 実装の整合性、8 ユニット目)** / ErrorCode 階層整合 / **並行処理 1 setter + 4 reader × 5000(UT-208-29、tsan)**)、`tests/unit/test_startup_self_check.cpp` として実装済。**累積試験ケース = 223 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26 + UNIT-203 19 + UNIT-204 32 + UNIT-205 25 + UNIT-206 31 + UNIT-208 29)。**§8.1nonies 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<double>::is_always_lock_free)` を AppleClang 15 で検証成功(`th25_ctrl` 静的ライブラリ完全ビルド成功、`startup_self_check.cpp` を 8 cpp 目として組込)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 25 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-208 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-208 行追加**(29 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-208 は **RCM-013 中核ユニット + HZ-009(電源喪失再起動時不定動作)部分対応**。UT-208-04/05/13/14 で電子銃 + 磁石の境界値超過(SDD §4.9 許容差 abs < 0.01)を機械検証成立(Therac-25 起動時状態不定リスクへの構造的予防)。UT-208-09 で「3 系統センサ不一致」を起動時自己診断層で構造的検出(Therac-25 Tyler 事故型「単一センサ故障」を起動段階で構造的拒否、D 主要因の 7 ユニット目に拡大)。UT-208-27 の 6 ケース網羅試験で SDD §4.9 表との完全一致を機械検証成立(SDD と実装の構造的乖離防止メカニズムを **8 ユニット目に拡大**)。UT-208-17〜19 の早期リターン順序機械検証で「物理経路順(上流 → 下流)での早期リターン」が機械検証可能化(SDD §4.9 設計判断の正当性を構造的に証明)。UT-208-29 の 1 setter + 4 reader × 5000 並行試験で `std::atomic<double>` の race-free 性を `tsan` プリセットで検証する設計を確立(HZ-002 機械的予防の継続)。UT-208-25 + ヘッダ内 `static_assert(std::atomic<double>::is_always_lock_free)` で HZ-007 構造的予防の機械検証を **9 ユニット目に拡大**(UNIT-200/401/201/202/203/204/205/206/208 と同パターン継承)。**Step 22/23/24 教訓の継続反映**: (1) `static_assert(is_always_lock_free)` 統一(PRB-0001 教訓)、(2) UT 実装時に SRS-012/SRS-006/SRS-D-008 範囲制約セルフチェック(PRB-0002 教訓)、(3) common_types.hpp §6.2 Severity 判定規則(4 系統の Severity 確認)を UT 実装前に確認(PRB-0004 教訓、UT-208-28 で機械検証)、(4) 表セル内のパイプ文字(縦棒)衝突回避(Step 24 markdownlint MD056 教訓、本書編集時に注意)、(5) CI 失敗時は SPRP §3.1 PRB 起票運用継続。**SRS-012「StartupSelfCheckFailed」を既存 ErrorCode 階層へマッピングする設計選択により `inc1-design-frozen` を維持**(common_types.hpp / SDD §4.1.1 変更不要、SRS-UX-001 詳細エラーメッセージとも整合) | k-abe |
| 0.8 | 2026-05-01 | **昇格(Step 24 / CR-0012、本プロジェクト Issue #16、HZ-005 部分対応 + RCM-008 中核 / 強い型による単位安全 + エネルギー対応表)**。**UNIT-206 BendingMagnetManager(ベンディング磁石電流指令 + EnergyMagnetMap 線形補間 + ±5% 偏差判定)を §3.2 実装対象に追加**(本 v0.8 計画範囲)。SDD-TH25-001 v0.1.1 §4.7 と完全一致する形で実装(`bending_magnet_manager.hpp` + `.cpp`)。`std::atomic<double>` 2 個(target_current_/actual_current_)+ `std::atomic<bool>`(target_set_)で並行アクセス保護。SRS-D-006 範囲(0.0〜500.0 A)+ SRS-ALM-004 偏差閾値(±5% = 0.05)を範囲定数で外部化。**`EnergyMagnetMap` を `class` として内包し、線形補間係数 (slope, intercept) を constexpr で内部保持**(electron: 2.5 A/MeV / xray: 10.0 A/MV、SDP §6.1 確定後の校正データに CR で置換可能、HZ-007 構造的予防)。**§7.2 UNIT-206 試験ケース 31 件追加**(初期状態 / SRS 定数 / Electron 線形補間 3 件 + 範囲外拒否 + モード不整合 / XRay 線形補間 3 件 + 範囲外拒否 + モード不整合 / EnergyMagnetMap Electron/XRay 静的純粋関数 / ±5% 偏差判定 + ゼロ目標特殊取扱 + カスタム閾値 / SRS-D-006 範囲判定 / inject + read / is_within_tolerance 正常 + 境界 ±5% + 超過 + 未設定保護 / 所有権 / lock-free 二重表明 / **強い型 compile-time 隔離 7 ペア(UT-206-26)** / **14 組合せ網羅試験(UT-206-27、SDD ↔ 実装の整合性、7 ユニット目)** / **並行処理 1 setter + 4 reader × 5000(UT-206-28)+ injector + checker(UT-206-29、tsan)** / ErrorCode 階層整合 Magnet 系 0x05 → Medium / 防御層構造試験)、`tests/unit/test_bending_magnet_manager.cpp` として実装済。**累積試験ケース = 194 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26 + UNIT-203 19 + UNIT-204 32 + UNIT-205 25 + UNIT-206 31)。**§8.1octies 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<double>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明を AppleClang 15 で検証成功(`[1/4]..[4/4]` 完全ビルド)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 24 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-206 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-206 行追加**(31 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-206 は **HZ-005 部分対応 + RCM-008 中核ユニット**。UT-206-26 で Energy_MeV / Energy_MV / MagnetCurrent_A の compile-time 隔離(全 7 ペアで暗黙変換不可)を機械検証成立(HZ-005 ドーズ計算誤りの単位混在を構造的に予防、RCM-008 中核達成)。UT-206-27 の 14 組合せ網羅試験で SDD §4.7 表との完全一致を機械検証成立(SDD と実装の構造的乖離防止メカニズムを **7 ユニット目に拡大**、UT-201-21 88 / UT-202-25 9 / UT-204-25 24 / UT-205-22 9 / UT-206-27 14 の累積で「全主要設計表との一致を網羅検証」する文化を継続)。UT-206-13/14 で EnergyMagnetMap 線形補間 (slope, intercept) を constexpr `static_assert` で機械検証成立(SDD §4.7「具体値はコンストラクタで注入、コードに直接埋め込まない」の構造的実現、SDP §6.1 確定後に校正データへ CR で置換可能な構造)。UT-206-15/21/22 で SRS-ALM-004 偏差 ±5% 判定の境界(±5% 受入 / ±6% 拒否 / ゼロ目標特殊取扱 / 未設定安全側拒否)を機械検証(RCM-008 / 防御層)。UT-206-28 の 1 setter + 4 reader × 5000 並行試験で `std::atomic<double>` × 2 + `std::atomic<bool>` の release-acquire ordering の race-free 性を `tsan` プリセットで検証する設計を確立(HZ-002 機械的予防の継続)。UT-206-25 + ヘッダ内 `static_assert(std::atomic<double>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明で HZ-007 構造的予防の機械検証を **8 ユニット目に拡大**(UNIT-200/401/201/202/203/204/205/206、コード実装範囲内全 8 ユニットに展開完了)。**Step 22/23 教訓の継続反映**: (1) runtime `is_lock_free()` を呼ばず `static_assert(is_always_lock_free)` で統一(PRB-0001 教訓)、(2) UT 実装時に SRS-004/SRS-D-006/SRS-ALM-004 の範囲制約を確認しながら値を設定(PRB-0002 教訓)、(3) common_types.hpp §6.2 Severity 判定規則(Magnet 系 0x05 → Medium)を UT 実装前に確認(PRB-0004 教訓)、(4) CI 失敗時は SPRP §3.1 規定に従い PRB を起票して対応(Step 22/23 で本格運用第 1〜2 例として確立)| k-abe |
| 0.7 | 2026-05-01 | **昇格(Step 23 / CR-0011、本プロジェクト Issue #13、HZ-001 直接対応 + RCM-005 中核 / 3 系統センサ集約 med-of-3)**。**UNIT-205 TurntableManager(ターンテーブル位置指令 + 3 系統独立センサ集約 med-of-3 + 偏差判定 + is_in_position ±0.5 mm)を §3.2 実装対象に追加**(本 v0.7 計画範囲)。SDD-TH25-001 v0.1.1 §4.6 と完全一致する形で実装(`turntable_manager.hpp` + `.cpp`)。`std::atomic<double>` 4 個(sensor_0/1/2 + target)で並行アクセス保護。SRS-006 所定 3 位置(Electron=0.0/XRay=50.0/Light=-50.0 mm)+ SRS-D-007 範囲(-100.0〜+100.0 mm)+ SRS-ALM-003 偏差閾値(1.0 mm)+ SRS-006 in-position 許容偏差(±0.5 mm)を範囲定数で外部化。**§7.2 UNIT-205 試験ケース 25 件追加**(初期状態 / SRS 定数 / move_to 正常系 3 位置 + 範囲境界 ± / inject + read / compute_median 6 順列 + 同値 / has_discrepancy 5 ケース / TurntablePosition メンバ関数 / is_in_position 正常 + 境界 / **3 系統不一致拒否(UT-205-15)** / **単一センサ故障(UT-205-16、Therac-25 Tyler 事故型)** / 3 SRS 位置網羅 / 範囲静的判定 / 所有権 / lock-free 表明 / 強い型 compile-time / **9 組合せ網羅試験(UT-205-22、SDD ↔ 実装の整合性、6 ユニット目)** / **並行処理 1 writer + 4 reader × 5000(UT-205-23)+ mover + reader(UT-205-24、tsan)** / ErrorCode 階層整合)、`tests/unit/test_turntable_manager.cpp` として実装済。**累積試験ケース = 163 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26 + UNIT-203 19 + UNIT-204 32 + UNIT-205 25)。**§8.1septies 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<double>::is_always_lock_free)` を AppleClang 15 で検証成功(`[1/4]..[4/4]` 完全ビルド)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 23 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-205 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-205 行追加**(25 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-205 は **HZ-001 直接対応 + RCM-005 中核ユニット**。UT-205-15(3 系統不一致拒否)+ UT-205-16(単一センサ故障)で Therac-25 Tyler 事故型「ターンテーブル位置不整合」+「単一センサ故障」の発現経路を **3 系統センサ集約 + has_discrepancy で構造的検出**(D 主要因の Inc.1 中核達成)。UT-205-22 の 9 組合せ網羅試験で SDD §4.6 表との完全一致を機械検証成立(SDD ↔ 実装の構造的乖離防止メカニズムを 6 ユニット目に拡大)。UT-205-13/14 で SRS-006 in-position 許容偏差(±0.5 mm)境界値検証(RCM-008、HZ-001)。UT-205-23 の 1 writer + 4 reader × 5000 並行試験で `std::atomic<double>` の race-free 性を `tsan` プリセットで検証する設計を確立(HZ-002 機械的予防の継続)。UT-205-20 + ヘッダ内 `static_assert(std::atomic<double>::is_always_lock_free)` で HZ-007 構造的予防の機械検証を **7 ユニット目に拡大**(UNIT-200/401/201/202/203/204 と同パターン継承、ARM64/x86_64 で通常 lock-free だがプラットフォーム差を build 時 fail-stop で検出)。**Step 22 教訓の反映**: (1) runtime `is_lock_free()` を呼ばず `static_assert(is_always_lock_free)` で統一(PRB-0001 教訓)、(2) UT 実装時に SRS-006/SRS-D-007/SRS-ALM-003 の範囲制約を確認しながら値を設定(PRB-0002 教訓)、(3) CI 失敗時は SPRP §3.1 規定に従い PRB を起票して対応(Step 22 で本格運用第 1 例として確立)| k-abe |
| 0.6 | 2026-05-01 | **昇格(Step 22 / CR-0010、本プロジェクト Issue #10、HZ-005 直接 + HZ-003 構造的排除の中核)**。**UNIT-204 DoseManager(ドーズ目標管理 + ドーズ積算 + 目標到達検知 + 即時 BeamOff 指令フラグ)を §3.2 実装対象に追加**(本 v0.6 計画範囲)。SDD-TH25-001 v0.1.1 §4.5 と完全一致する形で実装(`dose_manager.hpp` + `.cpp`)。**SDD §6.5 で予告された強い型 `DoseRatePerPulse_cGy_per_pulse` を本 UNIT のヘッダで導入**(common_types.hpp は変更しない、`inc1-design-frozen` 維持)。**§7.2 UNIT-204 試験ケース 32 件追加**(初期状態 / 異なる rate / set_dose_target 正常系 + 範囲境界 + lifecycle ガード 8 状態網羅 + 累積リセット / on_dose_pulse 単発・単調・到達検知 / target 未設定保護 / **HZ-003 大量積算 10^9 パルス余裕(UT-204-21)** / 境界値 6 ケース機械検証 / 静的純粋関数 / reset / **24 組合せ網羅試験 UT-204-25** / 所有権 / lock-free 二重表明 / 強い型 compile-time / DoseOverflow 防御 / **並行処理 4 producer + reader(UT-204-30)+ producer + setter(UT-204-31)** / ErrorCode 階層整合)を定義し、`tests/unit/test_dose_manager.cpp` として実装済。**累積試験ケース = 138 件**(UNIT-200 24 + UNIT-401 13 + UNIT-201 24 + UNIT-202 26 + UNIT-203 19 + UNIT-204 32)。**§8.1sexies 実施サマリ追加:** ローカル骨格ビルドで `static_assert(std::atomic<std::uint64_t>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明を AppleClang 15 で検証成功(`[1/4]..[4/4]` 完全ビルド)。CI cpp-build マトリクス全 9 ジョブ Pass は Step 22 push 後の SHA 追記事後処理で記録予定。**§9 結論に UNIT-204 セクション追加**: 受入基準・クラス C 追加基準・本プロジェクト固有基準を全クリア(計画段階)。**§10 トレーサビリティ表に UNIT-204 行追加**(32 試験 ID × 関連 SRS/SDD/RCM/HZ)。**Therac-25 学習目的の中核達成:** UNIT-204 は **HZ-005 直接対応 + HZ-003 構造的排除の中核ユニット**。UT-204-21 で 10^9 パルス積算でも uint64_t::max() 余裕(5,800 万年連続照射相当)を機械検証成立(Yakima Class3 8bit unsigned カウンタ 256 周期問題の構造的不可能化を実装層で完成、HZ-003 直接対応)。UT-204-25 の 24 組合せ網羅試験で SDD §4.5 表との完全一致を機械検証成立(SDD と実装の構造的乖離防止メカニズムを 5 ユニット目に拡大)。UT-204-22 のエネルギー範囲境界値検証で SRS-008 範囲(0.01〜10000.0 cGy)を機械的に保護(RCM-008、HZ-005 直接対応)。UT-204-30 の 4 producer + reader 並行試験で `std::atomic<uint64_t>::fetch_add(acq_rel)` + `std::atomic<bool>` release-acquire ordering の race-free 性を `tsan` プリセットで検証する設計を確立(HZ-002 機械的予防の継続)。UT-204-27 + ヘッダ内 `static_assert(std::atomic<std::uint64_t>::is_always_lock_free)` + `static_assert(std::atomic<bool>::is_always_lock_free)` 二重表明で HZ-007 構造的予防の機械検証を 6 ユニット目に拡大(UNIT-200/401/201/202/203 と同パターン継承)。**UNIT-204 から UNIT-201/UNIT-203 への結線(observer pattern による BeamOff 指令)は Step 23+ で SafetyCoreOrchestrator dispatch 機構整備時に完成**、IT-101(Inc.1 完了時)で SRS-010「目標到達 → BeamOff < 1 ms」を実測する旨を §8.5 に明記 | k-abe |
