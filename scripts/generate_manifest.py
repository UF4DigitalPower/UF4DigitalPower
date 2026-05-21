#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Regenerate manifest.json for an existing version directory.

Useful for re-computing SHA-256 after firmware files are replaced manually.

Usage
-----
python scripts/generate_manifest.py \\
    --kind power \\
    --version v0.1.3 \\
    --storage ./storage
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description="Regenerate manifest.json for an existing version directory.")
    parser.add_argument("--kind", required=True, help="Firmware kind: power | upper")
    parser.add_argument("--version", required=True, help="Version, e.g. v0.1.3")
    parser.add_argument("--storage", default="./storage", help="Root storage directory")
    args = parser.parse_args()

    storage = Path(args.storage).resolve()
    version_dir = storage / "firmware" / args.kind / "versions" / args.version
    manifest_path = version_dir / "manifest.json"

    if not manifest_path.exists():
        print(f"ERROR: manifest.json not found at {manifest_path}")
        return

    manifest: dict = json.loads(manifest_path.read_text(encoding="utf-8"))
    files = manifest.get("files", {})

    for file_type, file_info in files.items():
        name = file_info.get("name", "")
        fpath = version_dir / name
        if fpath.exists():
            digest = _sha256(fpath)
            size = fpath.stat().st_size
            file_info["sha256"] = digest
            file_info["size"] = size
            # Update sidecar
            (version_dir / f"{name}.sha256").write_text(f"{digest}  {name}\n", encoding="utf-8")
            print(f"  {file_type.upper()}: {name}  SHA256={digest}  size={size}")
        else:
            print(f"  WARNING: {name} not found, skipping.")

    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"manifest.json updated: {manifest_path}")


if __name__ == "__main__":
    main()
