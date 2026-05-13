# 仮想 Therac-25(Virtual Therac-25) — プロジェクト指示(Claude 向け)

## 概要

本リポジトリは **IEC 62304:2006+A1:2015「医療機器ソフトウェア ― ソフトウェアライフサイクルプロセス」** の **箇条 5(開発プロセス)、箇条 6(保守プロセス)、箇条 7(リスクマネジメントプロセス)、箇条 8(構成管理プロセス)、箇条 9(問題解決プロセス)** に基づく成果物を、歴史的医療機器 **Therac-25**(AECL、1982 年発売)を題材に仮想的に作成するためのプロジェクトである。

本ファイルは Claude(AI アシスタント)が本ディレクトリ配下で作業する際の **常時参照ルール** を記述する。テンプレートとしての汎用的記述(箇条 5〜9 の規格解説)と、本プロジェクト固有のルール(下部「本プロジェクト(仮想 Therac-25 / クラス C)固有のルール」節)が併記されている。矛盾した場合は **固有ルールが優先** する。

- **対象安全クラス: クラス C**(死亡又は重傷の可能性)
- **実装言語: C++20**
- **ファイル形式: Markdown**(Git での差分管理を前提)
- **規格発行元: IEC(国際電気標準会議)/ JIS T 2304(日本版)**
- **ベーステンプレート: [grace2riku/iec62304_template](https://github.com/grace2riku/iec62304_template)**

## 安全クラスについて

IEC 62304 の安全クラスは以下のとおり。本テンプレートは **クラス C を前提** としており、クラス A/B の要求事項もすべて包含する。

| クラス | 定義 |
|--------|------|
| A | 傷害又は健康被害はあり得ない |
| B | 重傷には至らない傷害があり得る |
| C | 死亡又は重傷があり得る |

**クラス C のみ**で追加適用される要求事項(箇条 5):
- 5.3.5 リスクコントロール手段のためのソフトウェア項目の分離
- 5.4.2 各ソフトウェアユニットの詳細設計の作成
- 5.4.3 インタフェースの詳細設計の作成
- 5.4.4 詳細設計の検証
- 5.5.4 追加のソフトウェアユニット受入基準
- 5.7.4 ソフトウェアシステム試験の妥当性確認

**クラス B 及び C** で追加適用される主な要求事項(箇条 5):
- 5.1.4 ソフトウェア開発標準・方法・ツールの計画
- 5.1.12 共通ソフトウェア欠陥の識別と回避
- 5.3.3 SOUP の機能的及び性能的要求事項の指定
- 5.3.4 SOUP に必要なシステム上の HW/SW の指定
- 5.4.1 ソフトウェアアーキテクチャのソフトウェアユニットへの改良
- 5.5.2 ソフトウェアユニット検証プロセスの確立
- 5.5.3 ソフトウェアユニット受入基準
- 5.6.x 結合および結合試験の大部分
- 5.7.x システム試験の大部分
- 5.8.x リリースの大部分

## ディレクトリ構造

```
.
├── CLAUDE.md                                              # 本ファイル
├── 5.1_software_development_planning/                     # 5.1 ソフトウェア開発計画
│   └── software_development_plan.md
├── 5.2_software_requirements_analysis/                    # 5.2 ソフトウェア要求事項分析
│   └── software_requirements_specification.md
├── 5.3_software_architecture_design/                      # 5.3 ソフトウェアアーキテクチャの設計
│   └── software_architecture_design.md
├── 5.4_software_detailed_design/                          # 5.4 ソフトウェア詳細設計
│   └── software_detailed_design.md
├── 5.5_software_unit_implementation/                      # 5.5 ソフトウェアユニットの実装
│   └── software_unit_test_plan_report.md
├── 5.6_software_integration_testing/                      # 5.6 ソフトウェア結合及び結合試験
│   └── software_integration_test_plan_report.md
├── 5.7_software_system_testing/                           # 5.7 ソフトウェアシステム試験
│   └── software_system_test_plan_report.md
├── 5.8_software_release/                                  # 5.8 ソフトウェアリリース
│   └── software_master_specification.md
├── 6_software_maintenance_process/                        # 6  ソフトウェア保守プロセス
│   └── software_maintenance_plan.md
├── 7_software_risk_management_process/                    # 7  ソフトウェアリスクマネジメントプロセス
│   ├── software_risk_management_plan.md
│   ├── software_safety_class_determination_record.md
│   └── risk_management_file.md                            # ISO 14971 RMF(機器全体)
├── 8_software_configuration_management_process/           # 8  ソフトウェア構成管理プロセス
│   ├── software_configuration_management_plan.md
│   ├── configuration_item_list.md
│   ├── ccb_operating_rules.md                             # CCB 運用規程
│   └── change_request_register.md                         # CR 台帳
├── 9_software_problem_resolution_process/                 # 9  ソフトウェア問題解決プロセス
│   └── software_problem_resolution_procedure.md
├── compliance/                                            # 監査対応
│   └── audit_checklist.md                                 # IEC 62304 条項別チェックリスト
├── .github/workflows/                                     # CI(GitHub Actions)
│   └── docs-check.yml                                     # 構造・lint・リンクチェック
├── .markdownlint-cli2.yaml                                # markdownlint 設定
└── lychee.toml                                            # リンクチェック設定
```

## ドキュメント間のトレーサビリティ

各ドキュメントには一意の ID を付与し、要求事項から設計・実装・試験まで双方向に追跡できるようにする。推奨する ID 体系:

| 種別 | プレフィックス | 例 |
|------|--------------|-----|
| ソフトウェア要求事項 | `SRS-` | SRS-001 |
| アーキテクチャ要素 | `ARCH-` | ARCH-001 |
| 詳細設計項目 | `SDD-` | SDD-001 |
| ソフトウェアユニット | `UNIT-` | UNIT-001 |
| ソフトウェア項目間 IF | `IF-U-` | IF-U-001 |
| ソフトウェア外部 IF | `IF-E-` | IF-E-001 |
| ユニット試験 | `UT-` | UT-001 |
| 結合試験 | `IT-` | IT-001 |
| システム試験 | `ST-` | ST-001 |
| リスクコントロール手段 | `RCM-` | RCM-001 |
| ハザード(システム) | `HZ-` | HZ-001 |
| SOUP | `SOUP-` | SOUP-001 |
| 分離(アーキテクチャ) | `SEP-` | SEP-001 |
| 構成アイテム | `CI-` | CI-SRC-001 |
| 問題報告 | `PRB-` | PRB-0001 |
| 変更要求 | `CR-` | CR-0001 |
| 異常(残留) | `ANOM-` | ANOM-001 |
| ベースライン | `BL-` | BL-20260417-001 |

## ドキュメント ID プレフィックス

各成果物ドキュメントにも一意の ID プレフィックスを付与する。

| 成果物 | プレフィックス |
|-------|------------|
| ソフトウェア開発計画書 | `SDP-` |
| ソフトウェア要求仕様書 | `SRS-` |
| ソフトウェアアーキテクチャ設計書 | `SAD-` |
| ソフトウェア詳細設計書 | `SDD-` |
| ユニットテスト計画書/報告書 | `UTPR-` |
| 結合試験計画書/報告書 | `ITPR-` |
| システム試験計画書/報告書 | `STPR-` |
| ソフトウェアマスタ仕様書 | `SMS-` |
| ソフトウェア保守計画書 | `SMP-` |
| ソフトウェアリスクマネジメント計画書 | `SRMP-` |
| ソフトウェア安全クラス決定記録 | `SSC-` |
| ソフトウェア構成管理計画書 | `SCMP-` |
| 構成アイテム一覧 | `CIL-` |
| CCB 運用規程 | `CCB-` |
| 変更要求台帳 | `CRR-` |
| リスクマネジメントファイル | `RMF-` |
| ソフトウェア問題解決手順書 | `SPRP-` |
| 監査チェックリスト | `ACL-` |

## 編集時のガイドライン

### 記述スタイル
- 見出しレベルは `#`(H1)をドキュメントタイトル、`##`(H2)を箇条番号に対応させる。
- プレースホルダは `{{...}}` 形式(例: `{{製品名}}`, `{{バージョン}}`)で統一する。
- 表は GitHub Flavored Markdown 形式で記述する。
- **表の列数はヘッダー・区切り行・全データ行で必ず一致させる**(列数不整合は markdownlint の MD056 で検出される)。補足情報はセル内に括弧書きで追加し、新しい列を増やさない(例: `| 要求事項(推奨) | 1 |` とする。`| 要求事項 | 推奨 | 1 |` のように列を増やさない)。
- 未記入項目は削除せず `TBD`(To Be Determined)と明記する。

### 日付の書式(必須)
- **ISO 8601 の拡張表記のみ使用**: `YYYY-MM-DD`(例: `2026-04-17`)
- 月単位の場合のみ `YYYY-MM`(例: `2026-04`)を許容する。
- 以下の表記は **使用禁止**:
  - スラッシュ区切り `YYYY/MM/DD` や欧州順 `DD/MM/YYYY`
  - `April 17, YYYY` のような英語月名による自然言語表記
  - 年を省略した `4/17` のような表記
- 規格整合性のため、全ドキュメントでこの書式を厳守する。CI(`.github/workflows/docs-check.yml`)が自動検出する。

### 英語略語の統一(ドキュメント内・ID プレフィックスとも)
- 略語は「ドキュメント ID プレフィックス」表に準拠する。
- 本文で初出の略語は **必ずフルネーム + 括弧内略語** で記述する(例: "ソフトウェア要求仕様書(SRS)")。
- 各ドキュメントの冒頭「用語と略語」セクションに使用する略語を列挙する。
- 規格用語と日本語訳の対応は以下を基準とする。

| 英語略語 | 英語フル | 日本語 |
|---------|---------|-------|
| SOUP | Software of Unknown Provenance | 素性不明のソフトウェア |
| SRS | Software Requirements Specification | ソフトウェア要求仕様書 |
| SAD | Software Architecture Description | ソフトウェアアーキテクチャ設計書 |
| SDD | Software Design Description | ソフトウェア詳細設計書 |
| SDP | Software Development Plan | ソフトウェア開発計画書 |
| SMP | Software Maintenance Plan | ソフトウェア保守計画書 |
| SRMP | Software Risk Management Plan | ソフトウェアリスクマネジメント計画書 |
| SCMP | Software Configuration Management Plan | ソフトウェア構成管理計画書 |
| SPRP | Software Problem Resolution Procedure | ソフトウェア問題解決手順書 |
| SMS | Software Master Specification | ソフトウェアマスタ仕様書 |
| RMF | Risk Management File | リスクマネジメントファイル(ISO 14971) |
| CIL | Configuration Item List | 構成アイテム一覧 |
| CRR | Change Request Register | 変更要求台帳 |
| CI | Configuration Item | 構成アイテム |
| CCB | Change Control Board | 変更管理委員会 |
| CR | Change Request | 変更要求 |
| PRB | Problem Report | 問題報告 |
| RCM | Risk Control Measure | リスクコントロール手段 |
| MC/DC | Modified Condition/Decision Coverage | 改良条件分岐網羅 |

### 改訂履歴
- 各ドキュメントの末尾に「改訂履歴」テーブルを必ず設ける。
- バージョンは `MAJOR.MINOR`(例: 1.0, 1.1, 2.0)を採用する。
- 変更内容は何が・なぜ変わったかを具体的に記述する。

### レビュー・承認
- 各ドキュメントの冒頭に「作成者 / レビュー者 / 承認者」欄を設ける。
- クラス C では設計検証・妥当性確認の記録が必須であるため、レビュー実施日・承認日を残す。

### トレーサビリティの維持
- 要求事項 → 設計 → 実装 → 試験の双方向トレーサビリティマトリクスを 5.2 と 5.7 のドキュメントに含める。
- 変更時は関連ドキュメントすべてを同一コミットで更新する(構成管理の原則)。

## 関連規格・参照

| 規格 | 内容 |
|------|------|
| ISO 14971 | 医療機器のリスクマネジメント(7 章・5.2.3 で参照) |
| ISO 13485 | 医療機器の品質マネジメントシステム(6 章市販後監視で参照) |
| IEC 82304-1 | ヘルスソフトウェア — 製品安全に関する一般要求事項 |
| IEC 62366-1 | ユーザビリティエンジニアリング(SRS 4.6 で参照) |
| IEC 60601-1-8 | アラームシステム(SRS 4.4 で参照) |
| IEC 80001-1 | IT ネットワーク適用のリスクマネジメント(SRS 4.10 で参照) |
| IEC 81001-5-1 | ヘルスソフトウェアのセキュリティ(SRS 4.5 で参照) |
| AAMI TIR57 | 医療機器セキュリティのリスクマネジメント原則 |

## CI(GitHub Actions)

Pull Request・push ごとに `.github/workflows/docs-check.yml` が以下を自動検証する。

1. **構造検証**: 箇条 5.1〜9 の必須ディレクトリ・主要テンプレートファイルの存在確認
2. **Markdown lint**: `.markdownlint-cli2.yaml` の規則に従った書式チェック
3. **内部リンクチェック**: `lychee.toml` 設定によるリンク切れ検出(オフラインモード)
4. **日付書式ポリシー**: ISO 8601 非準拠(スラッシュ区切り等)の検出

CI を失敗させずに事前確認するには、ローカルで以下を実行する。
```
npx markdownlint-cli2 "**/*.md"
lychee --offline --include-fragments './**/*.md'
```

## 本プロジェクト(仮想 Therac-25 / クラス C)固有のルール

### 題材と学習目的(最重要・判断基準)

本プロジェクトの題材は **実在した医療用リニアアクセラレータ Therac-25**(AECL、1982 年発売)である。1985 年 6 月〜1987 年 1 月に米国・カナダで **6 件の放射線過剰照射事故(6 名重症または死亡)** が発生した。

本プロジェクトの成果物は、IEC 62304 が定めるプロセスと成果物を、この歴史的事故を教材として再構成するものである。したがって以下を **設計・記述の第一原理** とする。

- **「もし IEC 62304 のプロセスが当時正しく回っていたら、どの成果物・どの活動が事故を防いだか?」** の視点で記述する。
- 事故原因として報告されている以下の項目は、SRS・SAD・SDD・RMF の各所で明示的に取り上げ、対応する **リスクコントロール手段(RCM)・ユニット試験・システム試験・検証活動** を計画する。
  - **race condition**(操作者入力処理とビーム設定のタイミング競合)
  - **整数オーバフロー/カウンタバグ**(共有変数の周期的ゼロ値による安全チェックバイパス)
  - **ハードウェアインターロック廃止によるソフトウェア単独安全設計**
  - **旧機種(Therac-6/20)コード再利用時の前提条件見直し漏れ**
  - **暗号的エラーメッセージ**("MALFUNCTION 54" 等)と操作者によるバイパス常態化
  - **ソフトウェア起因のハザード解析(FMEA / FTA)欠如**
  - **独立レビュー・独立試験の不在**
- 本リポジトリは学術・教育目的の **再構成(仮想)** であり、当時の AECL 社内文書ではない。成果物冒頭に「本プロジェクトは学習目的の仮想プロジェクトである」旨を明記する。

### 言語・実装技術の固定

- 実装言語は **C++20** に固定する(SDP、CIL、SCMP 等で表記統一)。理由: 当時の Therac-25 は PDP-11 アセンブリで実装されていたが、本プロジェクトは「現代の C++ で同等機能を IEC 62304 に従って実装する」という学習文脈である。
- コーディング規約の第一候補は **MISRA C++ 2023**、補助として **CERT C++ Secure Coding Standard**。
- 並行処理は **C++ 標準スレッドライブラリ + ThreadSanitizer** を用い、Therac-25 の race condition を再現→検出→修正する教材を構築する。

### 開発ステップ記録の更新義務(最重要)

本リポジトリでは、**開発ステップが一つ完了するごとに `DEVELOPMENT_STEPS.md` を必ず更新する** ことをプロジェクト運用ルールとする。本ルールは IEC 62304 §5.1.8(ドキュメント作成計画)の延長として、プロジェクト固有の追加要件とする(姉妹プロジェクト `virtual_infusion_pump_classC` と同一方針)。

**更新トリガ:**

- 新しいドキュメントを作成・承認した(SRS・SAD・SDD・UT 報告書 等、すべての成果物が対象)
- 既存ドキュメントを重大改訂した(バージョンアップを伴う変更)
- フェーズ遷移した(M0→M1、Inc.1→Inc.2 等)
- 計画した順序と実行した順序が乖離した
- 後続プロジェクトへの教訓が得られた(とくに Therac-25 の事故原因と本プロジェクトでの対応が対応付けられた場合)

**記録項目:**

- Step 番号、作業日(`YYYY-MM-DD`)
- 作業内容(要約)
- 成果物(ドキュメント ID、ファイルパス)
- 関連コミット(SHA-1 先頭 7 文字)
- 採用根拠(なぜこの順序・内容を選んだか。IEC 62304 条項と、可能なら Therac-25 事故との対応を含む)

**運用上の重要性:**

- 本書は後続プロジェクトが IEC 62304 の進め方を参照する **お手本** として機能する。
- 更新漏れは SPRP の対象(重大度 Minor 以上、根本原因分類「ドキュメント誤り」)として扱う。
- 成果物コミットと同一コミット(または直後のコミット)で更新するのが望ましい。

### ID 体系の本プロジェクト固有規則

本プロジェクトでは、テンプレートの ID 体系に以下の上書きを適用する。

| 従来(テンプレート) | 本プロジェクト | 理由 |
|-------------------|---------------|------|
| `PR-NNNN`(Problem Report) | **`PRB-NNNN`** | GitHub の Pull Request との ID 衝突を避けるため |

SCMP / SPRP / SMP および今後のすべてのドキュメントで `PRB-NNNN` を使用する。既存ドキュメント内の `PR-nnnn` 表記(問題報告を指すもの)は順次 `PRB-nnnn` に修正する。

なお、製品コードは **`TH25`** を用いる(例: SDP-TH25-001、SSC-TH25-001、RMF-TH25-001)。

### 単独開発下の独立性擬制

本プロジェクトは単独開発のため、CCB・品質責任者・RA 責任者・レビュアは作成者が兼任する。これは Therac-25 事故の主要因の一つである **「単独開発者+独立レビューの不在」を意図的に擬制で補う** という学習目的に直結する。独立性の代替手段:

1. 24 時間以上のインターバル(中程度以上の変更)
2. 自己レビューチェックリスト(PR テンプレートに埋め込む予定)
3. CI による機械的検証(ドキュメント lint、コード lint、試験、網羅率、脆弱性スキャン、TSan/ASan/UBSan)

これらは SCMP §4.1.1、SRMP §3.2、SPRP §5 で明文化する予定である。

### UT 作成時の二重セルフチェック (Severity マッピング + SRS 範囲内)(必須)

本節は UT 作成時に必須となる **2 つのセルフチェック** を統合的に記述する。両者とも「ローカル骨格ビルド (`build-local`、`TH25_BUILD_TESTS=OFF`) では `tests/` ディレクトリがビルドされないため CI でしか検出できない」という構造的特性を共有し、CI 失敗の **事前予防** として位置づけられる。

| セルフチェック項目 | 由来 PRB / CR | 確立 Step |
|------------------|--------------|----------|
| **A. Severity マッピング自己セルフチェック** | PRB-0007 / CR-0026 | Step 37 / 39(2026-05-10〜2026-05-13) |
| **B. SRS 範囲内セルフチェック** | PRB-0002 / PRB-0008 / CR-0029 | Step 22 / 41 / 43(2026-05-01〜2026-05-13) |

両ルールは SPRP §3.1 PRB 起票プロセスの教訓水平展開運用ルール本格運用例として、CR-0021(do-while パターン化、第 1 例)に続く第 2 例(CR-0026)・第 3 例(CR-0029)で確立された。

#### A. Severity マッピング自己セルフチェック

**背景:** Step 37 で UNIT-104 の `UT-104-08` を実装する際、UNIT-103 AlarmDisplay(Internal 系 0xFF = `Severity::Critical`)のテンプレを流用したが、`ErrorCode::AuthRequired` は Auth 系(0x07)= `Severity::Medium` であり、`static_assert(severity_of(AuthRequired) == Severity::Critical)` で compile-time 失敗。CI gcc-13 全 4 ジョブが Build フェーズで exit code 1 となった(本体 `918df6c` → 修正 `37aa742` の 2 コミット構成で解決)。

**運用ルール:**

1. **`UT-XXX-08` 等の Severity 静的表明を実装する際は、必ず以下を順に確認する:**
   - 該当 ErrorCode のカテゴリ(`0x01`〜`0xFF`)を `src/th25_ctrl/include/th25_ctrl/common_types.hpp` の定義で確認
   - SDD §6.2 `severity_of()` マッピング表(下表)で期待 Severity を再確認
   - UNIT-200 `tests/unit/test_common_types.cpp` 内の Severity 網羅試験で既に検証済みの値と一致することを確認
2. **他ユニット UT テンプレ流用時は、以下を必ず変更する:**
   - テスト名(例: `XxxIsCritical` → `XxxIsMedium`)
   - 期待 Severity 値(`Severity::Critical` → `Severity::Medium` 等)
   - コメント内の Severity 表記(「Critical (fail-stop)」→「Medium (Auth 系)」等)
3. **テンプレ流用箇所には必ず `// テンプレ流用元: UT-XXX-08 (...). Severity を SDD §6.2 で再確認済` のコメントを追加する**

**SDD §6.2 Severity マッピング表(参照用):**

| カテゴリ | 範囲 | Severity |
|---------|------|---------|
| Mode (0x01), Beam (0x02), Dose (0x03), Internal (0xFF) | 0x01xx/0x02xx/0x03xx/0xFFxx | **Critical** |
| Turntable (0x04), IPC (0x06) | 0x04xx/0x06xx | **High** |
| Magnet (0x05), Auth (0x07) | 0x05xx/0x07xx | **Medium** |
| その他 | — | Low |

#### B. SRS 範囲内セルフチェック

**背景:** Step 41 で UT-204-37 (`ConcurrentAttachDetachIsRaceFree`) を実装する際、producer (5000 pulse) で target 到達しない設計とするため `set_dose_target(DoseUnit_cGy{1.0e9}, ...)` を指定したが、SRS-008 範囲 [0.01, 10000.0] cGy を約 10 万倍超過していたため、`set_dose_target` が `ErrorCode::DoseOutOfRange` を返却し runtime fail。CI clang-tidy 以外の全 8 ジョブが UT-204-37 で失敗(本体 `0fc22b7` → 修正 `17295c6` の 2 コミット構成で解決)。同根本原因は Step 22 の PRB-0002(UT-204-30 で同 SRS-008 範囲外指定)で既発であり、同一ファイル内に修正済 UT-204-30 が存在したにもかかわらずテンプレ参照が行われなかったことが直接原因。

**運用ルール:**

1. **`set_*_target()` / `set_*_value()` 等の入力値を hard-coded する際は、必ず以下を順に確認する:**
   - 対応する SRS の範囲制約(SRS-008 等)を SRS 本体で再確認
   - SDD の対応する強い型定義(`DoseUnit_cGy` / `Energy_MeV` / `Position_mm` / `MagnetCurrent_A` 等)と SRS-D-XXX 行の範囲を併せて確認
   - 範囲内指定であること、**または**範囲外として意図的に拒否確認するケース(境界値網羅試験等)であることを機械的にセルフチェック
2. **並行 UT(producer + 多重 reader/attacher 等)を新規追加する際は、必ず以下を満たす:**
   - **同一ファイル内の既存 PRB 修正済 UT のパターン**(本件では UT-204-30)を必ず参照し、同型の根本原因再発を構造的に予防
   - 「target 到達させたい」/「target 到達させたくない」の **意図を UT コメントで明示**し、target 値選択の根拠(producer pulse 数 × rate との関係)を併記
3. **意図的範囲外指定(物理的飽和 clamp 試験 / 境界値網羅 / 拒否確認)の場合は、コメントで明示する:**
   - 例: `// SRS-D-008 範囲外 (15.0 mA、上限 10.0 mA 超) → clamp 動作確認`
   - 例: `// SRS-008 境界外 (10001.0 cGy) → DoseOutOfRange 拒否確認`
   - お手本: UT-301-19(`test_electron_gun_sim.cpp`、コメントで「SRS-D-008 範囲内」明示)/ UT-302-19(`test_bending_magnet_sim.cpp`、コメントで「SRS-D-006 範囲内」明示)

**主要な SRS 範囲制約一覧(参照用):**

| 強い型 | 範囲 | 出典 |
|-------|------|------|
| `DoseUnit_cGy` | 0.01〜10000.0 cGy(0.01 cGy ステップ) | SRS-008 / SRS-I-003 / SRS-D-004 |
| `Energy_MeV`(Electron) | 1.0〜25.0 MeV | SRS-005 等 / `common_types.hpp` |
| `Energy_MV`(XRay) | 5.0〜25.0 MV | SRS-005 等 / `common_types.hpp` |
| `Position_mm`(Turntable) | -100.0〜+100.0 mm | SRS-D-007 / `common_types.hpp` |
| `MagnetCurrent_A` | 0.0〜500.0 A | SRS-D-006 / `common_types.hpp` |
| `ElectronGunCurrent_mA` | 0.0〜10.0 mA | SRS-D-008 / `common_types.hpp` |

#### ローカル検出不可の構造的理由(A / B 共通)

`build-local`(`TH25_BUILD_TESTS=OFF`)では `tests/` ディレクトリがビルドされないため、Severity マッピング誤り(A)も SRS 範囲外 hard-coded 値による runtime 失敗(B)も **CI でしか検出できない**。よって本節 A / B 両ルールを CI 失敗の **事前予防** として位置づける。

## AI アシスタントへの指示

本ディレクトリで作業する際は:

1. **規格の箇条番号と章立てを絶対に変更しない**(監査時のトレーサビリティを損なうため)。
2. プレースホルダを埋める際は、既存の他ドキュメントとの整合性を確認する。
3. 要求事項を追加・変更した場合は、関連するトレーサビリティマトリクスも併せて更新する。
4. **クラス C 固有の要求事項(5.3.5, 5.4.2, 5.4.3, 5.4.4, 5.5.4, 5.7.4)は削除しない。**
5. 差分レビューを容易にするため、既存行を不必要に再フォーマットしない。
6. 新規テンプレート追加時は `compliance/audit_checklist.md` の対応表と、CI の `docs-check.yml` 必須ファイルリストを更新する。
7. 日付はすべて **`YYYY-MM-DD` 書式** を使用する(CI で検出される)。
8. **ドキュメントを新規作成・重大改訂した場合は、必ず同一コミットまたは直後のコミットで `DEVELOPMENT_STEPS.md` を更新する**(Step 番号の追加、採用根拠の記録)。忘れた場合は SPRP の対象となる。
9. 問題報告 ID は `PRB-NNNN` を使用する(`PR-NNNN` は使わない)。GitHub の Pull Request と区別するため。
10. 実装言語は **C++20** とする。Python や C 言語など他言語のサンプルコードは、比較・参照目的以外では提示しない。
11. Therac-25 の歴史的事故に関する記述は、学術文献(Leveson & Turner 1993 ほか)に基づく事実関係のみを記述し、AECL 社・当時の担当者個人への責任帰属を断定する表現は避ける。
