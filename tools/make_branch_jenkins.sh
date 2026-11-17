#!/usr/bin/env bash
#
# Create a release branch and a source archive.  Intended for CI, not for
# interactive use.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
branch="${1:?usage: make_branch_jenkins.sh <branch>}"
out="${2:-$ROOT/../release}"

cd "$ROOT"
git checkout -B "$branch"
git archive --format=tar.gz --output "$out-$branch.tar.gz" HEAD
echo "archive: $out-$branch.tar.gz"
