#!/usr/bin/env python3
"""Merge several output directories into one tree.

Later directories win when a file name collides; the original files are left
untouched so the script can be re-run safely.
"""

import argparse
import shutil
from pathlib import Path


def merge(indirs, outdir):
    out = Path(outdir)
    out.mkdir(parents=True, exist_ok=True)

    for src in indirs:
        src = Path(src)
        if not src.is_dir():
            raise SystemExit(f"not a directory: {src}")

        for path in src.rglob("*"):
            if not path.is_file():
                continue
            dest = out / path.relative_to(src)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--indirs", nargs="+", required=True)
    parser.add_argument("--outdir", required=True)
    args = parser.parse_args()
    merge(args.indirs, args.outdir)


if __name__ == "__main__":
    main()
