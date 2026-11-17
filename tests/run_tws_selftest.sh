#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT/tmp"
gcc -std=c11 -Wall -Wextra -I "$ROOT/include" \
    "$ROOT/tests/tws_selftest.c" \
    "$ROOT/bt/profiles/tws.c" \
    "$ROOT/bt/core/buf.c" \
    -o "$ROOT/tmp/tws_selftest"
"$ROOT/tmp/tws_selftest"
