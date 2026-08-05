#!/usr/bin/env python3
"""Generate deterministic typed BigBaseV2 native wrappers from pinned NativeDB data."""

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
from typing import Any

SOURCE_REPOSITORY = "alloc8or/gta5-nativedb-data"
SOURCE_PATH = "natives.json"
SOURCE_BLOB_SHA = "9a76bd0e0f4e794b83fd03247b3fa6ac858251d2"
SOURCE_API_URL = (
    "https://api.github.com/repos/"
    f"{SOURCE_REPOSITORY}/git/blobs/{SOURCE_BLOB_SHA}"
)
DEFAULT_OUTPUT = Path("BigBaseV2/src/natives.generated.hpp")
IDENTIFIER_RE = re.compile(r"[^A-Za-z0-9_]")
HASH_RE = re.compile(r"0x[0-9A-Fa-f]{16}")
CPP_KEYWORDS = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
    "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
    "class", "compl", "concept", "const", "consteval", "constexpr", "constinit",
    "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
    "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit",
    "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline",
    "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
    "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register",
    "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static",
    "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local",
    "throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned",
    "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
}

TYPE_MAP = {
    "void": "Void",
    "any": "Any",
    "any*": "Any*",
    "bool": "BOOL",
    "BOOL": "BOOL",
    "int": "int",
    "int*": "int*",
    "float": "float",
    "float*": "float*",
    "hash": "Hash",
    "Hash": "Hash",
    "entity": "Entity",
    "Entity": "Entity",
    "ped": "Ped",
    "Ped": "Ped",
    "vehicle": "Vehicle",
    "Vehicle": "Vehicle",
    "object": "Object",
    "Object": "Object",
    "pickup": "Pickup",
    "Pickup": "Pickup",
    "player": "Player",
    "Player": "Player",
    "cam": "Cam",
    "Cam": "Cam",
    "blip": "Blip",
    "Blip": "Blip",
    "fireid": "FireId",
    "scrhandle": "ScrHandle",
    "vector3": "Vector3",
    "Vector3": "Vector3",
    "char*": "char*",
    "const char*": "const char*",
    "charPtr": "char*",
}


def git_blob_sha(data: bytes) -> str:
    return hashlib.sha1(f"blob {len(data)}\0".encode("ascii") + data).hexdigest()


def fetch_json_blob(attempts: int = 3) -> bytes:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "BigBaseV2-native-generator/2.0",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"

    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        request = urllib.request.Request(SOURCE_API_URL, headers=headers)
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                payload = json.load(response)
            if not isinstance(payload, dict):
                raise RuntimeError("GitHub returned a non-object JSON response.")
            break
        except (urllib.error.URLError, TimeoutError, json.JSONDecodeError) as error:
            last_error = error
            if attempt < attempts:
                time.sleep(2 ** (attempt - 1))
    else:
        raise RuntimeError(f"Unable to download pinned NativeDB data: {last_error}")

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
            f"Downloaded NativeDB has Git blob SHA {actual_sha}; expected {SOURCE_BLOB_SHA}."
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
            f"Local NativeDB has Git blob SHA {actual_sha}; expected {SOURCE_BLOB_SHA}. "
            "Use --allow-unpinned-input only for deliberate review work."
        )
    return data


def load_database(data: bytes) -> dict[str, Any]:
    try:
        decoded = data.decode("utf-8-sig")
        database = json.loads(decoded)
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise RuntimeError(f"NativeDB is not valid UTF-8 JSON: {error}") from error

    if not isinstance(database, dict) or not database:
        raise RuntimeError("NativeDB must be a non-empty JSON object.")
    return database


def cpp_type(raw: str, *, is_return: bool = False) -> str:
    normalized = raw.strip()
    if normalized in TYPE_MAP:
        return TYPE_MAP[normalized]

    lower = normalized.lower()
    if lower in TYPE_MAP:
        return TYPE_MAP[lower]

    if normalized.endswith("*"):
        base = cpp_type(normalized[:-1].strip(), is_return=is_return)
        return f"{base}*"

    return IDENTIFIER_RE.sub("_", normalized) or ("Any" if not is_return else "Any")


def identifier(value: str, fallback: str) -> str:
    result = IDENTIFIER_RE.sub("_", value or fallback)
    if not result:
        result = fallback
    if result[0].isdigit():
        result = f"_{result}"
    if result in CPP_KEYWORDS:
        result += "_"
    return result


def native_name(hash_value: str, data: dict[str, Any]) -> str:
    name = data.get("name") or f"_{hash_value}"
    return identifier(str(name), f"_{hash_value}")


def render_native(hash_value: str, data: dict[str, Any]) -> str:
    return_type = cpp_type(str(data.get("return_type", "Any")), is_return=True)
    params = data.get("params") or []
    if not isinstance(params, list):
        raise RuntimeError(f"Native {hash_value} has a non-list params field.")

    declarations: list[str] = []
    arguments: list[str] = []
    used_names: set[str] = set()
    for index, parameter in enumerate(params):
        if not isinstance(parameter, dict):
            raise RuntimeError(f"Native {hash_value} parameter {index} is not an object.")
        param_type = cpp_type(str(parameter.get("type", "Any")))
        param_name = identifier(str(parameter.get("name", f"p{index}")), f"p{index}")
        if param_name in used_names:
            param_name = f"{param_name}_{index}"
        used_names.add(param_name)
        declarations.append(f"{param_type} {param_name}")
        arguments.append(param_name)

    declaration_text = ", ".join(declarations)
    invoke_arguments = ", " + ", ".join(arguments) if arguments else ""
    name = native_name(hash_value, data)
    comment = str(data.get("comment") or "").replace("\n", " ").replace("*/", "* /").strip()
    suffix = f" // {comment[:140]}" if comment else ""

    if return_type == "Void":
        body = f"invoke<Void>({hash_value}{invoke_arguments});"
    else:
        body = f"return invoke<{return_type}>({hash_value}{invoke_arguments});"

    return f"\tNATIVE_DECL {return_type} {name}({declaration_text}) {{ {body} }}{suffix}"


def render(database: dict[str, Any], source_blob: str) -> tuple[str, int]:
    output = [
        "#pragma once",
        '#include "common.hpp"',
        '#include "gta/natives.hpp"',
        '#include "gta/vector.hpp"',
        '#include "invoker.hpp"',
        "",
        "#include <type_traits>",
        "#include <utility>",
        "",
        "// AUTO-GENERATED FILE. DO NOT EDIT BY HAND.",
        f"// Source: {SOURCE_REPOSITORY}/{SOURCE_PATH}",
        f"// source-blob: {source_blob}",
        "",
        "template <typename Ret, typename ...Args>",
        "FORCEINLINE Ret invoke(rage::scrNativeHash hash, Args&&... args)",
        "{",
        "\tusing namespace big;",
        "\tg_native_invoker.begin_call();",
        "\t(g_native_invoker.push_arg(std::forward<Args>(args)), ...);",
        "\tg_native_invoker.end_call(hash);",
        "\tif constexpr (!std::is_same_v<Ret, void> && !std::is_same_v<Ret, Void>)",
        "\t\treturn g_native_invoker.get_return_value<Ret>();",
        "}",
        "",
        "#define NATIVE_DECL __declspec(noinline) inline",
        "",
    ]

    native_count = 0
    for namespace in sorted(database, key=str):
        natives = database[namespace]
        if not isinstance(natives, dict):
            continue

        rendered: list[str] = []
        for hash_value in sorted(natives, key=str):
            data = natives[hash_value]
            normalized_hash = str(hash_value)
            if not isinstance(data, dict) or not HASH_RE.fullmatch(normalized_hash):
                continue
            rendered.append(
                render_native(normalized_hash.upper().replace("0X", "0x"), data)
            )

        if not rendered:
            continue

        output.append(f"namespace {identifier(str(namespace), 'UNKNOWN')}")
        output.append("{")
        output.extend(rendered)
        output.append("}")
        output.append("")
        native_count += len(rendered)

    if native_count == 0:
        raise RuntimeError("NativeDB contained no valid 64-bit native entries.")

    return "\n".join(output), native_count


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
    parser.add_argument("--input", type=Path, help="Local alloc8or natives.json")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--allow-unpinned-input", action="store_true")
    parser.add_argument("--minimum-count", type=int, default=1000)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.minimum_count <= 0:
        raise RuntimeError("--minimum-count must be greater than zero.")

    data = (
        read_source(args.input, args.allow_unpinned_input)
        if args.input is not None
        else fetch_json_blob()
    )
    source_blob = git_blob_sha(data)
    generated, native_count = render(load_database(data), source_blob)
    if native_count < args.minimum_count:
        raise RuntimeError(
            f"Generated only {native_count} natives; minimum is {args.minimum_count}."
        )

    changed = write_if_changed(args.output, generated)
    action = "Generated" if changed else "Validated"
    print(
        f"{action} {native_count} native wrappers at {args.output} "
        f"from blob {source_blob}."
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
