#!/usr/bin/env python3
"""Generate the GTA V Enhanced vehicle catalog from a pinned Git blob.

The source is DurtyFree/gta-v-data-dumps VehicleList.ini. Each source row is
`model_name=unsigned_joaat`. The generator independently recomputes lowercase
JOAAT, rejects duplicate names/hashes, and emits a constexpr C++ table.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
import re
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path

SOURCE_REPOSITORY = "DurtyFree/gta-v-data-dumps"
SOURCE_PATH = "VehicleList.ini"
SOURCE_BLOB_SHA = "2ccc0b29cc3729609f03994f57078622bb0fe5d9"
SOURCE_API_URL = (
    "https://api.github.com/repos/"
    f"{SOURCE_REPOSITORY}/git/blobs/{SOURCE_BLOB_SHA}"
)
EXPECTED_MODEL_COUNT = 921
DEFAULT_OUTPUT = Path("BigBaseV2/src/menu/pages/vehicle_catalog_generated.hpp")
MODEL_NAME_PATTERN = re.compile(r"[a-z0-9_]+")


@dataclass(frozen=True, slots=True)
class VehicleModel:
    name: str
    model_hash: int


def joaat(value: str) -> int:
    result = 0
    for byte in value.lower().encode("ascii"):
        result = (result + byte) & 0xFFFFFFFF
        result = (result + (result << 10)) & 0xFFFFFFFF
        result ^= result >> 6
    result = (result + (result << 3)) & 0xFFFFFFFF
    result ^= result >> 11
    result = (result + (result << 15)) & 0xFFFFFFFF
    return result & 0xFFFFFFFF


def git_blob_sha(data: bytes) -> str:
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def fetch_pinned_source() -> bytes:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "BigBaseV2-vehicle-catalog-generator",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"

    request = urllib.request.Request(SOURCE_API_URL, headers=headers)
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            payload = json.load(response)
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError) as error:
        raise RuntimeError(f"Unable to download pinned vehicle data: {error}") from error

    if payload.get("sha") != SOURCE_BLOB_SHA:
        raise RuntimeError(
            f"GitHub returned blob {payload.get('sha')!r}; expected {SOURCE_BLOB_SHA}."
        )
    if payload.get("encoding") != "base64":
        raise RuntimeError(f"Unsupported GitHub blob encoding: {payload.get('encoding')!r}")

    try:
        data = base64.b64decode(payload["content"], validate=False)
    except (KeyError, ValueError) as error:
        raise RuntimeError("GitHub blob response did not contain valid base64 content.") from error

    actual_blob_sha = git_blob_sha(data)
    if actual_blob_sha != SOURCE_BLOB_SHA:
        raise RuntimeError(
            f"Downloaded vehicle data has Git blob SHA {actual_blob_sha}; "
            f"expected {SOURCE_BLOB_SHA}."
        )
    return data


def parse_models(data: bytes, expected_count: int) -> list[VehicleModel]:
    try:
        text = data.decode("utf-8-sig")
    except UnicodeDecodeError as error:
        raise RuntimeError("Vehicle data is not valid UTF-8.") from error

    models: list[VehicleModel] = []
    names: dict[str, int] = {}
    hashes: dict[int, str] = {}

    for line_number, raw_line in enumerate(text.splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith(("#", ";")):
            continue

        model_name, separator, hash_text = line.partition("=")
        if not separator:
            raise RuntimeError(f"Line {line_number} is missing '=': {raw_line!r}")

        normalized_name = model_name.strip().lower()
        if not MODEL_NAME_PATTERN.fullmatch(normalized_name):
            raise RuntimeError(
                f"Line {line_number} has an invalid model name: {model_name!r}"
            )

        try:
            source_hash = int(hash_text.strip(), 10)
        except ValueError as error:
            raise RuntimeError(
                f"Line {line_number} has an invalid unsigned hash: {hash_text!r}"
            ) from error

        if not 0 <= source_hash <= 0xFFFFFFFF:
            raise RuntimeError(
                f"Line {line_number} hash is outside uint32 range: {source_hash}"
            )

        computed_hash = joaat(normalized_name)
        if computed_hash != source_hash:
            raise RuntimeError(
                f"Line {line_number} JOAAT mismatch for {normalized_name!r}: "
                f"source=0x{source_hash:08X}, computed=0x{computed_hash:08X}"
            )

        previous_line = names.get(normalized_name)
        if previous_line is not None:
            raise RuntimeError(
                f"Duplicate model name {normalized_name!r} on lines "
                f"{previous_line} and {line_number}."
            )

        collision_name = hashes.get(source_hash)
        if collision_name is not None and collision_name != normalized_name:
            raise RuntimeError(
                f"JOAAT collision 0x{source_hash:08X}: "
                f"{collision_name!r} and {normalized_name!r}."
            )

        names[normalized_name] = line_number
        hashes[source_hash] = normalized_name
        models.append(VehicleModel(normalized_name, source_hash))

    if len(models) != expected_count:
        raise RuntimeError(
            f"Expected {expected_count} vehicle models, parsed {len(models)}."
        )

    models.sort(key=lambda model: model.name)
    return models


def render_header(models: list[VehicleModel]) -> str:
    lines = [
        "#pragma once",
        "",
        "// Generated by scripts/generate_vehicle_catalog.py. Do not edit by hand.",
        f"// Source: {SOURCE_REPOSITORY}/{SOURCE_PATH}",
        f"// source-blob: {SOURCE_BLOB_SHA}",
        f"// model-count: {len(models)}",
        "",
        f"inline constexpr std::array<vehicle_catalog_entry, {len(models)}> g_vehicle_catalog{{{{",
    ]

    lines.extend(
        f'\t{{0x{model.model_hash:08X}u, "{model.name}"}},' for model in models
    )
    lines.extend(
        [
            "}};",
            "",
            f"static_assert(g_vehicle_catalog.size() == {len(models)});",
            "static_assert(vehicle_catalog_hashes_valid(g_vehicle_catalog),",
            '\t"Generated vehicle model hashes must match lowercase JOAAT.");',
            "",
        ]
    )
    return "\n".join(lines)


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
    parser.add_argument("--source-file", type=Path)
    parser.add_argument("--expected-count", type=int, default=EXPECTED_MODEL_COUNT)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.source_file is not None:
        try:
            source_data = args.source_file.read_bytes()
        except OSError as error:
            raise RuntimeError(f"Unable to read {args.source_file}: {error}") from error
    else:
        source_data = fetch_pinned_source()

    models = parse_models(source_data, args.expected_count)
    changed = write_if_changed(args.output, render_header(models))
    action = "Generated" if changed else "Validated"
    print(
        f"{action} {len(models)} JOAAT-verified vehicle models at {args.output} "
        f"from blob {SOURCE_BLOB_SHA}."
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
