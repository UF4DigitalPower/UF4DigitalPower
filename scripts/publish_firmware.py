#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Publish a firmware release to the F4CP update server storage.

This script is intended for use in GitHub Actions or manual deployment.
It copies firmware binaries, computes SHA-256 digests, generates
manifest.json / changelog.md, and updates latest.json and index.json.

Usage example
-------------
python scripts/publish_firmware.py \\
    --kind power \\
    --version v0.1.4 \\
    --bin build/F4CP-Power-v0.1.4.bin \\
    --hex build/F4CP-Power-v0.1.4.hex \\
    --channel stable \\
    --set-latest \\
    --storage ./storage \\
    --notes "修复XXX" "优化YYY"
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sys
from datetime import date
from pathlib import Path

SEMVER = re.compile(r"^v(\d+)\.(\d+)\.(\d+)$")

# Per-kind defaults for device / hardware / pyocd target
KIND_DEFAULTS: dict[str, dict[str, str]] = {
    "power": {
        "display": "Power",
        "device": "STM32G474CBT6",
        "hardware": "F4CP-G474-Power",
        "burn_target": "stm32g474cbtx",
    },
    "upper": {
        "display": "Upper",
        "device": "STM32H750VBT6",
        "hardware": "F4CP-H750-Upper",
        "burn_target": "stm32h750vbtx",
    },
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def _load_json(path: Path) -> dict:
    if path.exists():
        return json.loads(path.read_text(encoding="utf-8"))
    return {}


def _dump_json(path: Path, data: dict) -> None:
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Publish a firmware version to the F4CP update server storage.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--kind", required=True, choices=list(KIND_DEFAULTS), help="Firmware kind: power | upper")
    parser.add_argument("--version", required=True, help="Semantic version, e.g. v0.1.3")
    parser.add_argument("--bin", dest="bin_path", help="Path to .bin firmware file")
    parser.add_argument("--hex", dest="hex_path", help="Path to .hex firmware file")
    parser.add_argument("--device", help="MCU device string (overrides default)")
    parser.add_argument("--hardware", help="Hardware board string (overrides default)")
    parser.add_argument("--channel", default="stable", choices=["stable", "beta"], help="Release channel")
    parser.add_argument("--min-client", default="0.4.0", dest="min_client", help="Minimum required client version")
    parser.add_argument("--notes", nargs="*", default=[], help="Release notes (one per entry)")
    parser.add_argument("--storage", default="./storage", help="Root storage directory (default: ./storage)")
    parser.add_argument("--set-latest", action="store_true", help="Update latest.json to this version")
    parser.add_argument("--date", dest="release_date", help="Release date YYYY-MM-DD (default: today)")
    args = parser.parse_args()

    # --- Validate version ---
    if not SEMVER.match(args.version):
        print(f"ERROR: Version '{args.version}' must match vX.Y.Z", file=sys.stderr)
        sys.exit(1)

    if not args.bin_path and not args.hex_path:
        print("ERROR: Provide at least one of --bin or --hex", file=sys.stderr)
        sys.exit(1)

    kind: str = args.kind
    meta = KIND_DEFAULTS[kind]
    version: str = args.version
    release_date: str = args.release_date or date.today().isoformat()
    storage = Path(args.storage).resolve()
    kind_dir = storage / "firmware" / kind
    version_dir = kind_dir / "versions" / version
    version_dir.mkdir(parents=True, exist_ok=True)
    print(f"\n=== Publishing {meta['display']} {version} ===")
    print(f"    Directory : {version_dir}")

    # --- Copy firmware files and compute SHA-256 ---
    files_meta: dict[str, dict] = {}
    for file_type, src_str in (("bin", args.bin_path), ("hex", args.hex_path)):
        if not src_str:
            continue
        src = Path(src_str).resolve()
        if not src.exists():
            print(f"ERROR: File not found: {src}", file=sys.stderr)
            sys.exit(1)

        dest_name = f"F4CP-{meta['display']}-{version}{src.suffix.lower()}"
        dest = version_dir / dest_name
        shutil.copy2(src, dest)
        digest = _sha256(dest)

        # Write SHA-256 sidecar
        sidecar = version_dir / f"{dest_name}.sha256"
        sidecar.write_text(f"{digest}  {dest_name}\n", encoding="utf-8")

        files_meta[file_type] = {
            "name": dest_name,
            "download_url": f"/api/v1/firmware/{kind}/versions/{version}/download?type={file_type}",
            "sha256": digest,
            "size": dest.stat().st_size,
        }
        print(f"    {file_type.upper()} : {dest_name}")
        print(f"    SHA256 : {digest}")

    # --- Write manifest.json ---
    device = args.device or meta["device"]
    hardware = args.hardware or meta["hardware"]
    manifest: dict = {
        "project": "F4CP",
        "kind": meta["display"],
        "version": version,
        "device": device,
        "hardware": hardware,
        "channel": args.channel,
        "date": release_date,
        "min_client": args.min_client,
        "files": files_meta,
        "burn": {
            "method": "pyocd",
            "target": meta["burn_target"],
            "reset_after_download": True,
        },
        "changelog_url": f"/api/v1/firmware/{kind}/versions/{version}/changelog",
        "notes": args.notes,
    }
    manifest_path = version_dir / "manifest.json"
    _dump_json(manifest_path, manifest)
    print(f"    manifest  : {manifest_path.name}")

    # --- Create changelog.md if missing ---
    changelog_path = version_dir / "changelog.md"
    if not changelog_path.exists():
        lines = [
            f"# {meta['display']} {version} Changelog\n",
            f"\n## {release_date}\n",
            "\n",
        ]
        for note in args.notes:
            lines.append(f"- {note}\n")
        changelog_path.write_text("".join(lines), encoding="utf-8")
        print(f"    changelog : {changelog_path.name} (created)")

    # --- Update index.json ---
    index_path = kind_dir / "index.json"
    index = _load_json(index_path) or {"project": "F4CP", "kind": meta["display"], "versions": []}
    # Remove any existing entry for the same version, then prepend new entry
    existing = [v for v in index.get("versions", []) if v.get("version") != version]
    new_entry: dict = {
        "version": version,
        "date": release_date,
        "channel": args.channel,
        "recommended": args.channel == "stable",
        "manifest_url": f"/api/v1/firmware/{kind}/versions/{version}",
    }
    index["versions"] = [new_entry] + existing
    _dump_json(index_path, index)
    print(f"    index     : updated ({len(index['versions'])} versions)")

    # --- Optionally update latest.json ---
    if args.set_latest:
        latest: dict = {
            "project": "F4CP",
            "kind": meta["display"],
            "latest": version,
            "manifest_url": f"/api/v1/firmware/{kind}/versions/{version}",
            "changelog_url": f"/api/v1/firmware/{kind}/versions/{version}/changelog",
        }
        latest_path = kind_dir / "latest.json"
        _dump_json(latest_path, latest)
        print(f"    latest    : updated to {version}")

    print(f"\nDone. {kind} {version} published to {version_dir}\n")


if __name__ == "__main__":
    main()
