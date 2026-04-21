#!/usr/bin/env bash
# IEC 62304 テンプレート運用で参照される GitHub ラベルを冪等に作成・更新する。
#
# 背景: Issue テンプレート(.github/ISSUE_TEMPLATE/*.md)の labels frontmatter に
# 指定したラベル本体がリポジトリに存在しないと、`gh issue create --label ...` が
# エラー終了する。新規派生プロジェクト立ち上げ時の躓きを防ぐため、必須ラベルを
# 一括作成する。
#
# 前提: gh CLI が利用可能で、対象リポジトリに対して認証済みであること。
# 対象リポジトリは引数 -R で指定可能(指定しない場合はカレントディレクトリの
# git origin が対象となる)。
#
# 使い方:
#   ./scripts/setup_labels.sh                   # カレントリポジトリに作成
#   ./scripts/setup_labels.sh -R owner/repo     # 別リポジトリを指定

set -uo pipefail

# ラベル定義: "name|color|description"
labels=(
  "change-request|0366d6|Change Request (CR) per SCMP/CCB"
  "problem-report|d73a4a|Problem Report (PRB) per SPRP"
)

created=0
updated=0
failed=0

for entry in "${labels[@]}"; do
  IFS='|' read -r name color desc <<< "$entry"
  if gh label create "$name" --color "$color" --description "$desc" "$@" >/dev/null 2>&1; then
    echo "[created] $name"
    created=$((created + 1))
  elif gh label edit "$name" --color "$color" --description "$desc" "$@" >/dev/null 2>&1; then
    echo "[updated] $name"
    updated=$((updated + 1))
  else
    echo "[failed]  $name" >&2
    failed=$((failed + 1))
  fi
done

echo
echo "Summary: created=$created  updated=$updated  failed=$failed"
[[ $failed -eq 0 ]]
