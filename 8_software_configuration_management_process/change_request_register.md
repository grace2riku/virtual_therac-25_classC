# 変更要求台帳(CR Register)

**ドキュメント ID:** CRR-TH25-001
**バージョン:** 0.7
**最終更新日:** 2026-04-26
**対象製品:** 仮想 Therac-25(Virtual Therac-25) / TH25-SIM-001

| 役割 | 氏名 | 所属 | 日付 | 署名 |
|------|------|------|------|------|
| 管理者 | k-abe | — | 2026-04-26 | |
| 承認者(品質保証) | — | — | — | |

> **本プロジェクトの位置づけ(注記)**
>
> 本ドキュメントは IEC 62304 に基づく医療機器ソフトウェア開発プロセスの学習・参考実装を目的とした **仮想プロジェクト** の成果物である。本 CRR は **空台帳として初期化** されており、本 CCB-TH25-001 v0.1 施行日(2026-04-24)以降の全変更要求(CR)を記録する。
>
> Phase 0〜3(プロジェクト初期化〜支援プロセス計画〜インフラ整備期完了)で実施した活動(Step 0〜12: テンプレート複製、SSC/SDP/README/CLAUDE/DEVELOPMENT_STEPS/UPSTREAM_FEEDBACK/SRMP/SCMP/SPRP/SMP/CIL/CCB/CRR/RMF の作成および `planning-baseline` 確定)は **「プロジェクト立ち上げ活動」** として扱い、本台帳への遡及登録は行わない(代わりに `DEVELOPMENT_STEPS.md` にステップ記録として残る)。CCB-TH25-001 は Step 10(2026-04-24)で施行されたが、Step 10〜12 はその規程自身およびベースライン整備を含む連続した立ち上げ活動であり、本台帳の **本格運用は Phase 4(Inc.1)着手時から開始** する。最初の CR が `CR-0001` となる(Inc.1 着手時の最初の機能追加 CR、または Inc.1 期間中の最初の問題修正 CR が候補)。

---

## 1. 目的と適用範囲

本台帳は、IEC 62304 箇条 8.2(変更制御)および SCMP-TH25-001 §4 / CCB-TH25-001 §8 に基づき、Therac-25 Virtual Control Software(TH25-CTRL)に対するすべての変更要求(CR: Change Request)を一元的に記録・追跡する。起票からクローズまでの状態遷移を記録し、問題報告(PRB)・リスクコントロール手段(RCM)・リリースとのトレーサビリティを維持する。

**本台帳の運用開始日:** 2026-04-24(CCB-TH25-001 v0.1 施行日と同日)

## 2. 参照文書

| ID | 文書名 | バージョン |
|----|--------|----------|
| [1] | IEC 62304:2006+A1:2015 箇条 8.2 | — |
| [2] | 変更管理委員会(CCB)運用規程(CCB-TH25-001) | 0.1 |
| [3] | ソフトウェア構成管理計画書(SCMP-TH25-001) | 0.1 |
| [4] | ソフトウェア問題解決手順書(SPRP-TH25-001) | 0.1 |
| [5] | ソフトウェアリスクマネジメント計画書(SRMP-TH25-001) | 0.1 |
| [6] | リスクマネジメントファイル(RMF-TH25-001) | 0.1 |
| [7] | 構成アイテム一覧(CIL-TH25-001) | 0.9 |

## 3. 記録ルール

### 3.1 CR ID の採番

- 形式: `CR-NNNN`(4 桁連番)
- 採番元: **GitHub Issue 番号** を流用する(Issue #42 → CR-0042)
- 本ファイルは GitHub Issue からのスナップショットを以下のタイミングで更新する:
  - PR マージ後(マージ時に該当 CR エントリを追加・状態更新)
  - ベースラインタグ付与前(ベースラインに含まれる CR を確定)
  - 月次(Phase 4 以降、定期同期)

### 3.2 状態(State)の定義

| 状態 | 定義 |
|------|------|
| OPEN | 起票直後、区分判定前 |
| TRIAGED | 区分判定済み、インターバル中または審議中 |
| APPROVED | 承認済み、実装待ち・実装中 |
| REJECTED | 却下(理由を記録) |
| DEFERRED | 承認されたが実装を将来リリースに延期 |
| VERIFICATION | 実装完了、検証中(PR 作成済みだが CI Pass または試験待ち) |
| CLOSED | 検証合格、マージ・リリース済み |
| REOPENED | 一度クローズ後、問題再発で再オープン |

状態遷移図:

```
    OPEN ──→ TRIAGED ──→ APPROVED ──→ VERIFICATION ──→ CLOSED
                │            │                          │
                │            └─→ DEFERRED               └─→ REOPENED ──→ (再度 TRIAGED へ)
                │
                └─→ REJECTED
```

### 3.3 区分(Category)

SCMP §4.1 および CCB §5.1 と整合:

| 区分 | 判定基準 | 承認フロー(本プロジェクト運用) |
|------|---------|----------------------------|
| MINOR | 軽微(誤字・コメント・整形・CI 設定の非機能変更) | セルフ承認、即時 PR 可 |
| MODERATE | 中程度(非安全関連機能の追加/変更、ドキュメント章追加) | 1 分インターバル + 自己レビューチェックリスト + CI 全 Pass(学習プロジェクト特例、CCB-TH25-001 §5.4) |
| MAJOR | 重大(安全関連・RCM 実装・アーキテクチャ変更・要求変更・SOUP 追加/更新・**コンパイラ更新**・**並行処理関連変更**・リリース影響) | 上記 + SRMP §7.4 影響解析 + RMF 更新 + **SPRP §4.3.1 Therac-25 事故 5 主要因チェック**(並行処理関連時は **TSan 全 Pass 必須**、コンパイラ更新時は **Sanitizer 全種再実行 + HZ-007 リスク評価必須**) |

### 3.4 記録項目(列定義)

| 列名 | 内容 |
|------|------|
| CR ID | `CR-NNNN`(Issue 番号) |
| 起票日 | `YYYY-MM-DD` |
| 起票者 | GitHub handle |
| 区分 | MINOR / MODERATE / MAJOR |
| 状態 | 上記 §3.2 のいずれか |
| 概要 | 一文での変更内容 |
| 関連 PRB | `PRB-NNNN`(問題修正起因の場合)/ — |
| 関連 CI | `CI-XXX-NNN`(複数可)|
| 関連 RCM | `RCM-NNN`(RCM 実装・修正の場合)/ — |
| 関連ハザード | `HZ-NNN`(HZ-001〜HZ-010、SSC-TH25-001) / — |
| **Therac-25 事故主要因類型** | A〜F(SPRP §4.3.1)/ — / N/A(MAJOR 以外) |
| 実装 PR | `#NNN`(GitHub PR 番号) |
| 検証 ID | `UT-NNN` / `IT-NNN` / `ST-NNN`(複数可) |
| **Sanitizer 検証** | ASan/UBSan/TSan の Pass 確認(該当時) / N/A |
| インターバル遵守 | Yes(**1 分以上**、学習プロジェクト特例、CCB-TH25-001 §5.4)/ N/A(MINOR)/ 例外(理由を備考) |
| RMF 更新 | Yes(エントリ ID) / No / N/A |
| クローズ日 | `YYYY-MM-DD` |
| 備考 | 特記事項(ホットフィックス、bypass 使用、並行処理影響、HZ-007 評価結果等) |

## 4. CR 台帳(本体)

> **本台帳の本格運用は Phase 4(Inc.1)着手時から開始する。** Phase 0〜3 の活動(Step 0〜12)は立ち上げ活動として遡及登録対象外(`DEVELOPMENT_STEPS.md` にて記録済、`planning-baseline`(2026-04-25)で凍結)。CCB-TH25-001 v0.1 は Step 10(2026-04-24)で施行されたが、Step 10〜12 は規程自身・RMF・ベースライン整備という連続した立ち上げ活動として扱う。**Phase 4 着手の Step 13(2026-04-26)で `CR-0001` を起票し、本格運用を開始した。**

| CR ID | 起票日 | 起票者 | 区分 | 状態 | 概要 | 関連 PRB | 関連 CI | 関連 RCM | 関連 HZ | Therac-25 主要因類型 | 実装 PR | 検証 ID | Sanitizer | インターバル | RMF 更新 | クローズ日 | 備考 |
|-------|-------|-------|------|------|------|---------|---------|---------|---------|------------------|---------|---------|----------|-----------|---------|-----------|------|
| CR-0001 | 2026-04-26 | k-abe | MAJOR | CLOSED | Inc.1 SRS-TH25-001 v0.1 作成(ビーム生成・モード制御コア要求化) | — | CI-DOC-SRS / CI-DOC-CIL / CI-DOC-CRR / CI-DOC-DEVSTEPS / CI-DOC-README | RCM-001 / 005 / 008 / 017 / 018 / 019(SRS で要求化) | HZ-001(直接) / HZ-005(部分) / HZ-002 / HZ-007(構造的) | A(構造的予防) / F(構造的予防) | 直接コミット: 201b0b1(本体)/ e920cda(SHA 追記、SCMP §4.1.2 admin bypass 例外条件下) | SRS §8 検証チェックリスト 7 項目クリア(本 CR の検証は SRS 自体への適用、UT/IT/ST は後続 CR) | N/A(本 CR は要求化のみで実装なし、Sanitizer は Inc.1 コード実装 Step で実施) | Yes(1 分以上、SRS 作成所要時間で自然に充足、CCB §5.4) | 不要(本 CR は要求化のみで HZ/RCM 自体の追加・変更なし) | 2026-04-26 | GitHub Issue #1。CCB-TH25-001 v0.1 本格運用第 1 例。`inc1-requirements-frozen`(BL-20260426-001)を Step 13 完了時に付与・タグ保護適用済 |
| CR-0002 | 2026-04-26 | k-abe | MAJOR | CLOSED | Inc.1 C++20 プロジェクト骨格構築(SOUP 選定 vcpkg、CMake/GoogleTest/Sanitizer/CI) | — | CI-SOUP-001/002/003/004/005/006/007/008/009/010/013/014/015、CI-TOOL-008〜014/017、CI-CFG-009〜014、CI-SRC-001/005、CI-DOC-CIL/CRR/DEVSTEPS/README | RCM-002 / 004 / 008 / 019(検証手段の確立、TSan + clang-tidy で機械的検出可能化) | HZ-002(構造的予防) / HZ-007(構造的予防、バージョン固定) | A(構造的予防、TSan 基盤確立) / F(構造的予防、SOUP バージョン完全固定) | 直接コミット: 2bdc42d(本体)/ 550d383(CI 修正: ubuntu-24.04)/ 262acd5(CI 修正: clang-tidy)/ e6543a9(SHA 追記、Step 15 着手前事後処理) | smoke test(`tests/unit/test_smoke.cpp` 3 アサーション、ローカル AppleClang 15 でビルド検証済、CI で TSan/ASan/UBSan 各構成 Pass を確認) | TSan/ASan/UBSan 基盤確立(Step 14 cpp-build CI で 4 プリセット × 2 コンパイラ = 8 ジョブで常時実行)、本 CR では実検出なし | Yes(1 分以上、Step 14 主作業所要時間で自然に充足、CCB §5.4) | 不要(SOUP 確定で HZ/RCM 自体に変更なし。HZ-007 はバージョン固定で構造的予防、HZ-002 は TSan 基盤で検証手段確立) | 2026-04-26 | GitHub Issue #2。CCB-TH25-001 v0.1 本格運用第 2 例。`inc1-requirements-frozen` は維持(SRS 変更なし、CR-0002 は SRS 配下の実装基盤の確立) |
| CR-0003 | 2026-04-26 | k-abe | MAJOR | CLOSED | Inc.1 SAD-TH25-001 v0.1 作成(アーキテクチャ設計、§5.3.5 分離設計、SOUP §5.3.3/§5.3.4 割付) | — | CI-DOC-SAD / CI-DOC-CIL / CI-DOC-CRR / CI-DOC-DEVSTEPS / CI-DOC-README | RCM-001 / 005 / 008 / 017 / 018 / 019(SAD ARCH 要素・SEP に割付) | HZ-001(直接、ARCH-002.2/3/5/6) / HZ-002(構造的、SEP-001/SEP-003) / HZ-005(直接、ARCH-002.4) / HZ-007(構造的、SOUP §7/§8 HW/SW 要求明文化) | A(構造的予防、SEP-001/SEP-003 で race condition を不可能化) / D(構造的前提、SafetyMonitor 配置) / E(構造的予防、UI プロセス分離 SEP-001) / F(構造的予防、SOUP HW/SW 要求の明文化) | 直接コミット: 6b9367f(本体、SCMP §4.1.2 admin bypass 例外条件下) | SAD §10 検証チェックリスト 8 項目クリア(本 CR の検証は SAD 自体への適用、SDD/UT/IT/ST は Step 16+) | N/A(本 CR は設計のみで実装なし、Sanitizer は Step 17+ コード実装時に実施) | Yes(1 分以上、SAD 作成所要時間で自然に充足、CCB §5.4。Issue 起票 2026-04-26T07:25:26Z) | 不要(本 CR は要求からアーキテクチャへの割付であり、HZ/RCM 自体の追加・変更なし。SAD 作成中に新規ハザード識別なし) | 2026-04-26 | GitHub Issue #3。CCB-TH25-001 v0.1 本格運用第 3 例。`inc1-requirements-frozen` 維持(SRS 変更なし、SAD は SRS の実現方式確定)。`inc1-design-frozen` は Step 16(SDD 完了)時に付与予定 |
| CR-0004 | 2026-04-26 | k-abe | MAJOR | CLOSED | Inc.1 SDD-TH25-001 v0.1 作成 + `inc1-design-frozen` タグ付与(IEC 62304 §5.4.2/3/4 クラス C 必須) | — | CI-DOC-SDD / CI-DOC-CIL / CI-DOC-CRR / CI-DOC-DEVSTEPS / CI-DOC-README | RCM-001 / 002 / 003 / 004 / 005 / 008 / 013 / 017 / 018 / 019(SDD UNIT に詳細割付) | HZ-001(直接、UNIT-202/203/205/206) / HZ-002(構造的、UNIT-401 + UNIT-200 PulseCounter) / HZ-003(構造的、UNIT-204 + PulseCounter `std::atomic<uint64_t>`) / HZ-005(直接、UNIT-204) / HZ-007(構造的、§7 SOUP 使用箇所 UNIT 単位明示) | A(構造的予防、UNIT-401 lock-free queue + メッセージパッシング) / B(構造的排除、PulseCounter `std::atomic<uint64_t>` で 5,800 万年連続照射相当の余裕) / D(構造的前提、UNIT-207 SafetyMonitor 独立タスク配置) / E(構造的予防、ErrorCode 階層 + Severity 型レベル表現) / F(構造的予防、SOUP 使用箇所 UNIT 単位明示) | 直接コミット: addc14a(本体、SCMP §4.1.2 admin bypass 例外条件下) + Git タグ `inc1-design-frozen`(annotated tag、BL-20260426-002) | SDD §8 検証チェックリスト 7 項目クリア(本 CR の検証は SDD 自体への適用、UT/IT/ST は Step 17+) | N/A(本 CR は詳細設計のみで実装なし、Sanitizer は Step 17+ コード実装時に実施) | Yes(1 分以上、SDD 作成所要時間で自然に充足、CCB §5.4。Issue 起票 2026-04-26T11:33:00Z) | 不要(本 CR は要求・アーキテクチャからユニット詳細への展開であり、HZ/RCM 自体の追加・変更なし。SDD 作成中に新規ハザード識別なし) | 2026-04-26 | GitHub Issue #4。CCB-TH25-001 v0.1 本格運用第 4 例。**`inc1-design-frozen`(BL-20260426-002)を本 CR 完了時に確定**(本体コミット addc14a に annotated tag 付与、保護タグルール GitHub Ruleset id 15519327 が `refs/tags/inc*-design-frozen` パターンを含むことを事前確認済) |
| CR-0005 | 2026-04-26 | k-abe | MAJOR | CLOSED | Inc.1 Step 17 — UNIT-200 CommonTypes 実装 + UTPR-TH25-001 v0.1 + 基本 UT(IEC 62304 §5.5.x クラス C 必須) | — | CI-SRC-001(UNIT-200 ヘッダ実装段階)/ CI-DOC-UTPR(v0.1 初版)/ CI-CFG-010(.clang-tidy 微調整)/ CI-DOC-CIL / CI-DOC-CRR / CI-DOC-DEVSTEPS / CI-DOC-README | RCM-001(`enum class TreatmentMode` 実装)/ RCM-003(`PulseCounter` `std::atomic<uint64_t>`)/ RCM-008(強い型 6 種実装)/ RCM-019(`PulseCounter` 経由のみ atomic 共有許可) | HZ-002(構造的予防、`PulseCounter` atomic + concurrent UT) / HZ-003(構造的排除、`std::atomic<uint64_t>` + 境界値 UT)/ HZ-007(構造的予防、`is_always_lock_free` `static_assert`) | A(構造的予防、`PulseCounter` atomic + UT-200-16 で 4 スレッド × 10000 increment race-free 検証 TSan で **CI Pass 確認済**) / B(構造的排除、`std::atomic<uint64_t>` + UT-200-15 で `numeric_limits::max()` 境界値検証) / F(構造的予防、ヘッダ内 `static_assert(is_always_lock_free)` で HZ-007 機械的予防成立) | 直接コミット: a18b6cf(本体、SCMP §4.1.2 admin bypass 例外条件下)+ d6a6bc7(CI 修正: UT-200-09 の `requires` 式を concept にラップ) | UTPR §6 クラス C 追加受入基準 9 項目 + §6.1 本プロジェクト固有追加基準 3 項目 全クリア + §7 UNIT-200 試験ケース 24 件 **CI cpp-build マトリクス全 8 ジョブ Pass + clang-tidy 警告 0 で確認(run id 24959543848)** | TSan(`clang-17/tsan` + `gcc-13/tsan`)/ ASan(`*/asan`)/ UBSan(`*/ubsan`)**全 6 ジョブ Pass、UT-200-16 race detection 0**(Tyler 第 1 例 race condition 類型の機械的検証成立)| Yes(1 分以上、実装 + UT + UTPR 作成所要時間で自然に充足、CCB §5.4。Issue 起票 2026-04-26T13:18:08Z) | 不要(本 CR は SDD §4.1 計画済の RCM-001/003/008 を実装するのみ、HZ/RCM 自体の追加・変更なし) | 2026-04-26 | GitHub Issue #5。CCB-TH25-001 v0.1 本格運用第 5 例。`inc1-design-frozen` 維持(SDD 変更なし、UNIT-200 実装は SDD §4.1 と完全一致)。本 CR は本プロジェクト最初の **本格 C++ コード実装**(Step 14 のプレースホルダとは異なる) |

## 5. 安全関連 CR の抽出(MAJOR 区分)

安全関連 CR(MAJOR 区分 または RCM 影響あり)は、本節で別途追跡する。各 Inc. 完了時および各リリース前に本節をレビューし、RMF-TH25-001 との整合を確認する。

| CR ID | 関連ハザード(HZ) | 関連 RCM | Therac-25 主要因類型 | リスク再評価日 | RMF 更新反映 | 判定 |
|-------|----------------|--------|------------------|-------------|-----------|------|
| CR-0001 | HZ-001(直接) / HZ-005(部分) / HZ-002 / HZ-007(構造的) | RCM-001 / 005 / 008 / 017 / 018 / 019(SRS で要求化) | A(並行処理、構造的予防) / F(レガシー前提、構造的予防) | 2026-04-26(本 CR 起票時) | 不要(要求化のみ、HZ/RCM 自体は追加・変更なし) | 受入(本 CR は SRS で構造的 RCM-018/019 を要求化することで race condition への構造的予防を確立。SAD/SDD/コード実装で具体化を確認) |
| CR-0002 | HZ-002(構造的予防) / HZ-007(構造的予防) | RCM-002 / 004 / 008 / 019(検証手段の確立) | A(並行処理、構造的予防 / TSan 基盤確立) / F(レガシー前提、SOUP バージョン完全固定) | 2026-04-26(本 CR 起票時) | 不要(SOUP 確定で HZ/RCM 自体に変更なし) | 受入(本 CR は CI Sanitizer 基盤で TSan による race condition 検出力を物理確立。HZ-007 は vcpkg builtin-baseline `c3867e714d...` で完全固定。コンパイラ更新時は SCMP §4.1 重大区分 + SMP §7.2 で対応) |
| CR-0003 | HZ-001(直接、ARCH-002.2/3/5/6 に割付) / HZ-002(構造的、SEP-001/003) / HZ-005(直接、ARCH-002.4 に割付) / HZ-007(構造的、SOUP §7/§8 HW/SW 要求明文化) | RCM-001 / 005 / 008 / 017 / 018 / 019(SAD ARCH 要素・SEP に割付) | A(並行処理、SEP-001/003 で race condition 構造的不可能化) / D(構造的前提、SafetyMonitor 配置) / E(構造的予防、UI プロセス分離 SEP-001) / F(構造的予防、SOUP HW/SW 要求の明文化) | 2026-04-26(本 CR 起票時) | 不要(SAD 作成中に新規ハザード識別なし、HZ/RCM 自体の追加・変更なし) | 受入(本 CR は §9 SEP-001(UI 層と安全コアの OS プロセス分離)+ SEP-003(共有変数禁止 + lock-free queue メッセージパッシング)で Therac-25 Tyler 第 1 例 race condition の根本構造をアーキテクチャ時点で構造的に不可能化。SOUP 13 件の機能・性能要求 + HW/SW 要求を明文化し IEC 62304 §5.3.3/§5.3.4 クラス B/C 必須要求に対応) |
| CR-0004 | HZ-001(直接、UNIT-202/203/205/206 に詳細割付) / HZ-002(構造的、UNIT-401 + UNIT-200 PulseCounter) / HZ-003(構造的排除、UNIT-204 + PulseCounter atomic) / HZ-005(直接、UNIT-204 に詳細割付) / HZ-007(構造的、§7 SOUP 使用箇所 UNIT 単位明示) | RCM-001 / 002 / 003 / 004 / 005 / 008 / 013 / 017 / 018 / 019(SDD UNIT に詳細割付) | A(並行処理、UNIT-401 lock-free queue + メッセージパッシング) / B(整数オーバフロー、PulseCounter `std::atomic<uint64_t>` で構造的排除、5,800 万年連続照射相当の余裕) / D(構造的前提、UNIT-207 SafetyMonitor 独立タスク配置) / E(構造的予防、ErrorCode 階層 + Severity 型レベル表現) / F(構造的予防、SOUP 使用箇所 UNIT 単位明示) | 2026-04-26(本 CR 起票時) | 不要(SDD 作成中に新規ハザード識別なし、HZ/RCM 自体の追加・変更なし) | 受入(本 CR は SAD ARCH 要素を全 22 ユニット(UNIT-101〜104 / 200〜210 / 301〜304 / 401〜402)に詳細展開し、IEC 62304 §5.4.2/§5.4.3/§5.4.4 クラス C 必須を全章カバー。`inc1-design-frozen`(BL-20260426-002)で Inc.1 設計を不可逆に凍結、Step 17+ コード実装の唯一の根拠を確立) |
| CR-0005 | HZ-002(構造的予防、PulseCounter atomic + concurrent UT TSan) / HZ-003(構造的排除、`std::atomic<uint64_t>` + UT-200-15 境界値) / HZ-007(構造的予防、`static_assert(is_always_lock_free)` ビルド時 fail-stop) | RCM-001 / 003 / 008 / 019(UNIT-200 CommonTypes 実装) | A(並行処理、`PulseCounter` 設計実装 + UT-200-16 で 4 スレッド × 10000 race-free 検証 TSan 必須) / B(整数オーバフロー、`uint64_t` + UT-200-15 で `numeric_limits::max()` 境界値検証) / F(レガシー前提喪失、`static_assert(is_always_lock_free)` でコンパイラ・標準ライブラリ更新時に fail-stop) | 2026-04-26(本 CR 起票時) | 不要(SDD §4.1 計画済の RCM-001/003/008 を実装、HZ/RCM 自体の追加・変更なし) | 受入(本 CR は本プロジェクト最初の本格 C++ コード実装。UNIT-200 で全ユニット土台型を確立し、`static_assert(is_always_lock_free)` で HZ-007 構造的予防を build 時に機械的検証可能化。UT-200-16 の concurrent fetch_add は `tsan` プリセットで race-free を検証する設計) |

## 6. リリース別 CR サマリ

各リリースに含まれる CR を一覧化し、ソフトウェアマスタ仕様書(SMS-TH25-001)の「リリースバージョンの文書化」およびリリースノートに用いる。リリース前の最終確認で必須。

### リリース `v0.1.0`(未確定、`planning-baseline`(2026-04-25)とは別、Inc.1 着手前のマイルストーンとして検討中)

| CR ID | 区分 | 概要 | Therac-25 主要因類型 | ユーザ通知要否 | 規制当局通知要否 | リリースノート記載 |
|-------|------|------|------------------|-------------|----------------|----------------|
| _(エントリ無し — Phase 0〜3 までの活動は立ち上げ活動として非対象)_ | | | | | | |

### リリース `v0.2.0`(Inc.1 完了予定)

| CR ID | 区分 | 概要 | Therac-25 主要因類型 | ユーザ通知要否 | 規制当局通知要否 | リリースノート記載 |
|-------|------|------|------------------|-------------|----------------|----------------|
| CR-0001 | MAJOR | Inc.1 SRS-TH25-001 v0.1 作成(ビーム生成・モード制御コア要求化) | A(構造的予防) / F(構造的予防) | N/A(リリース外、要求化フェーズ) | N/A | Yes(SRS 確定をリリースノートで言及) |
| CR-0002 | MAJOR | Inc.1 C++20 プロジェクト骨格構築(SOUP 選定 vcpkg、CMake/GoogleTest/Sanitizer/CI) | A(構造的予防、TSan 基盤確立) / F(構造的予防、SOUP バージョン完全固定) | N/A(リリース外、骨格構築フェーズ) | N/A | Yes(SOUP 確定とビルド/CI 基盤確立をリリースノートで言及) |
| CR-0003 | MAJOR | Inc.1 SAD-TH25-001 v0.1 作成(アーキテクチャ設計、§5.3.5 分離設計、SOUP §5.3.3/§5.3.4 割付) | A(SEP-001/003 で race condition 構造的不可能化) / D(SafetyMonitor 配置) / E(UI プロセス分離) / F(SOUP HW/SW 要求明文化) | N/A(リリース外、設計フェーズ) | N/A | Yes(SAD 確定とアーキテクチャ分離設計をリリースノートで言及) |
| CR-0004 | MAJOR | Inc.1 SDD-TH25-001 v0.1 作成 + `inc1-design-frozen` タグ付与(IEC 62304 §5.4.2/3/4 クラス C 必須) | A(UNIT-401 lock-free queue) / B(PulseCounter `std::atomic<uint64_t>` で整数オーバフロー構造的排除) / D(SafetyMonitor 独立タスク) / E(ErrorCode 階層 + Severity) / F(SOUP UNIT 単位明示) | N/A(リリース外、詳細設計フェーズ) | N/A | Yes(SDD 確定と `inc1-design-frozen` ベースライン凍結をリリースノートで言及) |
| CR-0005 | MAJOR | Inc.1 Step 17 — UNIT-200 CommonTypes 実装 + UTPR-TH25-001 v0.1 + 基本 UT(IEC 62304 §5.5.x クラス C 必須) | A(`PulseCounter` 実装 + concurrent UT TSan) / B(`uint64_t` 境界値 UT) / F(`static_assert(is_always_lock_free)` ビルド時 fail-stop) | N/A(リリース外、本格実装初版) | N/A | Yes(本プロジェクト最初の本格 C++ コード実装、UNIT-200 全ユニット土台型確立をリリースノートで言及) |

> 今後追加予定のリリース: `v0.1.0`(Phase 3 基盤整備完了) / `v0.2.0`(Inc.1 ビーム生成・モード制御コア完了) / `v0.3.0`(Inc.2 インターロック/安全監視完了) / `v0.4.0`(Inc.3 タスク同期・タイミング保証完了) / `v0.5.0`(Inc.4 操作者 UI・エラー報告完了) / `v1.0.0`(初回正式リリース)

## 7. 集計・傾向分析(SPRP §8 連携)

各 Inc. 完了時に以下を集計する。結果は SPRP の傾向分析と併読する。

| 期間 | 総 CR | MAJOR | MODERATE | MINOR | 安全関連 | 再発(REOPENED) | 平均リードタイム |
|------|------|-------|----------|-------|--------|----------------|---------------|
| Phase 2 完了時(2026-04-24、CCB 施行直後) | 0 | 0 | 0 | 0 | 0 | 0 | — |
| Phase 3 完了時(2026-04-25、`planning-baseline` 確定時) | 0 | 0 | 0 | 0 | 0 | 0 | — |
| Step 13 完了時(2026-04-26、`inc1-requirements-frozen` 確定時) | 1 | 1 | 0 | 0 | 1 | 0 | (CR-0001 は同日内クローズのため平均リードタイム < 1 日) |
| Step 14 完了時(2026-04-26、Inc.1 プロジェクト骨格完成) | 2 | 2 | 0 | 0 | 2 | 0 | (累積。CR-0001 / CR-0002 ともに同日内クローズのため平均リードタイム < 1 日) |
| Step 15 完了時(2026-04-26、Inc.1 SAD 完成) | 3 | 3 | 0 | 0 | 3 | 0 | (累積。CR-0001/CR-0002/CR-0003 ともに同日内クローズのため平均リードタイム < 1 日) |
| Step 16 完了時(2026-04-26、Inc.1 SDD 完成 + `inc1-design-frozen` 確定) | 4 | 4 | 0 | 0 | 4 | 0 | (累積。CR-0001〜0004 ともに同日内クローズのため平均リードタイム < 1 日) |
| Step 17 完了時(2026-04-26、Inc.1 UNIT-200 CommonTypes 実装 + UTPR v0.1) | 5 | 5 | 0 | 0 | 5 | 0 | (累積。CR-0001〜0005 ともに同日内クローズのため平均リードタイム < 1 日。本格 C++ コード実装の最初の CR) |
| Inc.1 完了時 | _(未実施)_ | | | | | | |
| Inc.2 完了時 | _(未実施)_ | | | | | | |
| Inc.3 完了時 | _(未実施)_ | | | | | | |
| Inc.4 完了時 | _(未実施)_ | | | | | | |

### 7.1 Therac-25 事故主要因類型別 CR 件数(本プロジェクト固有)

SPRP §8.2 の傾向分析と整合。**3 件以上発生した類型は SRMP §4.2 の対応カテゴリ再評価と RCM 強化を必須化**(SPRP §8.2 / SMP §4.8 と整合)。

| 期間 | A: 並行処理 | B: 整数オーバフロー | C: 共有状態 | D: インターロック欠落 | E: バイパス UI | F: レガシー前提喪失 |
|------|-----------|--------------|----------|----------------|-------------|---------------|
| Phase 2 完了時 | 0 | 0 | 0 | 0 | 0 | 0 |
| Phase 3 完了時 | 0 | 0 | 0 | 0 | 0 | 0 |
| Step 13 完了時 | 1(構造的予防、CR-0001 で RCM-018/019 要求化) | 0 | 0 | 0 | 0 | 1(構造的予防、CR-0001 で RCM-018 要求化) |
| Step 14 完了時 | 2(構造的予防、CR-0002 で TSan 基盤確立、累積) | 0 | 0 | 0 | 0 | 2(構造的予防、CR-0002 で SOUP バージョン完全固定、累積) |
| Step 15 完了時 | 3(構造的予防、CR-0003 で SEP-001/003 によるアーキテクチャ時点での race condition 不可能化を確定、累積) | 0 | 0 | 1(構造的前提、CR-0003 で SafetyMonitor を独立タスクとして配置、Inc.2 への基盤、累積) | 1(構造的予防、CR-0003 で UI プロセス分離 SEP-001 によりバイパス UI を構造的に阻止、Inc.4 で完成、累積) | 3(構造的予防、CR-0003 で SOUP §7/§8 HW/SW 要求の明文化により HZ-007 評価対象を明示、累積) |
| Step 16 完了時 | 4(構造的予防、CR-0004 で UNIT-401 lock-free queue + メッセージパッシング設計を UNIT 単位で確定、累積) | 1(**新規**: 構造的排除、CR-0004 で UNIT-204 PulseCounter `std::atomic<uint64_t>` により Yakima Class3 オーバフロー類型を 5,800 万年連続照射相当の余裕で構造的に排除) | 0 | 2(構造的前提、CR-0004 で UNIT-207 SafetyMonitor を独立タスク配置として詳細設計、累積) | 2(構造的予防、CR-0004 で `ErrorCode` 階層 + `Severity` enum で型レベルにバイパス可否を表現、累積) | 4(構造的予防、CR-0004 で SOUP 使用箇所を UNIT 単位で明示、HZ-007 影響度を表化、累積) |
| Step 17 完了時 | 5(構造的予防、CR-0005 で `PulseCounter` を実装し UT-200-16 で 4 スレッド × 10000 increment race-free を TSan 検証する設計、累積) | 2(構造的排除、CR-0005 で UT-200-15 が `uint64_t::max()` 境界値検証を実装、累積) | 0 | 2 | 2 | 5(構造的予防、CR-0005 で `static_assert(is_always_lock_free)` をヘッダに埋込、コンパイラ・標準ライブラリ更新時にビルド時 fail-stop、累積) |
| Inc.1 完了時 | _(未実施)_ | | | | | |
| Inc.2 完了時 | _(未実施)_ | | | | | |
| Inc.3 完了時 | _(未実施)_ | | | | | |
| Inc.4 完了時 | _(未実施)_ | | | | | |

### 7.2 Sanitizer 関連 CR 件数(本プロジェクト固有)

| 期間 | TSan 関連 | ASan 関連 | UBSan 関連 | コンパイラ更新(HZ-007)| TSan 全 Pass 遵守率 |
|------|---------|---------|----------|--------------------|------------------|
| Phase 2 完了時 | 0 | 0 | 0 | 0 | N/A |
| Phase 3 完了時 | 0 | 0 | 0 | 0 | N/A |
| Step 13 完了時 | 0(要求化フェーズ、コード未実装) | 0 | 0 | 0 | N/A(コード実装は Step 17+ 以降) |
| Step 14 完了時 | 0(基盤整備、コード本体は Step 17+ で実装。smoke test は TSan/ASan/UBSan で実行され全 Pass を期待) | 0 | 0 | 0 | 100%(smoke test 1 件 / 1 件、CI cpp-build で確認後に確定) |
| Step 15 完了時 | 0(設計フェーズ、コード本体未変更。smoke test は TSan/ASan/UBSan で全 Pass を維持) | 0 | 0 | 0 | 100%(smoke test 1 件 / 1 件、累積維持) |
| Step 16 完了時 | 0(詳細設計フェーズ、コード本体未変更。smoke test は TSan/ASan/UBSan で全 Pass を維持) | 0 | 0 | 0 | 100%(smoke test 1 件 / 1 件、累積維持) |
| Step 17 完了時 | **0(CI cpp-build マトリクス全 8 ジョブ Pass 確認済、特に `clang-17/tsan`・`gcc-13/tsan` で UT-200-16 race-free 検証成立、Tyler 第 1 例 race condition 類型の機械的検証手段成立)** | 0 | 0 | 0 | **100%(全 8 build ジョブ Pass + clang-tidy 警告 0、run id 24959543848、2026-04-26)** |
| Inc.1 完了時 | _(未実施)_ | | | | |

**傾向分析結果のフィードバック先:**

- SDP §6 開発標準(C++ コーディング規約、clang-tidy ルール)
- SDP §14 共通ソフトウェア欠陥一覧(新規類型)
- SRMP §4.2 潜在原因カテゴリ
- 試験計画(追加試験ケース、網羅率目標、Sanitizer 試験範囲)
- SOUP 選定方針(コンパイラ・標準ライブラリ実装)
- **SPRP §4.3.1 Therac-25 事故主要因チェックリスト(新類型の追加検討)**

## 8. 台帳の保管と監査

- **保管形態:** 本 Markdown ファイル(Git 管理)+ GitHub Issues(原データ)
- **バックアップ:** GitHub リポジトリ自体がバックアップ。定期ミラーリングは SCMP §5.3 参照
- **保管期間:** 実製品展開時は医療機器寿命 + 規定期間。本プロジェクトは学習アーカイブとして無期限保持
- **監査対応:** 本台帳と GitHub Issues(`change-request` ラベル)のエントリ数が一致すること、および各 CR について状態遷移の記録(Issue コメント・PR マージ)が残っていることを定期確認
- **Therac-25 学習目的の追加監査項目:** Therac-25 事故主要因類型(SPRP §4.3.1 A〜F)該当の MAJOR CR について、適用したチェックリストの実施記録(Issue コメント)が残っていることを四半期で確認(CCB-TH25-001 §9 と連動)

## 9. 改訂履歴

| バージョン | 日付 | 変更内容 | 変更者 |
|----------|------|---------|--------|
| 0.1 | 2026-04-24 | 初版作成(空台帳として初期化、CCB-TH25-001 v0.1 施行日 2026-04-24 と同時運用開始)。CR ID 採番規則・状態定義 8 種・区分 3 種・記録項目 18 列の枠組みを整備。**Therac-25 学習目的の追加要件として、(a) §3.4 記録項目に Therac-25 事故主要因類型列(A〜F)・Sanitizer 検証列を追加、(b) §3.3 MAJOR 区分に並行処理関連変更・コンパイラ更新を独立分類、(c) §5 安全関連 CR 抽出表に Therac-25 主要因類型列を追加、(d) §6 リリース別 CR サマリに Therac-25 主要因類型列を追加、(e) §7.1 Therac-25 事故主要因類型別 CR 件数の集計表(A〜F、SPRP §8.2 / SMP §4.8 と整合)を独立節として新設、(f) §7.2 Sanitizer 関連 CR 件数集計表(TSan/ASan/UBSan/コンパイラ更新 HZ-007/TSan Pass 遵守率)を独立節として新設、(g) §8 監査項目に Therac-25 事故主要因チェック実施記録の四半期確認を追加** | k-abe |
| 0.2 | 2026-04-25 | Step 12(`planning-baseline` 確定)に伴う整合化。**(1) §2 参照文書:** [6] RMF を「※作成予定」→ v0.1 に更新(Step 11 で RMF v0.1 作成済み、Step 11 時点で CRR への追随漏れを修正)、[7] CIL を v0.1 → v0.4 に更新。**(2) §7 集計表:** `Phase 3 完了時(2026-04-25、planning-baseline 確定時)` の行を追加(総 CR 0、MAJOR/MODERATE/MINOR/安全関連/REOPENED すべて 0、CCB 施行以降も Phase 3 は立ち上げ活動の延長としてゼロ)。**(3) §7.1 Therac-25 事故主要因類型別集計:** `Phase 3 完了時` 行を追加(A〜F すべて 0)。**(4) §7.2 Sanitizer 関連集計:** `Phase 3 完了時` 行を追加(C++ 実装未着手のため N/A)。**(5) 自己参照:** 冒頭バージョン v0.2 + 最終更新日 2026-04-25 に整合更新。**(6) 本台帳の本体 §4 はエントリゼロを維持:** Phase 0〜3 の全活動(Step 0〜12)は `DEVELOPMENT_STEPS.md` に記録された立ち上げ活動であり、CCB 施行直後の継続活動として遡及登録しない方針を維持(姉妹プロジェクト VIP の CRR-VIP-001 運用に整合) | k-abe |
| 0.3 | 2026-04-26 | **Step 13(CR-0001 起票・本格運用開始)に伴う本格運用開始版。** **(1) §2 参照文書:** [7] CIL を v0.4 → v0.5 に更新。**(2) §4 本体表に CR-0001 を最初のエントリとして登録**: 起票日 2026-04-26、起票者 k-abe、区分 MAJOR、状態 VERIFICATION、概要「Inc.1 SRS-TH25-001 v0.1 作成(ビーム生成・モード制御コア要求化)」、関連 CI(SRS/CIL/CRR/DEVSTEPS/README)、関連 RCM(001/005/008/017/018/019)、関連 HZ(HZ-001 直接 / HZ-005 部分 / HZ-002・HZ-007 構造的)、Therac-25 主要因類型(A 構造的予防 / F 構造的予防)、インターバル Yes(1 分以上充足)、RMF 更新不要、備考(GitHub Issue #1、CCB 本格運用第 1 例、`inc1-requirements-frozen` 付与予定)。**(3) §5 安全関連 CR 抽出表に CR-0001 を追加**: HZ・RCM・主要因類型・リスク再評価日・RMF 更新反映・判定(受入)を記録。**(4) §6 リリース別 CR サマリ:** `v0.2.0`(Inc.1 完了予定)に CR-0001 を仮登録(リリースノート記載 Yes)。**(5) §7 集計表:** `Step 13 完了時(2026-04-26、inc1-requirements-frozen 確定時)` 行を追加(総 CR 1、MAJOR 1、安全関連 1)。**(6) §7.1 Therac-25 主要因類型別:** `Step 13 完了時` 行を追加(A: 1 構造的予防、F: 1 構造的予防)。**(7) §7.2 Sanitizer 関連:** `Step 13 完了時` 行を追加(要求化フェーズ・コード未実装のため全 0)。**(8) §4 注記更新:** 「Phase 4 着手時から本格運用開始」を「Step 13(2026-04-26)で CR-0001 起票し本格運用を開始」に確定文に変更。**(9) 自己参照:** 冒頭バージョン v0.3 + 最終更新日 2026-04-26 に整合更新。**(事後記録 2026-04-26 / Step 14 着手前): §4 本体表で CR-0001 を VERIFICATION → CLOSED に遷移、実装 PR 列に直接コミット 201b0b1 / e920cda、検証 ID 列に「SRS §8 検証チェックリスト 7 項目クリア」、クローズ日 2026-04-26 を記入。本事後記録は SCMP §4.1.2 admin bypass 例外条件下の軽微な事後記録で、CRR 自体のバージョンは v0.3 のまま** | k-abe |
| 0.4 | 2026-04-26 | Step 14(CR-0002 起票・Inc.1 C++20 プロジェクト骨格構築)に伴う整合化。**(1) §2 参照文書:** [7] CIL を v0.5 → v0.6 に更新。**(2) §4 本体表に CR-0002 を 2 件目のエントリとして登録**: 起票日 2026-04-26、起票者 k-abe、区分 MAJOR、状態 VERIFICATION、概要「Inc.1 C++20 プロジェクト骨格構築(SOUP 選定 vcpkg、CMake/GoogleTest/Sanitizer/CI)」、関連 CI(SOUP-001〜010/013〜015、TOOL-008〜014/017、CFG-009〜014、SRC-001/005、DOC-CIL/CRR/DEVSTEPS/README)、関連 RCM(002/004/008/019、検証手段の確立)、関連 HZ(HZ-002 構造的予防 / HZ-007 構造的予防)、Therac-25 主要因類型(A 構造的予防 / F 構造的予防)、インターバル Yes、Sanitizer 列「TSan/ASan/UBSan 基盤確立(本 CR では実検出なし)」、備考(GitHub Issue #2、CCB 本格運用第 2 例、`inc1-requirements-frozen` 維持)。**(3) §5 安全関連 CR 抽出表に CR-0002 を追加**: HZ-002/HZ-007 構造的予防、RCM-002/004/008/019 検証手段、判定「受入(TSan 基盤で race condition 検出力を物理確立、HZ-007 は vcpkg builtin-baseline 完全固定)」。**(4) §6 リリース別 CR サマリ:** `v0.2.0` に CR-0002 を仮登録(リリースノート記載 Yes、SOUP/ビルド/CI 基盤確立を言及)。**(5) §7 集計表:** `Step 14 完了時(2026-04-26、Inc.1 プロジェクト骨格完成)` 行を追加(累積: 総 CR 2、MAJOR 2、安全関連 2)。**(6) §7.1 Therac-25 主要因類型別:** `Step 14 完了時` 行を追加(A: 2 累積、F: 2 累積、いずれも構造的予防)。**(7) §7.2 Sanitizer 関連:** `Step 14 完了時` 行を追加(基盤整備で実検出 0、TSan 全 Pass 遵守率は smoke test 1/1 で 100% 期待値、CI cpp-build 確認後に確定)。**(8) 自己参照:** 冒頭バージョン v0.4 + 最終更新日 2026-04-26 に整合更新 | k-abe |
| 0.5 | 2026-04-26 | Step 15(CR-0003 起票・Inc.1 SAD-TH25-001 v0.1 作成)に伴う整合化。**(1) §2 参照文書:** [7] CIL を v0.6 → v0.7 に更新。**(2) §4 本体表に CR-0003 を 3 件目のエントリとして登録**: 起票日 2026-04-26、起票者 k-abe、区分 MAJOR、状態 VERIFICATION、概要「Inc.1 SAD-TH25-001 v0.1 作成(アーキテクチャ設計、§5.3.5 分離設計、SOUP §5.3.3/§5.3.4 割付)」、関連 CI(SAD/CIL/CRR/DEVSTEPS/README)、関連 RCM(001/005/008/017/018/019、SAD ARCH/SEP に割付)、関連 HZ(HZ-001 直接 / HZ-002 構造的 / HZ-005 直接 / HZ-007 構造的)、Therac-25 主要因類型(A 構造的予防 / D 構造的前提 / E 構造的予防 / F 構造的予防)、インターバル Yes(Issue 起票 2026-04-26T07:25:26Z、SAD 作成所要時間で自然充足)、Sanitizer 列「N/A(本 CR は設計のみで実装なし)」、備考(GitHub Issue #3、CCB 本格運用第 3 例、`inc1-requirements-frozen` 維持、`inc1-design-frozen` は Step 16 SDD 完了時に付与予定)。**(3) §4 本体表で CR-0002 を VERIFICATION → CLOSED に遷移**: Step 14 完了で CI 全 Pass を確認済み、実装 PR 列に本体・CI 修正・SHA 追記コミットを追記、クローズ日 2026-04-26 を記入。**(4) §5 安全関連 CR 抽出表に CR-0003 を追加**: HZ-001/002/005/007、RCM 6 件、判定「受入(SEP-001/003 で race condition 構造的不可能化、SOUP HW/SW 要求明文化で HZ-007 評価対象明示)」。**(5) §6 リリース別 CR サマリ:** `v0.2.0` に CR-0003 を仮登録(リリースノート記載 Yes、SAD アーキテクチャ分離設計を言及)。**(6) §7 集計表:** `Step 15 完了時(2026-04-26、Inc.1 SAD 完成)` 行を追加(累積: 総 CR 3、MAJOR 3、安全関連 3)。**(7) §7.1 Therac-25 主要因類型別:** `Step 15 完了時` 行を追加(A: 3 累積、D: 1 累積、E: 1 累積、F: 3 累積、いずれも構造的予防または構造的前提)。**(8) §7.2 Sanitizer 関連:** `Step 15 完了時` 行を追加(設計フェーズで実検出 0、TSan Pass 遵守率 100% 維持)。**(9) 自己参照:** 冒頭バージョン v0.5 + 最終更新日 2026-04-26 に整合更新 | k-abe |
| 0.6 | 2026-04-26 | Step 16(CR-0004 起票・Inc.1 SDD-TH25-001 v0.1 作成 + `inc1-design-frozen` 確定)に伴う整合化。**(1) §2 参照文書:** [7] CIL を v0.7 → v0.8 に更新。**(2) §4 本体表に CR-0004 を 4 件目のエントリとして登録**: 起票日 2026-04-26、起票者 k-abe、区分 MAJOR、状態 VERIFICATION、概要「Inc.1 SDD-TH25-001 v0.1 作成 + `inc1-design-frozen` タグ付与(IEC 62304 §5.4.2/3/4 クラス C 必須)」、関連 CI(SDD/CIL/CRR/DEVSTEPS/README)、関連 RCM(10 件、SDD UNIT に詳細割付)、関連 HZ(HZ-001 直接 / HZ-002/003 構造的 / HZ-005 直接 / HZ-007 構造的)、Therac-25 主要因類型(A 構造的予防 / **B 構造的排除** / D 構造的前提 / E 構造的予防 / F 構造的予防)、インターバル Yes(Issue 起票 2026-04-26T11:33:00Z、SDD 作成所要時間で自然充足)、Sanitizer 列「N/A(本 CR は詳細設計のみで実装なし)」、備考(GitHub Issue #4、CCB 本格運用第 4 例、`inc1-design-frozen`(BL-20260426-002)を本 CR 完了時に確定)。**(3) §5 安全関連 CR 抽出表に CR-0004 を追加**: HZ-001/002/003/005/007、RCM 10 件、判定「受入(SAD ARCH 要素を全 22 ユニットに詳細展開、IEC 62304 §5.4.2/3/4 クラス C 必須を全章カバー、`inc1-design-frozen` で Inc.1 設計を不可逆に凍結)」。**(4) §6 リリース別 CR サマリ:** `v0.2.0` に CR-0004 を仮登録(リリースノート記載 Yes、SDD 確定とベースライン凍結を言及)。**(5) §7 集計表:** `Step 16 完了時(2026-04-26、Inc.1 SDD 完成 + inc1-design-frozen 確定)` 行を追加(累積: 総 CR 4、MAJOR 4、安全関連 4)。**(6) §7.1 Therac-25 主要因類型別:** `Step 16 完了時` 行を追加(A: 4 / **B: 1 新規**(構造的排除) / D: 2 / E: 2 / F: 4 累積)。**(7) §7.2 Sanitizer 関連:** `Step 16 完了時` 行を追加(詳細設計フェーズで実検出 0、TSan Pass 遵守率 100% 維持)。**(8) 自己参照:** 冒頭バージョン v0.6 + 最終更新日 2026-04-26 に整合更新 | k-abe |
| 0.7 | 2026-04-26 | Step 17(CR-0005 起票・Inc.1 UNIT-200 CommonTypes 実装 + UTPR-TH25-001 v0.1 + 基本 UT)に伴う整合化。**(1) §2 参照文書:** [7] CIL を v0.8 → v0.9 に更新。**(2) §4 本体表に CR-0005 を 5 件目のエントリとして登録**: 起票日 2026-04-26、起票者 k-abe、区分 MAJOR、状態 VERIFICATION、概要「Inc.1 Step 17 — UNIT-200 CommonTypes 実装 + UTPR-TH25-001 v0.1 + 基本 UT(IEC 62304 §5.5.x クラス C 必須)」、関連 CI(SRC-001 UNIT-200 ヘッダ実装段階 / DOC-UTPR v0.1 / CFG-010 微調整 / DOC-CIL/CRR/DEVSTEPS/README)、関連 RCM(001/003/008/019)、関連 HZ(HZ-002/003/007 構造的)、Therac-25 主要因類型(A 並行処理 + UT TSan / B 整数オーバフロー + UT 境界値 / F `static_assert(is_always_lock_free)` ビルド時 fail-stop)、インターバル Yes(Issue 起票 2026-04-26T13:18:08Z、実装+UT+UTPR 作成所要時間で自然充足)、Sanitizer 列「TSan/ASan/UBSan は CI cpp-build マトリクス全 8 ジョブで実行(本 CR では UT-200-16 が tsan プリセットで race-free を検証する設計、実検出 0 を期待)」、備考(GitHub Issue #5、CCB 本格運用第 5 例、本プロジェクト最初の本格 C++ コード実装)。**(3) §5 安全関連 CR 抽出表に CR-0005 を追加**: HZ-002/003/007、RCM 4 件、判定「受入(本 CR は本プロジェクト最初の本格 C++ コード実装。UNIT-200 で全ユニット土台型を確立、`static_assert(is_always_lock_free)` で HZ-007 構造的予防を build 時に機械的検証可能化)」。**(4) §6 リリース別 CR サマリ:** `v0.2.0` に CR-0005 を仮登録(リリースノート記載 Yes、本格 C++ コード実装初版を言及)。**(5) §7 集計表:** `Step 17 完了時(2026-04-26、Inc.1 UNIT-200 CommonTypes 実装 + UTPR v0.1)` 行を追加(累積: 総 CR 5、MAJOR 5、安全関連 5)。**(6) §7.1 Therac-25 主要因類型別:** `Step 17 完了時` 行を追加(A: 5 / B: 2 / D: 2 / E: 2 / F: 5 累積)。**(7) §7.2 Sanitizer 関連:** `Step 17 完了時` 行を追加(本格コード実装初版で UT-200-16 を `tsan` プリセットで実行設計、SHA 追記事後処理時に CI 実績確定)。**(8) 自己参照:** 冒頭バージョン v0.7 + 最終更新日 2026-04-26 に整合更新 | k-abe |
