#!/usr/bin/env bash
set -euo pipefail

OUTPUT="CHANGELOG.md"

# 检查 conventional-changelog
if ! command -v conventional-changelog >/dev/null 2>&1; then
    echo "❌ 请先安装 conventional-changelog-cli:"
    echo "   npm install -g conventional-changelog-cli"
    exit 1
fi

# 临时文件存放新生成内容
TMP_NEW=$(mktemp)

# 生成最新提交的 changelog (Angular 风格)
# -r 0 表示从初始 commit 开始生成，可修改为只生成未发布部分
conventional-changelog -p angular -r 0 > "$TMP_NEW"

# 如果 CHANGELOG.md 存在，则保留原有 # CHANGELOG 标题
if [ -f "$OUTPUT" ]; then
    # 读取原文件的 # CHANGELOG 行
    HEADER=$(grep -m 1 '^# CHANGELOG' "$OUTPUT") || HEADER="# CHANGELOG"
    
    # 保存 # CHANGELOG 标题后的内容（不包含标题本身）
    TAIL=$(tail -n +2 "$OUTPUT" || true)
    
    # 写回文件：标题 + 新增 changelog + 原有内容
    {
        echo "$HEADER"
        echo
        cat "$TMP_NEW"
        echo
        echo "$TAIL"
    } > "$OUTPUT"
else
    # 文件不存在，直接写入
    {
        echo "# CHANGELOG"
        echo
        cat "$TMP_NEW"
    } > "$OUTPUT"
fi

# 清理临时文件
rm -f "$TMP_NEW"

echo "✔ CHANGELOG 已更新: $OUTPUT"
