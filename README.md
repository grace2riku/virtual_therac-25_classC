# 仮想 Therac-25(Virtual Therac-25) — IEC 62304 クラス C 準拠ドキュメント作成プロジェクト

**本リポジトリは、IEC 62304:2006+A1:2015(JIS T 2304:2017)に基づく医療機器ソフトウェア開発プロセスの学習・参考実装を目的とした仮想プロジェクトです。**

実機・ハードウェアは存在せず、PC 単体で動作する仮想シミュレータ(ハードウェアモデル含む)を対象として、ソフトウェア安全クラス **C**(死亡又は重傷の可能性)の要求事項に則ったライフサイクル成果物を作成します。

## 題材: Therac-25 と歴史的事故

Therac-25 は 1982 年に AECL(Atomic Energy of Canada Limited)が開発した放射線治療装置(医療用リニアアクセラレータ)です。高エネルギーの透過性放射線を患者の患部に照射し、周囲の正常組織を傷つけることなく、がん細胞を破壊することを目的として開発されました。

しかし、**1985 年 6 月から 1987 年 1 月にかけて、米国・カナダの医療機関において計 6 件の放射線過剰照射事故が発生し、6 名の患者が重症または死亡に至りました**。推定被曝線量は処方量の 100 倍以上に達したケースもあり、医療機器ソフトウェアの安全性を論じるうえで最も著名な事例となっています。

主な技術的要因として以下が報告されています(N. G. Leveson, C. S. Turner, "An Investigation of the Therac-25 Accidents", IEEE Computer, 1993 ほか)。

- ハードウェアインターロック(Therac-6/20 までは機械式連動)を廃止し、ソフトウェア単独で安全機能を実現した設計判断
- 操作者入力処理とビーム設定間の **タイミング競合(race condition)** により、電子ビーム用高出力モードのまま X 線ターゲットが挿入されない状態でビーム照射
- 共有変数のカウンタ・オーバーフローによる安全チェックのバイパス
- 旧機種(Therac-6/20)のコード再利用の際に、依存していたハードウェアインターロックが失われた前提の見直し漏れ
- 暗号的なエラーメッセージ(例: "MALFUNCTION 54")と、操作者による「P」キー押下でのバイパス常態化
- ソフトウェア起因のハザード解析・FMEA / FTA の欠如、独立レビューの不在
- 単独開発者による安全クリティカルコードの実装

**本プロジェクトは、Therac-25 の事故を IEC 62304 のプロセスに照らし直し、「どの成果物・どの活動があれば事故は避けられたか」を追体験することを狙います。**

## 目的

- IEC 62304 の開発プロセス(箇条 5)、保守(箇条 6)、リスクマネジメント(箇条 7)、構成管理(箇条 8)、問題解決(箇条 9)の各プロセスを、実プロジェクトと同等の粒度で体験・文書化する。
- Therac-25 という歴史的な公知事例を題材に、クラス C 特有の要求事項(箇条 5.3.5 / 5.4.2〜5.4.4 / 5.5.4 / 5.7.4)の意義と実装を学ぶ。
- Markdown + Git + Pull Request ベースのドキュメント運用を実践する。

## 対象製品の概要

| 項目 | 内容 |
|------|------|
| 製品名 | 仮想 Therac-25(Virtual Therac-25) |
| 型式 | TH25-SIM-001 |
| ソフトウェア名称 | Therac-25 Virtual Control Software(TH25-CTRL) |
| ソフトウェア安全クラス | **C**(IEC 62304) |
| 実装言語 | **C++20**(PC 単独動作の仮想シミュレータ) |
| ライフサイクルモデル | V字モデル(インクリメンタル方式) |
| 開発インクリメント(予定) | Inc.1 ビーム生成・モード制御コア → Inc.2 インターロック/安全監視 → Inc.3 タスク同期・タイミング保証 → Inc.4 操作者 UI・エラー報告・ロギング |

### スコープの補足

- 本プロジェクトは学習目的の **仮想実装** であり、実ビーム・高電圧・磁場は扱わない。ハードウェア(電子銃、ベンディング磁石、ターゲット、コリメータ、ターンテーブル、イオンチェンバ、ドーズモニタ、インターロックスイッチ)は C++ によるソフトウェアモデルで置換する。
- 安全クラスの判定は、**仮に本ソフトウェアが実機の Therac-25 相当機に搭載されたと仮定した場合**の危害重大度に基づいて行う(詳細は [SSC-TH25-001](./7_software_risk_management_process/software_safety_class_determination_record.md) を参照)。

## ベース

本プロジェクトは [grace2riku/iec62304_template](https://github.com/grace2riku/iec62304_template) をベースとし、同テンプレートの構造・雛形を継承しています。また、姉妹プロジェクトである [grace2riku/virtual_infusion_pump_classC](https://github.com/grace2riku/virtual_infusion_pump_classC) の運用ルール(`DEVELOPMENT_STEPS.md` 更新義務、`PRB-` プレフィックス、単独開発下の独立性擬制)も踏襲しています。テンプレート本体の更新は `upstream` remote から随時確認可能です。

## ドキュメント進捗

### 箇条 5 ソフトウェア開発プロセス

| 箇条 | ドキュメント | ファイル | 状態 |
|------|-----------|--------|------|
| 5.1 | ソフトウェア開発計画書(SDP) | [`5.1_.../software_development_plan.md`](./5.1_software_development_planning/software_development_plan.md) | 🟡 v0.1 骨子(テンプレートから派生・製品固有化) |
| 5.2 | ソフトウェア要求仕様書(SRS) | [`5.2_.../software_requirements_specification.md`](./5.2_software_requirements_analysis/software_requirements_specification.md) | 🟡 v0.1(Inc.1 範囲: ビーム生成・モード制御コア。機能 12 / 性能 4 / 入出力 13 / IF 5 / アラーム 5 / セキュリティ 2 / ユーザビリティ 3 / データ 10 / 規制 4 / RCM 6。HZ-001/HZ-005 直接対応、構造的 RCM-018/019 を要求化。`inc1-requirements-frozen`(BL-20260426-001)で凍結) |
| 5.3 | ソフトウェアアーキテクチャ設計書 | [`5.3_.../software_architecture_design.md`](./5.3_software_architecture_design/software_architecture_design.md) | ⬜ 未着手(テンプレートのみ) |
| 5.4 | ソフトウェア詳細設計書 | [`5.4_.../software_detailed_design.md`](./5.4_software_detailed_design/software_detailed_design.md) | ⬜ 未着手(テンプレートのみ) |
| 5.5 | ユニット試験計画書/報告書 | [`5.5_.../software_unit_test_plan_report.md`](./5.5_software_unit_implementation/software_unit_test_plan_report.md) | ⬜ 未着手(テンプレートのみ) |
| 5.6 | 結合試験計画書/報告書 | [`5.6_.../software_integration_test_plan_report.md`](./5.6_software_integration_testing/software_integration_test_plan_report.md) | ⬜ 未着手(テンプレートのみ) |
| 5.7 | システム試験計画書/報告書 | [`5.7_.../software_system_test_plan_report.md`](./5.7_software_system_testing/software_system_test_plan_report.md) | ⬜ 未着手(テンプレートのみ) |
| 5.8 | ソフトウェアマスタ仕様書 | [`5.8_.../software_master_specification.md`](./5.8_software_release/software_master_specification.md) | ⬜ 未着手(テンプレートのみ) |

### 箇条 6〜9 ライフサイクルプロセス

| 箇条 | ドキュメント | ファイル | 状態 |
|------|-----------|--------|------|
| 6 保守 | ソフトウェア保守計画書(SMP) | [`6_.../software_maintenance_plan.md`](./6_software_maintenance_process/software_maintenance_plan.md) | 🟡 v0.1(GitHub Issues セルフフィードバック窓口、SemVer、SOUP・コンパイラ監視、Therac-25 事故主要因チェックリスト定期適用 §4.8、ホットフィックス時の全コードベース水平展開) |
| 7 リスク | ソフトウェアリスクマネジメント計画書(SRMP) | [`7_.../software_risk_management_plan.md`](./7_software_risk_management_process/software_risk_management_plan.md) | 🟡 v0.1(HZ-001〜HZ-010 を初期ハザードとしてプロセス計画策定、TSan + モデル検査 + 境界値試験の 3 層検証を必須化) |
| 7 リスク | ソフトウェア安全クラス決定記録(SSC) | [`7_.../software_safety_class_determination_record.md`](./7_software_risk_management_process/software_safety_class_determination_record.md) | 🟡 v0.1(クラス C 決定、Therac-25 事故事例に基づく根拠記録済み) |
| 7 リスク | リスクマネジメントファイル(RMF, ISO 14971) | [`7_.../risk_management_file.md`](./7_software_risk_management_process/risk_management_file.md) | 🟡 v0.1(HZ-001〜HZ-010 + RCM-001〜RCM-019、Therac-25 実事故事象シーケンス 5 件、P 値を実証発現頻度ベースで初期推定、構造的 RCM-018/019 で race condition への構造的対応) |
| 8 構成管理 | ソフトウェア構成管理計画書(SCMP) | [`8_.../software_configuration_management_plan.md`](./8_software_configuration_management_process/software_configuration_management_plan.md) | 🟡 v0.1(GitHub Flow + CR 区分 3 段階、C++ エコシステム対応 SOUP 識別、並行処理関連変更時の TSan 必須化、コンパイラ更新時の HZ-007 リスク評価を規定) |
| 8 構成管理 | 構成アイテム一覧(CIL) | [`8_.../configuration_item_list.md`](./8_software_configuration_management_process/configuration_item_list.md) | 🟡 v0.6(Phase 4(Inc.1)着手 + プロジェクト骨格完成時点。`inc1-requirements-frozen`(BL-20260426-001)確定済。SOUP 13 件正式登録(vcpkg ベースライン commit `c3867e714d...` で完全固定、TSan ランタイムを HZ-002 直接対応として CI 常時実行)、ツール 8 件運用中、構成定義 6 件運用中、§10.1 planning-baseline / §10.2 inc1-requirements-frozen 詳細スナップショット) |
| 8 構成管理 | CCB 運用規程 | [`8_.../ccb_operating_rules.md`](./8_software_configuration_management_process/ccb_operating_rules.md) | 🟡 v0.1(1 分インターバル正式定義 §5.4、Therac-25 事故 5 主要因チェック組込、並行処理レビュア役割追加、TSan/Sanitizer 全種再実行を決定記録に必須化) |
| 8 構成管理 | 変更要求台帳 | [`8_.../change_request_register.md`](./8_software_configuration_management_process/change_request_register.md) | 🟡 v0.4(Step 13/14 で CR-0001(Inc.1 SRS、CLOSED)/ CR-0002(Inc.1 C++20 骨格、VERIFICATION)を §4 本体表に登録。両件 MAJOR・安全関連・Therac-25 主要因類型 A/F 構造的予防。集計表に Step 13/14 完了時行を追加) |
| 9 問題解決 | ソフトウェア問題解決手順書(SPRP) | [`9_.../software_problem_resolution_procedure.md`](./9_software_problem_resolution_process/software_problem_resolution_procedure.md) | 🟡 v0.1(GitHub Issue ベースの PRB-NNNN 運用、根本原因 13 分類、Therac-25 事故 5 主要因チェックリストを Critical 案件で必須化、傾向分析に Therac-25 主要因類型別集計を組込) |

### 補助

| 目的 | ファイル |
|------|--------|
| **開発ステップ記録(お手本用)** | [`DEVELOPMENT_STEPS.md`](./DEVELOPMENT_STEPS.md) |
| 上流テンプレートへのフィードバック記録 | [`UPSTREAM_FEEDBACK.md`](./UPSTREAM_FEEDBACK.md) |
| 条項別 監査チェックリスト | [`compliance/audit_checklist.md`](./compliance/audit_checklist.md) |
| 作業・編集ガイド(AI 支援含む) | [`CLAUDE.md`](./CLAUDE.md) |

## 技術スタック

| カテゴリ | ツール | 状態 |
|---------|-------|------|
| 言語 | **C++20** | 確定(SDP §6.1) |
| ビルド | **CMake 3.28+ / Ninja 1.11+** | 確定(Step 14、`CMakeLists.txt` + `CMakePresets.json`) |
| パッケージ管理 | **vcpkg**(ベースライン commit `c3867e714d...` = リリース 2026.03.18 完全固定) | 確定(Step 14、`vcpkg.json` の `builtin-baseline`) |
| コンパイラ | **clang++ 17+ / g++ 13+** | 確定(Step 14、CI 両対応) |
| 静的解析 | **clang-tidy 17**(`.clang-tidy` 整備済)/ cppcheck / IWYU(Step 17+ 検討) | 一部確定 |
| 動的解析 | **AddressSanitizer / UndefinedBehaviorSanitizer / ThreadSanitizer** | 確定(Step 14、CMake `TH25_SANITIZER` プリセット `asan` / `ubsan` / `tsan`) |
| 試験 | **GoogleTest / GoogleMock 1.14+**(vcpkg 経由) | 確定(Step 14、smoke test 1 件) |
| 網羅率 | llvm-cov(MC/DC 相当の分岐網羅) | Step 17+ で導入予定 |
| コーディング規約 | **MISRA C++ 2023 関連 + CERT C++**(`.clang-tidy` の `bugprone-*` / `cert-*` / `concurrency-*` / `cppcoreguidelines-*` / `hicpp-*` / `misc-*` / `modernize-*` / `performance-*` / `portability-*` / `readability-*` で実現) | 確定(Step 14) |
| コード整形 | **clang-format**(LLVM ベース、`.clang-format` 整備済) | 確定(Step 14) |
| 構成管理 | **Git / GitHub** | 確定 |
| CI | **GitHub Actions**(Documentation Checks + cpp-build) | 確定(Step 14、`.github/workflows/cpp-build.yml`) |
| ドキュメント検証 | **markdownlint-cli2 / lychee** | 確定 |

詳細は SDP §6.2 および 構成アイテム一覧(CIL-TH25-001 v0.6)§5/§6/§7 を参照。

## 開発プロセスの方針

- **V字モデル(インクリメンタル方式)** を採用し、機能単位で V字サイクルを一周する。各インクリメントは Therac-25 事故の主要因に対応させ、事故を防ぐ設計・検証活動を段階的に積み上げる。
- **テスト駆動開発(TDD)** を原則とし、MISRA C++ + 静的/動的解析 + 徹底したユニット試験で品質を担保する。
- **スレッド/タスク間の競合解析(TSan、モデル検査)** をクラス C 固有要求事項 5.4.x / 5.7.4 の要として扱う(Therac-25 の直接原因が race condition であったため)。
- **Pull Request + GitHub Actions CI** による自動検証で、単独開発下でも機械的独立性を確保する(Therac-25 における「単独開発者+独立レビュー欠如」を意図的に擬制で補う)。
- ベースラインは Inc. 完了時およびリリース時にタグで凍結する。

詳細は [SDP(SDP-TH25-001)](./5.1_software_development_planning/software_development_plan.md) を参照。

## CI(GitHub Actions)

Pull Request・push ごとに以下を自動検証します(`.github/workflows/docs-check.yml`):

1. 必須ディレクトリ・主要テンプレートファイルの存在確認
2. Markdown lint(`markdownlint-cli2`、日本語・密集テーブル向けに調整済)
3. 内部リンク切れ検出(`lychee`、オフラインモード)
4. 日付書式(ISO 8601 `YYYY-MM-DD` 強制)

ローカルで事前確認する場合(ドキュメント):

```bash
npx markdownlint-cli2 "**/*.md"
lychee --offline --include-fragments './**/*.md'
```

ローカルで事前確認する場合(C++ コード、Step 14 以降):

```bash
# vcpkg をローカルにセットアップしている場合
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset debug         # configure(GoogleTest を vcpkg 経由で取得)
cmake --build --preset debug
ctest --preset debug

# vcpkg を持たない開発環境では tests を無効化して骨格のみ検証可能
cmake -B build -DTH25_BUILD_TESTS=OFF -G Ninja
cmake --build build
```

CI 用には `.github/workflows/cpp-build.yml` が Ubuntu + clang-17 / gcc-13 上で各 Sanitizer プリセット(asan / ubsan / tsan)+ debug を自動実行する(Step 14 で導入)。網羅率(llvm-cov)・ファジング(libFuzzer)・脆弱性スキャン(OSV-Scanner / Dependabot)は Step 17+ で順次追加予定。

## 関連規格

| 規格 | 用途 |
|------|------|
| IEC 62304:2006+A1:2015 / JIS T 2304:2017 | 本プロジェクトの直接の根拠 |
| ISO 14971:2019 | リスクマネジメント(箇条 7、RMF で参照) |
| ISO 13485:2016 | 品質マネジメントシステム(保守プロセスで連携) |
| IEC 60601-2-1 | 電子加速器型医用電気機器の安全性・基本性能(Therac-25 相当機の個別規格) |
| IEC 62366-1 | ユーザビリティエンジニアリング(操作者入力の確定・エラー表示) |
| IEC 60601-1-8 | アラームシステム(インターロック逸脱時の通知) |
| AAMI TIR45:2012 | アジャイル/インクリメンタル開発の医療機器ソフトウェアへの適用 |
| AAMI TIR57:2016 | 医療機器セキュリティのリスクマネジメント原則 |
| ISO/IEC 25010 | ソフトウェア品質モデル(信頼性・保守性の属性評価) |

## 参考文献(Therac-25 事故解析)

- N. G. Leveson, C. S. Turner. "An Investigation of the Therac-25 Accidents." IEEE Computer, 26(7):18-41, 1993.
- N. G. Leveson. *Safeware: System Safety and Computers*. Addison-Wesley, 1995.(Appendix A に Therac-25 詳細)
- FDA 21 CFR 1020.40(医用電子加速器)および関連通達

## 免責事項

- 本プロジェクトは **学習目的の仮想プロジェクト** であり、実在の医療機器ではありません。
- Therac-25 は実在した装置ですが、本リポジトリの成果物は、当時の AECL 社の公式文書ではなく、公開事故報告書・学術文献に基づく **再構成・学習用の仮想成果物** です。特定製品の規制適合や事故再評価を保証するものではありません。
- 実運用前には各組織の品質マネジメントシステム・リスク受容基準・規制要求に従った調整と、QMS/RA 責任者の正式レビューが必要です。
- 医療的判断・放射線治療計画・被曝線量評価に本ソフトウェアを使用しないでください。

## ライセンス

未設定(今後決定予定)。

## 参考: 元テンプレートの README

本プロジェクトのベースとなっているテンプレートの詳細は [grace2riku/iec62304_template](https://github.com/grace2riku/iec62304_template) を参照してください。
