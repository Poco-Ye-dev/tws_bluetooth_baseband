#!/usr/bin/env python3
"""Generate compile_commands.json from a dry-run build.

The build system does not depend on this script; it is a developer
convenience for clangd and other tooling.
"""

import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "compile_commands.json"


def main():
    result = subprocess.run(
        ["make", "-n", "app"], cwd=ROOT, capture_output=True, text=True
    )
    if result.returncode != 0:
        print(result.stderr, file=sys.stderr)
        return 1

    commands = []
    for line in result.stdout.splitlines():
        if " -c " not in line or "clang" not in line:
            continue
        match = re.search(r"-c\s+(\S+)", line)
        if not match:
            continue
        source = match.group(1)
        commands.append(
            {
                "directory": str(ROOT),
                "command": line.strip(),
                "file": source,
            }
        )

    OUT.write_text(json.dumps(commands, indent=2))
    print(f"wrote {OUT} ({len(commands)} entries)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
