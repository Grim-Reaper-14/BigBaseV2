#!/usr/bin/env python3
"""Generate typed BigBaseV2 native wrappers from alloc8or NativeDB JSON.

The generator accepts a local natives.json or downloads the current public
NativeDB. It writes a standalone C++17 header using BigBaseV2's invoke<T>()
helper. Crossmap generation is intentionally separate because NativeDB does not
contain build-specific registration hashes.
"""

from __future__ import annotations

import argparse
import json
import re
import urllib.request
from pathlib import Path
from typing import Any

DEFAULT_URL = "https://raw.githubusercontent.com/alloc8or/gta5-nativedb-data/master/natives.json"
IDENTIFIER_RE = re.compile(r"[^A-Za-z0-9_]")

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


def load_json(path: Path | None, url: str) -> dict[str, Any]:
    if path:
        return json.loads(path.read_text(encoding="utf-8"))
    with urllib.request.urlopen(url, timeout=30) as response:
        return json.loads(response.read().decode("utf-8"))


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

    # Unknown NativeDB types are intentionally preserved for review rather
    # than silently collapsing everything to Any.
    return IDENTIFIER_RE.sub("_", normalized) or ("Any" if not is_return else "Any")


def identifier(value: str, fallback: str) -> str:
    result = IDENTIFIER_RE.sub("_", value or fallback)
    if not result:
        result = fallback
    if result[0].isdigit():
        result = f"_{result}"
    if result in {"char", "class", "default", "delete", "float", "int", "new", "return", "this", "void"}:
        result += "_"
    return result


def native_name(hash_value: str, data: dict[str, Any]) -> str:
    name = data.get("name") or f"_{hash_value}"
    return identifier(str(name), f"_{hash_value}")


def render_native(hash_value: str, data: dict[str, Any]) -> str:
    return_type = cpp_type(str(data.get("return_type", "Any")), is_return=True)
    params = data.get("params") or []

    declarations: list[str] = []
    arguments: list[str] = []
    for index, parameter in enumerate(params):
        param_type = cpp_type(str(parameter.get("type", "Any")))
        param_name = identifier(str(parameter.get("name", f"p{index}")), f"p{index}")
        declarations.append(f"{param_type} {param_name}")
        arguments.append(param_name)

    declaration_text = ", ".join(declarations)
    invoke_arguments = ""
    if arguments:
        invoke_arguments = ", " + ", ".join(arguments)

    name = native_name(hash_value, data)
    comment = str(data.get("comment") or "").replace("\n", " ").replace("*/", "* /").strip()
    suffix = f" // {comment[:140]}" if comment else ""

    if return_type == "Void":
        body = f"invoke<Void>({hash_value}{invoke_arguments});"
    else:
        body = f"return invoke<{return_type}>({hash_value}{invoke_arguments});"

    return f"\tNATIVE_DECL {return_type} {name}({declaration_text}) {{ {body} }}{suffix}"


def render(database: dict[str, Any], source: str) -> str:
    output = [
        "#pragma once",
        "#include \"common.hpp\"",
        "#include \"gta/natives.hpp\"",
        "#include \"gta/vector.hpp\"",
        "#include \"invoker.hpp\"",
        "",
        "// AUTO-GENERATED FILE. DO NOT EDIT BY HAND.",
        f"// Source: {source}",
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

    for namespace, natives in database.items():
        if not isinstance(natives, dict):
            continue
        output.append(f"namespace {identifier(str(namespace), 'UNKNOWN')}")
        output.append("{")
        for hash_value, data in natives.items():
            if not isinstance(data, dict) or not re.fullmatch(r"0x[0-9A-Fa-f]{16}", str(hash_value)):
                continue
            output.append(render_native(str(hash_value).upper().replace("0X", "0x"), data))
        output.append("}")
        output.append("")

    return "\n".join(output)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, help="Local alloc8or natives.json")
    parser.add_argument("--url", default=DEFAULT_URL)
    parser.add_argument("--output", type=Path, default=Path("BigBaseV2/src/natives.generated.hpp"))
    args = parser.parse_args()

    database = load_json(args.input, args.url)
    source = str(args.input) if args.input else args.url
    generated = render(database, source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(generated, encoding="utf-8")
    print(f"Wrote {args.output} ({len(generated.splitlines())} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
