#!/usr/bin/env bash
#
# Install the repository commit hook.  The hook rejects whitespace errors and
# missing Signed-off-by lines, matching the project policy.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOOK="$ROOT/.git/hooks/pre-commit"

if [ ! -d "$ROOT/.git" ]; then
    echo "not a git repository: $ROOT" >&2
    exit 1
fi

cat > "$HOOK" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

git diff --cached --check >/dev/null || {
    echo "pre-commit: whitespace errors" >&2
    exit 1
}

if ! git log -1 --pretty=%B 2>/dev/null | grep -q '^Signed-off-by: '; then
    echo "pre-commit: missing Signed-off-by" >&2
    exit 1
fi
EOF

chmod +x "$HOOK"
echo "installed $HOOK"
