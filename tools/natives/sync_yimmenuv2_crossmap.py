#!/usr/bin/env python3
"""Synchronize the GTA V Enhanced native hash table from YimMenuV2.

YimMenuV2 Enhanced does not use an old-hash/current-hash pair crossmap. Its
Crossmap.hpp is an ordered list of original native hashes. GTA Enhanced's
InitNativeTables routine rewrites that ordered list into native handlers.

This script downloads the official YimMenu/YimMenuV2 Enhanced table, validates
its declared size and values, and writes BigBaseV2/src/crossmap_enhanced.hpp.
"""

from __future__ import annotations

import argparse
import re
import urllib.request
from pathlib import Path

DEFAULT_URL = (
    "https://raw.githubusercontent.com/YimMenu/YimMenuV2/"
    "enhanced/src/game/gta/invoker/Crossmap.hpp"
)

DECLARATION_RE = re.compile(
    r"std::array\s*<\s*rage::scrNativeHash\s*,\s*(\d+)\s*>\s+g_Crossmap\s*=\s*\{(.*?)\};",
    re.DOTALL,
)
HASH_RE = re.compile(r"0x[0-9A-Fa-f]{1,16}")


def download_text(url: str) -> str:
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "BigBaseV2-native-sync/1.0"},
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read().decode("utf-8")


def parse_crossmap(source: str) -> list[int]:
    declaration = DECLARATION_RE.search(source)
    if not declaration:
        raise ValueError("Could not locate YimMenuV2 g_Crossmap declaration")

    declared_count = int(declaration.group(1))
    hashes = [int(value, 16) for value in HASH_RE.findall(declaration.group(2))]

    if len(hashes) != declared_count:
        raise ValueError(
            f"YimMenuV2 declared {declared_count} hashes but {len(hashes)} were parsed"
        )
    if not hashes:
        raise ValueError("YimMenuV2 crossmap is empty")
    if any(value == 0 for value in hashes):
        raise ValueError("YimMenuV2 crossmap contains a zero hash")
    if len(set(hashes)) != len(hashes):
        raise ValueError("YimMenuV2 crossmap contains duplicate original hashes")

    return hashes


def render(hashes: list[int], source_url: str, source_ref: str) -> str:
    rows: list[str] = []
    for offset in range(0, len(hashes), 4):
        chunk = hashes[offset : offset + 4]
        rows.append("\t\t" + ", ".join(f"0x{value:016X}" for value in chunk) + ",")

    return f'''#pragma once

#include "gta/natives.hpp"

#include <array>

namespace big
{{
\tinline constexpr const char* g_enhanced_crossmap_source = "{source_url}";
\tinline constexpr const char* g_enhanced_crossmap_ref = "{source_ref}";
\tinline constexpr std::array<rage::scrNativeHash, {len(hashes)}> g_enhanced_native_hashes{{
{chr(10).join(rows)}
\t}};
}}
'''


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--url", default=DEFAULT_URL)
    parser.add_argument("--source-ref", default="YimMenu/YimMenuV2:enhanced")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("BigBaseV2/src/crossmap_enhanced.hpp"),
    )
    parser.add_argument("--input", type=Path, help="Use a reviewed local Crossmap.hpp")
    args = parser.parse_args()

    source = (
        args.input.read_text(encoding="utf-8")
        if args.input
        else download_text(args.url)
    )
    hashes = parse_crossmap(source)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        render(hashes, args.url, args.source_ref),
        encoding="utf-8",
    )

    print(f"Validated {len(hashes)} Enhanced native hashes")
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
