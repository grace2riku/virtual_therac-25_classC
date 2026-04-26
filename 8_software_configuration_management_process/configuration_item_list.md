# 構成アイテム一覧(CI List)

**ドキュメント ID:** CIL-TH25-001
**バージョン:** 0.5
**最終更新日:** 2026-04-26
**対象製品:** 仮想 Therac-25(Virtual Therac-25) / TH25-SIM-001
**対象リリース:** 0.1.0(初期開発)以降

| 役割 | 氏名 | 所属 | 日付 | 署名 |
|------|------|------|------|------|
| 管理者 | k-abe | — | 2026-04-26 | |
| 承認者 | — | — | — | |

> **本プロジェクトの位置づけ(注記)**
>
> 本ドキュメントは IEC 62304 に基づく医療機器ソフトウェア開発プロセスの学習・参考実装を目的とした **仮想プロジェクト** の成果物である。本 CIL は **Phase 4(Inc.1)着手時点(`inc1-requirements-frozen` 確定: 2026-04-26)** の構成アイテムを網羅する。Phase 2 終了時(v0.1、2026-04-24)で初版、Phase 3 各ステップ(Step 10: CCB/CRR 追加、Step 11: RMF 追加、Step 12: `planning-baseline` 確定)、Phase 4 着手(Step 13: SRS 追加、`inc1-requirements-frozen` 確定)の進捗に応じて昇格してきた。Inc.1 のコード実装着手以降はソースコード・SOUP 正式登録・試験資産の追加に応じて更新する。

---

## 1. 目的と適用範囲

本書は、IEC 62304 箇条 5.1.10(管理対象となる支援項目)および箇条 8.1(構成識別)に基づき、構成管理対象となるすべての構成アイテム(CI)を識別・記録する。本書はベースライン毎にスナップショットを取り、アーカイブの一部として保管する(SCMP-TH25-001 §5 参照)。

## 2. 記録ルール

- 各 CI には一意の `CI ID` を付与する。種別プレフィクス + 連番または略称:
  - `CI-SRC-NNN` — C++ ソースコード(`.cpp` / `.hpp`)
  - `CI-DOC-XXX` — ドキュメント(XXX は略称、例: SDP、SRS)
  - `CI-SOUP-NNN` — SOUP(コンパイラ、標準ライブラリ実装、外部ライブラリ、Sanitizer ランタイム等)
  - `CI-TOOL-NNN` — 開発・検証ツール
  - `CI-TD-NNN` — 試験データ・試験資産(GoogleTest ケース等)
  - `CI-BIN-NNN` — 成果バイナリ・配布物(実行可能モジュール、共有ライブラリ等)
  - `CI-CFG-NNN` — 構成定義ファイル(CMake、`.clang-tidy`、`.clang-format`、CI 設定等)
- バージョンは Git タグ又はコミットハッシュで識別。SOUP 等の外部成果物はベンダ提供バージョン + SHA-256。
- 本一覧は変更のたびに更新し、改訂履歴にベースラインとの関連を記録する。
- SOUP のバージョン固定は **vcpkg を採用する場合**: `vcpkg.json` のマニフェスト + `vcpkg-configuration.json` のベースライン commit、**Conan を採用する場合**: `conanfile.py` または `conanfile.txt` + `conan.lock` で実施する(Inc.1 着手時に確定、SCMP §3.2 参照)。

## 3. ソースコード

> **現時点(Phase 2 終了時)、実装ソースコードは存在しない。** Inc.1 着手時(Step 12 以降)に追加する。SDP §3.1 のインクリメント計画に基づき、以下のサブシステムを想定する。

| CI ID | 名称(予定) | リポジトリ / パス(予定) | 現行バージョン | 安全クラス | 備考 |
|-------|------|-----------------|-------------|----------|------|
| (予定) CI-SRC-001 | ビーム生成・モード制御コア(Inc.1) | `src/th25_ctrl/` | — | C | Inc.1 で作成予定。電子線/X 線モード切替、エネルギー設定、磁場指令、ターンテーブル位置決め |
| (予定) CI-SRC-002 | インターロック/安全監視(Inc.2) | `src/th25_safety/` | — | C | Inc.2 で作成予定。ドア、ターンテーブル位置センサ、ドーズモニタ、ビーム電流監視 |
| (予定) CI-SRC-003 | タスク同期・タイミング保証(Inc.3) | `src/th25_sync/` | — | C | Inc.3 で作成予定。`std::atomic` / `std::mutex` を中心に Therac-25 race condition 対策の中核 |
| (予定) CI-SRC-004 | 操作者 UI・エラー報告・ロギング(Inc.4) | `src/th25_ui/` | — | C(ロガー読取専用部分は B 候補) | Inc.4 で作成予定。明示的エラー表示・バイパス禁止 UI(HZ-006 直接対応) |
| (予定) CI-SRC-005 | 仮想ハードウェアモデル | `src/th25_sim/` | — | C | 電子銃、ベンディング磁石、ターンテーブル、コリメータ、イオンチェンバ、インターロックスイッチの C++ クラス群。SAD で「仮想ハードウェアアーキテクチャ」として明示分離 |

## 4. ドキュメント

Phase 4(Inc.1)着手時点(`inc1-requirements-frozen` 確定)。バージョンは 2026-04-26 時点。

| CI ID | ドキュメント | パス | 現行バージョン | 状態 |
|-------|-----------|------|-------------|------|
| CI-DOC-SDP | ソフトウェア開発計画書 | `5.1_software_development_planning/software_development_plan.md` | 0.1 | ドラフト(セルフ承認、Step 2 で骨子確定) |
| CI-DOC-SRS | ソフトウェア要求仕様書 | `5.2_software_requirements_analysis/software_requirements_specification.md` | 0.1 | ドラフト(セルフ承認、Step 13 / CR-0001 で作成。Inc.1 範囲: 機能 12 / 性能 4 / 入出力 13 / IF 5 / アラーム 5 / セキュリティ 2 / ユーザビリティ 3 / データ 10 / 規制 4 / RCM 6。HZ-001/HZ-005 直接対応、構造的 RCM-018/019 を要求化) |
| CI-DOC-SAD | ソフトウェアアーキテクチャ設計書 | `5.3_software_architecture_design/software_architecture_design.md` | — | 未着手(テンプレートのみ、Inc.1 で作成予定) |
| CI-DOC-SDD | ソフトウェア詳細設計書 | `5.4_software_detailed_design/software_detailed_design.md` | — | 未着手(テンプレートのみ、Inc.1 で作成予定) |
| CI-DOC-UTPR | ユニットテスト計画書/報告書 | `5.5_software_unit_implementation/software_unit_test_plan_report.md` | — | 未着手(テンプレートのみ、Inc.1 で作成予定) |
| CI-DOC-ITPR | 結合試験計画書/報告書 | `5.6_software_integration_testing/software_integration_test_plan_report.md` | — | 未着手(テンプレートのみ、Inc.1 で作成予定) |
| CI-DOC-STPR | システム試験計画書/報告書 | `5.7_software_system_testing/software_system_test_plan_report.md` | — | 未着手(テンプレートのみ、Inc.1 で作成予定) |
| CI-DOC-SMS | ソフトウェアマスタ仕様書 | `5.8_software_release/software_master_specification.md` | — | 未着手(テンプレートのみ、M_final 予定) |
| CI-DOC-SMP | ソフトウェア保守計画書 | `6_software_maintenance_process/software_maintenance_plan.md` | 0.1 | ドラフト(セルフ承認、Step 8 で作成、Therac-25 事故主要因チェックリスト定期適用 §4.8 を組込) |
| CI-DOC-SRMP | ソフトウェアリスクマネジメント計画書 | `7_software_risk_management_process/software_risk_management_plan.md` | 0.1 | ドラフト(セルフ承認、Step 5 で作成、3 層検証 + 2 段冗長 RCM 必須化) |
| CI-DOC-SSC | ソフトウェア安全クラス決定記録 | `7_software_risk_management_process/software_safety_class_determination_record.md` | 0.1 | ドラフト(セルフ承認、Step 1 で作成、HZ-001〜HZ-010 確定) |
| CI-DOC-RMF | リスクマネジメントファイル(ISO 14971) | `7_software_risk_management_process/risk_management_file.md` | 0.1 | ドラフト(セルフ承認、Step 11 で作成。HZ-001〜HZ-010 + RCM-001〜RCM-019、Therac-25 実事故事象シーケンス 5 件、構造的 RCM-018/019 で race condition への構造的対応) |
| CI-DOC-SCMP | ソフトウェア構成管理計画書 | `8_software_configuration_management_process/software_configuration_management_plan.md` | 0.1 | ドラフト(セルフ承認、Step 6 で作成、コンパイラ更新時 HZ-007 リスク評価必須化) |
| CI-DOC-CIL | 構成アイテム一覧(本書) | `8_software_configuration_management_process/configuration_item_list.md` | 0.5 | ドラフト(セルフ承認、Step 13 反映で v0.4 → v0.5 昇格 — `inc1-requirements-frozen` 確定に伴う整合化、SRS v0.1 の §4 反映、§10.2 詳細スナップショット節新設) |
| CI-DOC-CCB | CCB 運用規程 | `8_software_configuration_management_process/ccb_operating_rules.md` | 0.1 | ドラフト(セルフ承認、Step 10 で作成、1 分インターバル §5.4 で正式定義、Therac-25 事故 5 主要因チェック組込) |
| CI-DOC-CRR | 変更要求台帳 | `8_software_configuration_management_process/change_request_register.md` | 0.1 | ドラフト(セルフ承認、Step 10 で空台帳として初期化、Therac-25 事故主要因類型 A〜F 別集計表新設) |
| CI-DOC-SPRP | ソフトウェア問題解決手順書 | `9_software_problem_resolution_process/software_problem_resolution_procedure.md` | 0.1 | ドラフト(セルフ承認、Step 7 で作成、Therac-25 事故 5 主要因チェックリスト §4.3.1 を必須化) |
| CI-DOC-ACL | IEC 62304 監査チェックリスト | `compliance/audit_checklist.md` | テンプレートのまま | 未編集(Inc.1 以降で本プロジェクト向けに整備予定) |
| CI-DOC-README | プロジェクト README | `README.md` | — | 継続更新(製品概要・進捗表・関連規格・参考文献 Leveson & Turner 1993 を含む) |
| CI-DOC-DEVSTEPS | 開発ステップ記録(お手本) | `DEVELOPMENT_STEPS.md` | 0.11 | 継続更新(Step 0〜13 を時系列記録。v0.7 Step 9、v0.8 Step 10、v0.9 Step 11、v0.10 Step 12、v0.11 Step 13 に対応) |
| CI-DOC-CLAUDE | 編集ガイド(AI 支援含む) | `CLAUDE.md` | — | 本プロジェクト固有ルール追記済(Therac-25 事故主要因の第一原理、C++20 固定、PRB- プレフィックス、独立性擬制、DEVELOPMENT_STEPS 更新義務) |
| CI-DOC-UPSTREAM | iec62304_template への修正要望台帳 | `UPSTREAM_FEEDBACK.md` | 0.1 | ドラフト(セルフ承認、Step 4 で作成、初期は空台帳) |

## 5. SOUP(Software of Unknown Provenance)

> **現時点(Phase 2 終了時)、SOUP は未確定**。Inc.1 着手時(Step 12 以降)に vcpkg または Conan のいずれを採用するか確定し、各 SOUP の正式バージョンを本表に登録する。SOUP の評価は SRMP-TH25-001 §4.3 に従い、コンパイラ・標準ライブラリ実装・Sanitizer ランタイムも SOUP として扱う(SCMP-TH25-001 §3.2)。

### 5.1 採用予定 SOUP(候補)

| CI ID(予定) | 名称 | 供給元 | 候補バージョン | ライセンス | 用途 | 関連ハザード |
|-----------|------|-------|-----------|----------|---------|------------|
| CI-SOUP-001 | C++ コンパイラ(clang++) | LLVM Project | 17.0.x 以上(C++20 完全サポート + Sanitizer + libFuzzer) | Apache-2.0 with LLVM Exception | C++ ビルド、各 Sanitizer、libFuzzer | 全般、特に HZ-002/003/007 |
| CI-SOUP-002 | C++ コンパイラ(g++、副コンパイラ) | GNU Project | 13.x 以上(C++20 サポート) | GPL-3.0-with-GCC-RLE | C++ ビルド(コンパイラ非依存性検証用) | 全般 |
| CI-SOUP-003 | C++ 標準ライブラリ実装(libc++) | LLVM Project | clang 同梱 | Apache-2.0 with LLVM Exception | 標準ライブラリ | 全般 |
| CI-SOUP-004 | C++ 標準ライブラリ実装(libstdc++) | GNU Project | gcc 同梱 | GPL-3.0-with-GCC-RLE | 標準ライブラリ(g++ 利用時) | 全般 |
| CI-SOUP-005 | CMake | Kitware | 3.28 以上 | BSD-3-Clause | ビルドシステム | — |
| CI-SOUP-006 | Ninja | Ninja-build | 1.11 以上 | Apache-2.0 | ビルドジェネレータ | — |
| CI-SOUP-007 | パッケージ管理(vcpkg または Conan) | Microsoft / JFrog | 最新(Inc.1 で確定) | MIT(vcpkg)/ MIT(Conan) | 依存ライブラリ取得 | — |
| CI-SOUP-008 | GoogleTest | Google | 1.14 以上 | BSD-3-Clause | ユニット試験フレームワーク | — |
| CI-SOUP-009 | GoogleMock | Google | 1.14 以上(GoogleTest 同梱) | BSD-3-Clause | モック生成 | HZ-001〜010(仮想ハードウェアの試験用モック) |
| CI-SOUP-010 | clang-tidy | LLVM Project | clang 同梱 | Apache-2.0 with LLVM Exception | 静的解析(MISRA C++ 関連チェック有効化) | HZ-002/003/005/007 |
| CI-SOUP-011 | cppcheck | Cppcheck team | 2.13 以上 | GPL-3.0 | 補助静的解析 | — |
| CI-SOUP-012 | Include-What-You-Use(IWYU) | IWYU project | LLVM 同期版 | NCSA | ヘッダ依存最小化 | HZ-007 |
| CI-SOUP-013 | AddressSanitizer ランタイム | LLVM Project | clang/libc++ 同梱 | Apache-2.0 with LLVM Exception | メモリエラー検出 | — |
| CI-SOUP-014 | UndefinedBehaviorSanitizer ランタイム | LLVM Project | clang/libc++ 同梱 | Apache-2.0 with LLVM Exception | UB 検出 | HZ-005/007 |
| CI-SOUP-015 | **ThreadSanitizer ランタイム** | LLVM Project | clang/libc++ 同梱 | Apache-2.0 with LLVM Exception | **race condition 検出(Therac-25 race condition 対策の中核)** | **HZ-002 直接対応** |
| CI-SOUP-016 | libFuzzer | LLVM Project | clang 同梱 | Apache-2.0 with LLVM Exception | カバレッジガイド型ファジング | HZ-005/006 |
| CI-SOUP-017 | llvm-cov | LLVM Project | LLVM 同梱 | Apache-2.0 with LLVM Exception | 網羅率計測(MC/DC 相当) | — |
| CI-SOUP-018 | clang-format | LLVM Project | LLVM 同梱 | Apache-2.0 with LLVM Exception | コード整形 | — |
| (検討中) CI-SOUP-019 | TLA+ / SPIN / Frama-C | LAMSADE / Bell Labs / CEA | 最新 | MIT / 各種 | モデル検査(Inc.3 タスク同期着手時に採用可否判断) | HZ-002/003 |

### 5.2 SOUP 評価の実施計画

- SRMP-TH25-001 §4.3(SOUP 公開異常リストの評価)に従い、Inc.1 着手時に各 SOUP の脆弱性履歴・ライセンス適合性・供給元信頼性を評価する。
- 脆弱性情報源: OSV(<https://osv.dev/>)、GitHub Advisory Database、CVE/NVD、LLVM Security(<https://llvm.org/docs/Security.html>)、GCC Security(<https://gcc.gnu.org/wiki/Security>)
- SHA-256 固定は Inc.1 完了ベースライン(`inc1-baseline`)付与時に vcpkg.json ベースライン commit または conan.lock の生成で実施する計画

## 6. 開発・検証ツール

現時点で運用中のツール。

| CI ID | 種別 | ツール名 | バージョン | 役割 | 校正/資格認定 |
|-------|------|--------|----------|------|------------|
| CI-TOOL-001 | バージョン管理 | Git | 2.40+(ローカル) | 構成管理 | — |
| CI-TOOL-002 | ホスティング | GitHub | — | リモートリポジトリ、CI、Issue、PR。<https://github.com/grace2riku/virtual_therac-25_classC> | — |
| CI-TOOL-003 | CLI | gh(GitHub CLI) | 最新 | GitHub API 操作、リポジトリ作成、run 確認等 | — |
| CI-TOOL-004 | CI/CD | GitHub Actions | — | 自動 lint・ドキュメント検証 | — |
| CI-TOOL-005 | ドキュメント lint | markdownlint-cli2 | CI 側固定(`DavidAnson/markdownlint-cli2-action@v16`) | Markdown 書式検査 | — |
| CI-TOOL-006 | リンクチェック | lychee | CI 側最新(`lycheeverse/lychee-action@v2`) | 内部リンク切れ検出 | — |
| CI-TOOL-007 | 課題追跡 | GitHub Issues | — | CR / PRB 管理 | — |
| (予定) CI-TOOL-008 | C++ コンパイラ(clang++) | LLVM | 17+(SOUP-001 と共通) | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-009 | C++ コンパイラ(g++、副) | GNU | 13+(SOUP-002 と共通) | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-010 | ビルドシステム | CMake + Ninja | SOUP-005, 006 と共通 | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-011 | パッケージ管理 | vcpkg または Conan | SOUP-007 と共通 | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-012 | 静的解析スイート | clang-tidy / cppcheck / IWYU | SOUP-010〜012 と共通 | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-013 | 試験フレームワーク | GoogleTest / GoogleMock | SOUP-008, 009 と共通 | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-014 | 動的解析(Sanitizer) | ASan / UBSan / **TSan** | SOUP-013〜015 と共通 | Inc.1 着手時に確定。**TSan 試験は CI 必須(SRMP §6.1 / SCMP §4.1.1 に準拠)** | — |
| (予定) CI-TOOL-015 | ファジング | libFuzzer | SOUP-016 と共通 | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-016 | 網羅率計測 | llvm-cov | SOUP-017 と共通 | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-017 | コード整形 | clang-format | SOUP-018 と共通 | Inc.1 着手時に確定 | — |
| (予定) CI-TOOL-018 | 脆弱性スキャン | OSV-Scanner / Trivy / Dependabot | 最新 | Inc.1 着手時に CI 統合 | — |
| (検討中) CI-TOOL-019 | モデル検査 | TLA+ / SPIN / Frama-C | SOUP-019 と共通 | Inc.3 着手時に採用可否判断 | — |

## 7. 構成定義ファイル(CFG)

| CI ID | 名称 | パス | 用途 |
|-------|------|------|------|
| CI-CFG-001 | markdownlint 設定 | `.markdownlint-cli2.yaml` | Markdown lint ルール定義(MD013/MD033/MD024/MD058/MD060 等の緩和、日本語・密集テーブル向け。Step 12 で MD058/MD060 を明示無効化し、CI action と npx 最新版の差異を解消) |
| CI-CFG-002 | lychee 設定 | `lychee.toml` | リンクチェック設定(オフラインモード、`{{...}}` プレースホルダ除外) |
| CI-CFG-003 | GitHub Actions ワークフロー(ドキュメント) | `.github/workflows/docs-check.yml` | ドキュメント系 CI 定義(構造検証 + markdownlint + lychee + 日付書式) |
| CI-CFG-004 | Issue テンプレート(PRB) | `.github/ISSUE_TEMPLATE/problem_report.md` | 問題報告起票用(Therac-25 事故要因対応欄、ASan/UBSan/TSan 検出元含む) |
| CI-CFG-005 | Issue テンプレート(CR) | `.github/ISSUE_TEMPLATE/change_request.md` | 変更要求起票用(並行処理影響評価欄含む) |
| CI-CFG-006 | Issue テンプレート config | `.github/ISSUE_TEMPLATE/config.yml` | contact_links(DEVELOPMENT_STEPS / SPRP / SCMP / SSC への誘導)、blank issue 制御 |
| CI-CFG-007 | PR テンプレート | `.github/pull_request_template.md` | PR 作成時の必須情報収集(影響範囲、リスク影響、TSan Pass 確認欄等) |
| CI-CFG-008 | .gitignore | `.gitignore` | Git 対象外ファイルの定義(C++ ビルド成果物、Sanitizer 出力、vcpkg/Conan キャッシュ、Claude Code ローカル設定) |
| (予定) CI-CFG-009 | CMake トップレベル設定 | `CMakeLists.txt` | C++ ビルド定義(Inc.1 着手時に追加) |
| (予定) CI-CFG-010 | clang-tidy 設定 | `.clang-tidy` | 静的解析ルール(MISRA C++ 2023 関連チェック有効化、Inc.1 着手時に追加) |
| (予定) CI-CFG-011 | clang-format 設定 | `.clang-format` | コード整形ルール(Inc.1 着手時に追加) |
| (予定) CI-CFG-012 | パッケージマニフェスト | `vcpkg.json` または `conanfile.txt` | 依存宣言(Inc.1 着手時に確定) |
| (予定) CI-CFG-013 | パッケージロックファイル | `vcpkg-configuration.json` または `conan.lock` | 依存の完全固定(Inc.1 着手時に追加、`inc1-baseline` 付与時に SHA-256 固定) |
| (予定) CI-CFG-014 | コード CI ワークフロー | `.github/workflows/cpp-build.yml`(仮称) | C++ ビルド + clang-tidy + GoogleTest + ASan/UBSan/TSan + llvm-cov + OSV-Scanner(Inc.1 着手時に追加) |
| (予定) CI-CFG-015 | scripts | `scripts/setup_branch_protection.sh` / `scripts/setup_labels.sh` | GitHub 運用補助(テンプレートから引継ぎ済、適用は Inc.1 着手前) |

## 8. 試験データ・試験資産

> **現時点(Phase 2 終了時)、試験資産は存在しない。** Inc.1 着手時(Step 12 以降)に GoogleTest ケースを `tests/` 配下に追加する。

| CI ID(予定) | 名称 | パス(予定) | 用途 |
|-----------|------|------|------|
| (予定) CI-TD-001 | ユニット試験ケース | `tests/unit/test_*.cpp` | 各 UNIT-NNN に対応する GoogleTest ケース |
| (予定) CI-TD-002 | 結合試験シナリオ | `tests/integration/` | 故障注入シナリオ含む結合試験(§5.6、ITPR v0.1 作成時) |
| (予定) CI-TD-003 | システム試験シナリオ | `tests/system/` | 要求ベースのシステム試験(§5.7、STPR v0.1 作成時) |
| (予定) CI-TD-004 | プロパティベース試験/ファジング仕様 | `tests/fuzz/` | libFuzzer 入力シード・コーパス |
| (予定) CI-TD-005 | 試験入力データ | `tests/data/` | YAML/JSON 形式の試験データ |
| (予定) CI-TD-006 | モデル検査仕様 | `model/` | TLA+ / SPIN 仕様(Inc.3 着手時、採用可否判断後) |

## 9. 成果バイナリ・配布物

> **現時点で成果バイナリは存在しない。** リリース時に GitHub Releases 経由で配布する。

| CI ID(予定) | 名称 | 形式 | SHA-256 | 保管場所 |
|-----------|------|------|---------|--------|
| (予定) CI-BIN-001 | th25_ctrl 実行可能バイナリ | ELF / Mach-O / PE | — | GitHub Releases |
| (予定) CI-BIN-002 | th25_ctrl ソース配布 | tar.gz | — | GitHub Releases |
| (予定) CI-BIN-003 | システム試験記録アーカイブ(Sanitizer ログ含む) | tar.gz | — | GitHub Releases |

## 10. ベースライン履歴

ベースライン凍結時に本表に追記する。各ベースラインは Git タグで永続化される(SCMP §5 参照)。

| ベースライン ID | Git タグ | 日付 | 目的 | 承認者 | 含まれる主要 CI | 関連 CR |
|---------------|---------|------|------|-------|--------------|---------|
| BL-20260425-001 | `planning-baseline` | 2026-04-25 | 計画凍結(M0 基盤整備期完了)。RMF 初版・CCB 規程・CRR 整備完了で確定(詳細は §10.1 参照) | k-abe | CI-DOC-{SDP v0.1, SMP v0.1, SRMP v0.1, SSC v0.1, RMF v0.1, SCMP v0.1, SPRP v0.1, CIL v0.4, CCB v0.1, CRR v0.2}、CI-CFG-{001〜008}、CI-DOC-{README, DEVSTEPS v0.10, CLAUDE, UPSTREAM v0.1, ACL} | — |
| BL-20260426-001 | `inc1-requirements-frozen` | 2026-04-26 | Inc.1 要求凍結(SRS v0.1)。HZ-001/HZ-005 直接対応 + 構造的 RCM-018/019 要求化(詳細は §10.2 参照) | k-abe | 上記 + CI-DOC-SRS v0.1、CIL v0.5、CRR v0.3、DEVSTEPS v0.11 | CR-0001(GitHub Issue #1) |
| (予定) BL-YYYYMMDD-NNN | `inc1-design-frozen` | — | Inc.1 設計凍結(SAD v0.1 + SDD v0.1) | — | 上記 + CI-DOC-{SAD, SDD} | — |
| (予定) BL-YYYYMMDD-NNN | `inc1-baseline` | — | Inc.1 完了(試験合格) | — | 上記 + CI-SRC-{001, 005}、CI-TD-* | — |
| (予定) BL-YYYYMMDD-NNN | `v1.0.0` | — | 初回リリース | — | 全 CI | — |

### 10.1 BL-20260425-001(`planning-baseline`)詳細スナップショット

SCMP-TH25-001 §5.2 の「ベースライン状態記録」に従い、`planning-baseline` 確定時点の状態を以下に記録する。

| 項目 | 内容 |
|------|------|
| ベースライン ID | `BL-20260425-001` |
| Git タグ | `planning-baseline` |
| ベースライン日 | 2026-04-25 |
| Git コミット | `e71a7c5fe48ad3d73b726b96771eab66bada2fb9`(短縮: `e71a7c5`)|
| 目的 | M0(計画期+支援プロセス計画+インフラ整備)完了凍結。Phase 4(Inc.1 着手)への前提として、開発計画・リスクマネジメント・構成管理・問題解決・保守の各プロセス計画を確定し、RMF 初版 + CCB 規程 + 空 CRR の整備を完了した状態 |
| 承認者・承認日 | k-abe、2026-04-25 |
| 関連 CR | 無し(Phase 0〜3 の活動はすべて `DEVELOPMENT_STEPS.md` に記録された立ち上げ活動であり、CCB-TH25-001 施行前および直後の継続活動に該当) |

**含まれる主要 CI(バージョン固定):**

| CI ID | 名称 | バージョン | 備考 |
|-------|-----|----------|------|
| CI-DOC-SSC | ソフトウェア安全クラス決定記録 | 0.1 | Step 1 / HZ-001〜HZ-010 確定、クラス C 決定 |
| CI-DOC-SDP | ソフトウェア開発計画書 | 0.1 | Step 2 / 骨子確定、C++20 固定、V 字+インクリメンタル |
| CI-DOC-SRMP | ソフトウェアリスクマネジメント計画書 | 0.1 | Step 5 / 3 層検証(TSan+モデル検査+境界値)+ 2 段冗長 RCM 必須化 |
| CI-DOC-SCMP | ソフトウェア構成管理計画書 | 0.1 | Step 6 / CR 区分 3 段階、並行処理時 TSan 必須、コンパイラ更新時 HZ-007 評価 |
| CI-DOC-SPRP | ソフトウェア問題解決手順書 | 0.1 | Step 7 / Therac-25 事故 5 主要因チェックリスト §4.3.1 必須化 |
| CI-DOC-SMP | ソフトウェア保守計画書 | 0.1 | Step 8 / 主要因チェックリスト定期適用 §4.8、全コードベース水平展開 §5.4 |
| CI-DOC-CIL | 構成アイテム一覧(本書) | 0.4 | Step 9 初版、Step 10/11/12 で昇格 |
| CI-DOC-CCB | CCB 運用規程 | 0.1 | Step 10 / 1 分インターバル §5.4 正式定義 |
| CI-DOC-CRR | 変更要求台帳 | 0.2 | Step 10 空台帳、Step 12 で参照文書整合化 |
| CI-DOC-RMF | リスクマネジメントファイル(ISO 14971) | 0.1 | Step 11 / HZ-001〜010 + RCM-001〜019 登録、Therac-25 実事故事象シーケンス 5 件 |
| CI-DOC-{SRS, SAD, SDD, UTPR, ITPR, STPR, SMS} | 各ライフサイクル成果物 | 未着手(テンプレートのまま) | Inc.1 着手後に順次作成 |
| CI-DOC-ACL | IEC 62304 監査チェックリスト | テンプレートのまま | Inc.1 以降で本プロジェクト向け整備 |
| CI-DOC-README | プロジェクト README | 継続更新 | Phase 3 完了時点の進捗表を反映 |
| CI-DOC-DEVSTEPS | 開発ステップ記録 | 0.10 | Step 0〜12 時系列記録 |
| CI-DOC-CLAUDE | 編集ガイド | — | Therac-25 固有ルール反映済 |
| CI-DOC-UPSTREAM | upstream フィードバック台帳 | 0.1 | 空台帳 |
| CI-CFG-{001〜008} | 運用構成定義 | — | markdownlint / lychee / GH Actions / Issue/PR テンプレート / `.gitignore` |

**検証成果物(機械的独立性の代替):**

- GitHub Actions `Documentation Checks` ワークフロー: Step 5〜11 のすべての push で `success`(構造検証 + markdownlint + lychee + 日付書式ポリシー)
  - Step 11 最終確定コミット `88a3b170`: 2026-04-24T14:18:35Z `success`
  - Step 11 RMF 作成コミット `117863a1`: 2026-04-24T14:18:03Z `success`
  - それ以前の Step 5〜10 も同様に全件 `success`
- ローカル lint 結果(本 Step 12 時点、`npx markdownlint-cli2 "**/*.md"`): 0 error(Step 12 で `.markdownlint-cli2.yaml` に MD058/MD060 の明示無効化を追加し、CI 側 `DavidAnson/markdownlint-cli2-action@v16` との環境差異を解消)
- lychee ローカル実行: CI の `lycheeverse/lychee-action@v2` による内部リンクチェックが全件 `success` のため代替とする

**残存する既知の限界:**

- Inc.1 以降の成果物(SRS / SAD / SDD / UT / IT / ST / SMS)は本ベースライン時点で未着手。これは V 字モデル+インクリメンタル方式の計画通り(SDP §3.1)
- SOUP の正式バージョン固定(vcpkg または Conan ロックファイル + SHA-256)は Inc.1 着手時点で実施(SCMP §3.2、`inc1-baseline` 時点で確定)
- SPRP §4.3.1 Therac-25 事故 5 主要因チェックリストの **運用実績はゼロ**(CCB 施行直後のため)。Phase 4(Inc.1 着手)以降で運用実績を蓄積する

### 10.2 BL-20260426-001(`inc1-requirements-frozen`)詳細スナップショット

SCMP-TH25-001 §5.2 の「ベースライン状態記録」に従い、Inc.1 要求凍結時点の状態を以下に記録する。本プロジェクトでは「案 a 早期凍結」方式を採用(DEVELOPMENT_STEPS Step 12 採用根拠参照)。SAD/SDD で SRS 改訂が必要となった場合は CR + 新タグ(`inc1-requirements-frozen-v2`)で対応する。

| 項目 | 内容 |
|------|------|
| ベースライン ID | `BL-20260426-001` |
| Git タグ | `inc1-requirements-frozen` |
| ベースライン日 | 2026-04-26 |
| Git コミット | _(Step 13 の最終コミット時に追記)_ |
| 目的 | Inc.1(ビーム生成・モード制御コア)の要求事項を SAD/SDD 着手前に不可逆に凍結。HZ-001(電子モード+X 線ターゲット非挿入)直接対応 + HZ-005(線量計算誤り)対応 + 構造的 RCM-018/019(UI 層分離・メッセージパッシング)を SRS で要求化済の状態を確定 |
| 承認者・承認日 | k-abe、2026-04-26 |
| 関連 CR | CR-0001(GitHub Issue #1)— Inc.1 SRS-TH25-001 v0.1 作成、MAJOR 区分、CCB-TH25-001 本格運用第 1 例 |

**含まれる主要 CI(本ベースラインで凍結):**

| CI ID | 名称 | バージョン | 備考 |
|-------|-----|----------|------|
| CI-DOC-SRS | ソフトウェア要求仕様書 | 0.1 | **本ベースラインで初凍結**。Inc.1 範囲機能要求 + 全 Inc 共通非機能要求初版 |
| CI-DOC-CIL | 構成アイテム一覧(本書) | 0.5 | Step 13 反映で v0.4 → v0.5 昇格 |
| CI-DOC-CRR | 変更要求台帳 | 0.3 | CR-0001 を §4 本体表に最初のエントリとして登録(本格運用開始) |
| CI-DOC-DEVSTEPS | 開発ステップ記録 | 0.11 | Step 13 を時系列に追記 |
| CI-DOC-{SDP, SMP, SRMP, SSC, SCMP, SPRP, RMF, CCB, ACL, README, CLAUDE, UPSTREAM} | 既存ドキュメント群 | `planning-baseline` 時と同一 | 本 Step では未変更 |
| CI-DOC-{SAD, SDD, UTPR, ITPR, STPR, SMS} | 各設計・試験・リリース成果物 | 未着手 | Inc.1 後続 Step(SAD: Step 15、SDD: Step 16、UT/IT/ST: Step 17+)で順次作成 |
| CI-CFG-{001〜008} | 運用構成定義 | `planning-baseline` 時と同一 | 本 Step では未変更 |

**検証成果物(機械的独立性の代替):**

- 本 SRS 自体の検証: §8 要求事項の検証チェックリスト(7 項目)を全クリア。**1 分インターバル**(CCB §5.4): GitHub Issue #1 起票 2026-04-26T01:06:44Z → 本コミット作成時点で 1 分以上経過(SRS 作成自体に十分な時間を要するため自然に満たされる)
- ローカル lint(`npx markdownlint-cli2 "**/*.md"`): 0 error / 25 files
- GitHub Actions `Documentation Checks`: Step 12 までの全 push で `success` 履歴。本 Step push 後の結果は CIL v0.6 改訂で確認・追記
- CCB プロセス遵守チェックリスト(CR-0001 Issue Body §「CCB プロセス遵守」): 本 SRS 提出時点ですべて完了
- SPRP §4.3.1 Therac-25 事故 5 主要因チェックリスト: 本 CR は要求化(実装ではない)のため、形式的・予防的観点で適用済(類型 A: RCM-018/019 構造的予防、類型 F: RCM-018 物理隔離による前提保護)

**Inc.1 残作業(後続 Step で実施):**

- Step 14: SOUP 正式選定(vcpkg または Conan)+ C++20 プロジェクト骨格(CMake + GoogleTest + Sanitizer)
- Step 15: SAD-TH25-001 v0.1 作成(SRS-RCM-018/019 を §5.3.5 分離設計として確定)→ `inc1-design-frozen` 候補
- Step 16: SDD-TH25-001 v0.1 作成
- Step 17+: コード実装 + UT + IT + ST + `inc1-baseline` タグ付与

**残存する既知の限界:**

- §9.2 SRS ↔ ARCH ↔ SDD ↔ UT/IT/ST トレーサビリティマトリクスは本 v0.1 では枠組みのみ。SAD v0.1(Step 15)で ARCH 列を、SDD v0.1(Step 16)で SDD 列を、各試験計画(Step 17+)で試験列を順次充填する
- Inc.2〜4 範囲の機能要求は本 v0.1 では未定義(SDP §3.1 の計画通り、各 Inc 着手時に SRS v0.2/v0.3/v0.4 で追加)
- SAD/SDD で SRS 不備が発見された場合、案 a 方針により SRS 改訂(v0.1.1 等)+ 新タグ(`inc1-requirements-frozen-v2`)で対応(SCMP §5.1 の凍結タグ命名規則の `-vN` 拡張は実発生時に検討)

## 11. 改訂履歴

| バージョン | 日付 | 変更内容 | 変更者 |
|----------|------|---------|--------|
| 0.1 | 2026-04-24 | 初版作成(Phase 2 終了時点の CI を網羅登録)。**ドキュメント 22 件**(うち実体化済 9 件: SDP / SMP / SRMP / SSC / SCMP / SPRP / CIL 自身 / DEVSTEPS / UPSTREAM、テンプレートのまま 9 件、追記済 4 件: README / CLAUDE / ACL は未編集)、**SOUP 候補 19 件**(C++ 標準ライブラリ実装 + コンパイラ + Sanitizer ランタイム + テストフレームワーク等を含む。Inc.1 着手時に正式登録)、**ツール 7 件運用中 + 12 件予定**、**構成定義ファイル 8 件運用中 + 7 件予定**、**ベースライン 5 件すべて予定**(planning-baseline は Phase 3 完了時に確定)。Therac-25 学習目的の特徴として、ThreadSanitizer ランタイム(CI-SOUP-015)を「race condition 検出 / HZ-002 直接対応」と明示分類し、`vcpkg.json` ベースライン commit または `conan.lock` による SOUP の SHA-256 固定計画を §5.2 で明記 | k-abe |
| 0.2 | 2026-04-24 | Step 10(CCB-TH25-001 v0.1 + CRR-TH25-001 v0.1 作成)に伴う整合化。**(1) §4 ドキュメント:** CI-DOC-CCB を「未着手」→ v0.1 に昇格(1 分インターバル §5.4 正式定義、Therac-25 事故 5 主要因チェック組込)、CI-DOC-CRR を「未着手」→ v0.1 に昇格(空台帳、Therac-25 事故主要因類型別集計表 §7.1 新設)。**(2) 自己参照:** CI-DOC-CIL を v0.1 → v0.2 に更新、冒頭バージョン v0.2 + 最終更新日 2026-04-24 を整合更新。**(3) 派生ドキュメント更新漏れ予防:** 姉妹プロジェクト VIP の運用教訓「CIL の派生ドキュメント更新漏れ」(VIP では 11 度発生)を反映し、CCB/CRR 作成と同時に CIL を昇格。Step 10 のコミットに CCB/CRR/CIL/DEVSTEPS/README を一括含める | k-abe |
| 0.3 | 2026-04-24 | Step 11(RMF-TH25-001 v0.1 作成)に伴う整合化。**(1) §4 ドキュメント:** CI-DOC-RMF を「未着手」→ v0.1 に昇格(HZ-001〜HZ-010 + RCM-001〜RCM-019、Therac-25 実事故事象シーケンス 5 件、構造的 RCM-018/019 で race condition への構造的対応)。**(2) 自己参照:** CI-DOC-CIL を v0.2 → v0.3 に更新、冒頭バージョン v0.3 + 最終更新日 2026-04-24 を整合更新。**(3) 派生ドキュメント更新漏れ予防:** RMF 作成と同時に CIL を昇格。Step 11 のコミットに RMF/CIL/DEVSTEPS/README を一括含める | k-abe |
| 0.4 | 2026-04-25 | Step 12(`planning-baseline` 確定)に伴う整合化。**(1) §10 ベースライン履歴:** 予定行 `(予定) BL-YYYYMMDD-001` を実確定行 `BL-20260425-001` に昇格し、Git タグ `planning-baseline` / 日付 2026-04-25 / 承認者 k-abe / 含まれる主要 CI をすべてバージョン付きで確定記載。**(2) §10.1 詳細スナップショット節新設**(SCMP §5.2 の状態記録項目に準拠): ベースライン ID / Git タグ / ベースライン日 / 目的 / 承認者 / 関連 CR / 含まれる全 CI とバージョン一覧 / 検証成果物(CI 全 success 履歴 + ローカル lint 0 error)/ 残存する既知の限界(SOUP 未固定等)を網羅。**(3) 派生ドキュメント追随漏れ一括修正:** CI-DOC-DEVSTEPS のバージョンを v0.6(古) → v0.10(最新) に修正(Step 9/10/11 時点の追随漏れ、VIP 類型に合致)。**(4) CI-CFG-001 の説明更新:** `.markdownlint-cli2.yaml` で MD058/MD060 を明示無効化した旨を反映。**(5) 自己参照:** CI-DOC-CIL を v0.3 → v0.4、冒頭バージョン・最終更新日を 2026-04-25 に整合更新。**(6) 冒頭位置づけ注記更新:** 「Phase 2 終了時点」→「Phase 3 完了時点(`planning-baseline` 確定: 2026-04-25)」に表現更新 | k-abe |
| 0.5 | 2026-04-26 | Step 13(`inc1-requirements-frozen` 確定 / CR-0001)に伴う整合化。**(1) §4 ドキュメント:** CI-DOC-SRS を「未着手」→ v0.1 に昇格(Inc.1 範囲: 機能 12 / 性能 4 / 入出力 13 / IF 5 / アラーム 5 / セキュリティ 2 / ユーザビリティ 3 / データ 10 / 規制 4 / RCM 6、HZ-001/HZ-005 直接対応、構造的 RCM-018/019 を要求化)。**(2) §10 ベースライン履歴:** 予定行 `(予定) inc1-requirements-frozen` を実確定行 `BL-20260426-001` に昇格、Git タグ `inc1-requirements-frozen` / 日付 2026-04-26 / 承認者 k-abe / 関連 CR `CR-0001` を確定記載。**(3) §10.2 詳細スナップショット節新設**: SCMP §5.2 準拠の状態記録(項目 / 含まれる CI / 検証成果物 / Inc.1 残作業 / 残存する既知の限界)を網羅。**(4) 自己参照:** CI-DOC-CIL を v0.4 → v0.5、CI-DOC-DEVSTEPS を v0.10 → v0.11、冒頭バージョン・最終更新日を 2026-04-26 に整合更新。**(5) 冒頭位置づけ注記更新:** 「Phase 3 完了時点」→「Phase 4(Inc.1)着手時点(`inc1-requirements-frozen` 確定: 2026-04-26)」に表現更新 | k-abe |

## 付録 A: CIL 更新時チェックリスト

CIL を更新する際は、以下の順序で **全走査** すること。「今回変更する CI」だけに注力すると、自己参照(CI-DOC-CIL)や過去ステップで完成済みだが未反映の CI が漏れやすい。

- [ ] 今回の CR で直接変更する CI の現行バージョン・状態を更新した
- [ ] **自己参照(CI-DOC-CIL)の現行バージョンと状態を、本 CIL 自身の昇格後バージョンに更新した**
- [ ] 過去ステップで完成したが CIL に未反映の CI(ソースコード・ドキュメント・SOUP・ツール・試験資産・成果バイナリ)が無いか §3〜§9 を全走査した
- [ ] ベースライン履歴(§10)への追加が必要か判定し、必要なら追記した
- [ ] 改訂履歴(§11)に今回の昇格エントリを追加した
- [ ] 関連する CRR のエントリ・CCB 議事録と CI バージョンが整合していることを確認した
- [ ] **冒頭メタ(バージョン・最終更新日)を整合更新した**(姉妹プロジェクト VIP の運用教訓)
