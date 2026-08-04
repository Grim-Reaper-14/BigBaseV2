#!/usr/bin/env python3
"""Generate BigBaseV2/src/crossmap.hpp from build-specific mapping data.

Accepted input formats:
- JSON list: [["0xOLD", "0xNEW"], ...]
- JSON object: {"0xOLD": "0xNEW", ...}
- CSV/TXT: OLD_HASH,NEW_HASH (comments beginning with # are ignored)

This project targets GTA5_Enhanced.exe. The importer requires Enhanced edition
metadata and refuses Legacy/generic build labels.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

HASH_RE = re.compile(r"0x[0-9A-Fa-f]{1,16}")
NATIVE_HASH_RE = re.compile(r"invoke<[^>]+>\(\s*(0x[0-9A-Fa-f]{1,16})")
ENHANCED_BUILD_RE = re.compile(r"enhanced-[A-Za-z0-9._-]+", re.IGNORECASE)


@dataclass(frozen=True)
class Mapping:
    original: int
    current: int


def parse_hash(value: object) -> int:
    if isinstance(value, int):
        result = value
    elif isinstance(value, str) and HASH_RE.fullmatch(value.strip()):
        result = int(value, 16)
    else:
        raise ValueError(f"Invalid native hash: {value!r}")

    if result <= 0 or result > 0xFFFFFFFFFFFFFFFF:
        raise ValueError(f"Native hash out of range: {value!r}")
    return result


def read_json(path: Path) -> list[Mapping]:
    data = json.loads(path.read_text(encoding="utf-8"))
    items: Iterable[tuple[object, object]]
    if isinstance(data, dict):
        items = data.items()
    elif isinstance(data, list):
        items = data
    else:
        raise ValueError("JSON crossmap must be an object or a list of pairs")

    mappings: list[Mapping] = []
    for item in items:
        if not isinstance(item, (list, tuple)) or len(item) != 2:
            raise ValueError(f"Invalid mapping pair: {item!r}")
        mappings.append(Mapping(parse_hash(item[0]), parse_hash(item[1])))
    return mappings


def read_delimited(path: Path) -> list[Mapping]:
    mappings: list[Mapping] = []
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        for line_number, raw_line in enumerate(stream, start=1):
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            row = next(csv.reader([line]))
            if len(row) != 2:
                raise ValueError(f"Line {line_number}: expected two comma-separated hashes")
            mappings.append(Mapping(parse_hash(row[0].strip()), parse_hash(row[1].strip())))
    return mappings


def read_mappings(path: Path) -> list[Mapping]:
    return read_json(path) if path.suffix.lower() == ".json" else read_delimited(path)


def validate(mappings: list[Mapping]) -> list[Mapping]:
    if not mappings:
        raise ValueError("Crossmap contains no mappings")

    originals: dict[int, int] = {}
    for mapping in mappings:
        previous = originals.get(mapping.original)
        if previous is not None and previous != mapping.current:
            raise ValueError(
                f"Original hash 0x{mapping.original:016X} maps to both "
                f"0x{previous:016X} and 0x{mapping.current:016X}"
            )
        originals[mapping.original] = mapping.current

    return [Mapping(original, current) for original, current in sorted(originals.items())]


def validate_target(edition: str, executable: str, game_build: str) -> None:
    if edition.lower() != "enhanced":
        raise ValueError("This BigBaseV2 branch only accepts --edition enhanced")
    if executable.lower() != "gta5_enhanced.exe":
        raise ValueError("Expected --executable GTA5_Enhanced.exe")
    if not ENHANCED_BUILD_RE.fullmatch(game_build):
        raise ValueError("--game-build must begin with enhanced-, for example enhanced-3717")


def native_hashes(path: Path) -> set[int]:
    if not path.exists():
        return set()
    return {int(match, 16) for match in NATIVE_HASH_RE.findall(path.read_text(encoding="utf-8"))}


def render(mappings: list[Mapping], game_build: str, source: str, executable: str) -> str:
    rows = "\n".join(
        f"\t\t{{ 0x{item.original:016X}, 0x{item.current:016X} }}," for item in mappings
    )
    return f'''#pragma once
#include "gta/natives.hpp"

namespace big
{{
\tinline constexpr const char* g_crossmap_edition = "enhanced";
\tinline constexpr const char* g_crossmap_executable = "{executable}";
\tinline constexpr const char* g_crossmap_game_build = "{game_build}";
\tinline constexpr const char* g_crossmap_source = "{source}";
\tinline constexpr const rage::scrNativeMapping g_crossmap[]
\t{{
{rows}
\t}};
}}
'''


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path, help="Build-specific JSON/CSV mapping file")
    parser.add_argument("--output", type=Path, default=Path("BigBaseV2/src/crossmap.hpp"))
    parser.add_argument("--natives", type=Path, default=Path("BigBaseV2/src/natives.hpp"))
    parser.add_argument("--edition", required=True, choices=("enhanced",))
    parser.add_argument("--executable", default="GTA5_Enhanced.exe")
    parser.add_argument("--game-build", required=True, help="Example: enhanced-3717")
    parser.add_argument("--source", required=True, help="Mapping source/version description")
    parser.add_argument("--allow-missing", action="store_true")
    args = parser.parse_args()

    validate_target(args.edition, args.executable, args.game_build)
    mappings = validate(read_mappings(args.input))
    mapped_hashes = {mapping.original for mapping in mappings}
    wrappers = native_hashes(args.natives)
    missing = sorted(wrappers - mapped_hashes)

    print("Target edition: GTAV Enhanced")
    print(f"Target executable: {args.executable}")
    print(f"Mappings: {len(mappings)}")
    print(f"Wrapper hashes: {len(wrappers)}")
    print(f"Missing wrapper mappings: {len(missing)}")

    if missing and not args.allow_missing:
        preview = ", ".join(f"0x{value:016X}" for value in missing[:20])
        raise SystemExit(f"Crossmap does not cover natives.hpp. First missing hashes: {preview}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        render(mappings, args.game_build, args.source, args.executable),
        encoding="utf-8",
    )
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
