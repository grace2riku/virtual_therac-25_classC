# iec62304_template への修正要望

**ドキュメント ID:** UF-TH25-001
**作成日:** 2026-04-21
**対象リポジトリ:** [grace2riku/iec62304_template](https://github.com/grace2riku/iec62304_template)
**位置づけ:** 本プロジェクト(仮想 Therac-25 / クラス C)でテンプレートを運用する中で発見した改善提案を記録し、upstream テンプレートへ随時フィードバックするための台帳。

---

## 本書の目的

本プロジェクトは [grace2riku/iec62304_template](https://github.com/grace2riku/iec62304_template) をベースに作成された。テンプレートを実プロジェクトで運用する過程で発見した「テンプレート側で改善すべき点」を本書に蓄積し、upstream に Issue や Pull Request として還元する。

姉妹プロジェクト [grace2riku/virtual_infusion_pump_classC](https://github.com/grace2riku/virtual_infusion_pump_classC) で同じ運用を実施しており、当該プロジェクトの `UPSTREAM_FEEDBACK.md`(UF-VIP-001)は独立台帳として並行運用される。両プロジェクト双方から提起された要望が重複する場合は、**先に提起済みのエントリに本プロジェクト側の発見経緯を追記する** 方針で重複を避ける。

本書は監査成果物ではなく **プロジェクト運用補助文書** であるが、構成アイテム(CI-DOC-UPSTREAM)として CIL に登録し、Git 管理下に置く。

## 記録ルール

### エントリ ID 形式

`UF-NNN`(Upstream Feedback、3 桁連番)。NNN は本書内でのローカル連番であり、GitHub Issue 番号とは独立。upstream に Issue 起票した際は「upstream Issue」欄に Issue 番号を記録する。

### 記録項目

| 項目 | 内容 |
|------|------|
| ID | `UF-NNN` |
| 発見日 | `YYYY-MM-DD` |
| 発見経緯 | 本プロジェクトのどの Step / 作業で発見したか |
| 現状(upstream 側) | テンプレートでの現状の問題点 |
| 提案 | 具体的な修正提案 |
| 優先度 | High(安全/整合性問題)/ Medium(運用効率)/ Low(改善提案) |
| 状態 | 未提案 / 提案済み(Issue #NNN) / 受領済み(PR #NNN マージ) / 却下 |
| upstream Issue | GitHub Issue URL(提案後に記入) |
| 関連 DEVELOPMENT_STEPS | 本リポジトリ内の関連 Step |

### 更新タイミング

- 新しい修正要望を発見した際に即時追記
- upstream に提案した際に「状態」欄を更新
- upstream で受領/却下された際に「状態」欄を更新

---

## 修正要望一覧

*(現時点で要望は未登録。Inc.1 以降の運用で発見したものから順次追記予定。)*

<!--
記録テンプレート:

### UF-NNN — 要望タイトル

| 項目 | 内容 |
|------|------|
| 発見日 | YYYY-MM-DD |
| 発見経緯 | Step N(作業内容) |
| 現状(upstream 側) | … |
| 提案 | … |
| 優先度 | High / Medium / Low |
| 状態 | 未提案 / 提案済み / 受領済み / 却下 |
| upstream Issue | <https://github.com/grace2riku/iec62304_template/issues/NNN> |
| 関連 DEVELOPMENT_STEPS | Step N |
-->

## 改訂履歴

| バージョン | 日付 | 変更内容 | 変更者 |
|----------|------|---------|--------|
| 0.1 | 2026-04-21 | 初版作成(空台帳) | k-abe |
