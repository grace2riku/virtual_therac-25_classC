#!/usr/bin/env bash
# main ブランチに docs-check の必須ステータスチェックを設定する。
#
# 背景: CI(`.github/workflows/docs-check.yml`)が失敗していても気付かずに
# 次ステップへ進行してしまう問題を防ぐ。安全クリティカルなクラス C 開発では
# CI 失敗の放置は監査時のプロセス外進行リスクとなるため、Branch Protection
# Rule で `docs-check` ジョブを required status checks に登録し、PR の
# マージを物理的にブロックする。
#
# 設定内容:
# - required_status_checks: docs-check.yml の 4 ジョブを必須化(strict mode)
# - allow_force_pushes / allow_deletions: false
# - enforce_admins: false(緊急ホットフィックス用に管理者は除外可)
# - required_pull_request_reviews: null(ソロ開発を想定し PR レビュー必須化はしない)
#
# 前提: gh CLI が利用可能で、対象リポジトリへの admin 権限を持つこと。
#
# 使い方:
#   ./scripts/setup_branch_protection.sh                     # カレントの main
#   ./scripts/setup_branch_protection.sh -R owner/repo       # 別リポジトリ
#   ./scripts/setup_branch_protection.sh -b develop          # 別ブランチ

set -uo pipefail

REPO=""
BRANCH="main"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -R|--repo)   REPO="$2";   shift 2 ;;
    -b|--branch) BRANCH="$2"; shift 2 ;;
    -h|--help)
      sed -n '2,22p' "$0" | sed 's/^# \{0,1\}//'
      exit 0 ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

if [[ -z "$REPO" ]]; then
  REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner 2>/dev/null)" || {
    echo "[error] -R で対象リポジトリを指定するか、git リポジトリ内で実行してください" >&2
    exit 1
  }
fi

echo "Applying branch protection: $REPO@$BRANCH"

gh api -X PUT "repos/$REPO/branches/$BRANCH/protection" --input - <<'EOF'
{
  "required_status_checks": {
    "strict": true,
    "contexts": [
      "Verify IEC 62304 directory structure",
      "Markdown lint",
      "Internal link check",
      "Date format policy"
    ]
  },
  "enforce_admins": false,
  "required_pull_request_reviews": null,
  "restrictions": null,
  "allow_force_pushes": false,
  "allow_deletions": false
}
EOF

echo "Branch protection applied: $REPO@$BRANCH"
