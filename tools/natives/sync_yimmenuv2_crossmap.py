#!/usr/bin/env python3
"""Generate the GTA V Enhanced native table from a pinned YimMenuV2 Git blob."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

SOURCE_REPOSITORY = "YimMenu/YimMenuV2"
SOURCE_PATH = "src/game/gta/invoker/Crossmap.hpp"
SOURCE_BLOB_SHA = "6f5c995a26612765ce29fe65a3668643a57aab27"
SOURCE_API_URL = (
    "https://api.github.com/repos/"
    f"{SOURCE_REPOSITORY}/git/blobs/{SOURCE_BLOB_SHA}"
)
EXPECTED_HASH_COUNT = 6720
DEFAULT_OUTPUT = Path("BigBaseV2/src/crossmap_enhanced.hpp")

DECLARATION_RE = re.compile(
    r"std::array\s*<\s*rage::scrNativeHash\s*,\s*(\d+)\s*>\s+"
    r"g_Crossmap\s*=\s*\{(.*?)\};",
    re.DOTALL,
)
HASH_RE = re.compile(r"0x[0-9A-Fa-f]{1,16}")


def git_blob_sha(data: bytes) -> str:
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()


def fetch_json(url: str, attempts: int = 3) -> dict[str, object]:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "BigBaseV2-enhanced-crossmap-sync/2.0",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"

    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        request = urllib.request.Request(url, headers=headers)
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                payload = json.load(response)
            if not isinstance(payload, dict):
                raise RuntimeError("GitHub returned a non-object JSON response.")
            return payload
        except (urllib.error.URLError, TimeoutError, json.JSONDecodeError) as error:
            last_error = error
            if attempt < attempts:
                time.sleep(2 ** (attempt - 1))

    raise RuntimeError(f"Unable to download pinned Enhanced native data: {last_error}")


def fetch_pinned_source() -> bytes:
    payload = fetch_json(SOURCE_API_URL)
    if payload.get("sha") != SOURCE_BLOB_SHA:
        raise RuntimeError(
            f"GitHub returned blob {payload.get('sha')!r}; expected {SOURCE_BLOB_SHA}."
        )
    if payload.get("encoding") != "base64":
        raise RuntimeError(f"Unsupported GitHub blob encoding: {payload.get('encoding')!r}")

    encoded = payload.get("content")
    if not isinstance(encoded, str):
        raise RuntimeError("GitHub blob response did not contain string content.")

    try:
        data = base64.b64decode("".join(encoded.split()), validate=True)
    except ValueError as error:
        raise RuntimeError("GitHub blob response contained invalid base64 data.") from error

    actual_sha = git_blob_sha(data)
    if actual_sha != SOURCE_BLOB_SHA:
        raise RuntimeError(
            f"Downloaded crossmap has Git blob SHA {actual_sha}; expected {SOURCE_BLOB_SHA}."
        )
    return data


def read_source(path: Path, allow_unpinned: bool) -> bytes:
    try:
        data = path.read_bytes()
    except OSError as error:
        raise RuntimeError(f"Unable to read {path}: {error}") from error

    actual_sha = git_blob_sha(data)
    if not allow_unpinned and actual_sha != SOURCE_BLOB_SHA:
        raise RuntimeError(
            f"Local input has Git blob SHA {actual_sha}; expected {SOURCE_BLOB_SHA}. "
            "Use --allow-unpinned-input only for deliberate review work."
        )
    return data


def parse_crossmap(data: bytes, expected_count: int) -> list[int]:
    try:
        source = data.decode("utf-8-sig")
    except UnicodeDecodeError as error:
        raise RuntimeError("YimMenuV2 Crossmap.hpp is not valid UTF-8.") from error

    declaration = DECLARATION_RE.search(source)
    if not declaration:
        raise RuntimeError("Could not locate the YimMenuV2 g_Crossmap declaration.")

    declared_count = int(declaration.group(1))
    hashes = [int(value, 16) for value in HASH_RE.findall(declaration.group(2))]

    if declared_count != expected_count:
        raise RuntimeError(
            f"YimMenuV2 declares {declared_count} hashes; expected {expected_count}."
        )
    if len(hashes) != declared_count:
        raise RuntimeError(
            f"YimMenuV2 declared {declared_count} hashes but {len(hashes)} were parsed."
        )
    if any(value <= 0 or value > 0xFFFFFFFFFFFFFFFF for value in hashes):
        raise RuntimeError("YimMenuV2 crossmap contains an invalid native hash.")
    if len(set(hashes)) != len(hashes):
        raise RuntimeError("YimMenuV2 crossmap contains duplicate original hashes.")

    return hashes


def render(hashes: list[int], source_blob: str) -> str:
    rows = []
    for offset in range(0, len(hashes), 4):
        chunk = hashes[offset : offset + 4]
        rows.append("\t\t" + ", ".join(f"0x{value:016X}" for value in chunk) + ",")

    return "\n".join(
        [
            "#pragma once",
            "",
            '#include "gta/natives.hpp"',
            "",
            "#include <array>",
            "",
            "// Generated by tools/natives/sync_yimmenuv2_crossmap.py. Do not edit by hand.",
            f"// Source: {SOURCE_REPOSITORY}/{SOURCE_PATH}",
            f"// source-blob: {source_blob}",
            f"// native-count: {len(hashes)}",
            "",
            "namespace big",
            "{",
            f'\tinline constexpr const char* g_enhanced_crossmap_source = "{SOURCE_REPOSITORY}/{SOURCE_PATH}";',
            f'\tinline constexpr const char* g_enhanced_crossmap_blob = "{source_blob}";',
            f"\tinline constexpr std::array<rage::scrNativeHash, {len(hashes)}> g_enhanced_native_hashes{{",
            *rows,
            "\t};",
            "",
            f"\tstatic_assert(g_enhanced_native_hashes.size() == {len(hashes)});",
            "}",
            "",
        ]
    )


def write_if_changed(path: Path, content: str) -> bool:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_file() and path.read_text(encoding="utf-8") == content:
        return False

    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(content, encoding="utf-8", newline="\n")
    temporary.replace(path)
    return True


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--input", type=Path, help="Use a reviewed local Crossmap.hpp")
    parser.add_argument("--expected-count", type=int, default=EXPECTED_HASH_COUNT)
    parser.add_argument("--allow-unpinned-input", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.expected_count <= 0:
        raise RuntimeError("--expected-count must be greater than zero.")

    source = (
        read_source(args.input, args.allow_unpinned_input)
        if args.input is not None
        else fetch_pinned_source()
    )
    hashes = parse_crossmap(source, args.expected_count)
    source_blob = git_blob_sha(source)
    changed = write_if_changed(args.output, render(hashes, source_blob))
    action = "Generated" if changed else "Validated"
    print(
        f"{action} {len(hashes)} Enhanced native hashes at {args.output} "
        f"from blob {source_blob}."
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
