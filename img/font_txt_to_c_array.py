#!/usr/bin/env python3
"""Convert img/font.txt glyph data to a C typFNT_GB12 array.

The input format is expected to contain lines like:
    0x04,0x06,...,"孵",0

Usage:
    python img/font_txt_to_c_array.py img/font.txt -o img/font12.c
    python img/font_txt_to_c_array.py img/font.txt -o img/font12.c --header img/font12.h
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

GLYPH_RE = re.compile(r'^(?P<data>(?:0x[0-9A-Fa-f]{1,2}\s*,\s*)+)"(?P<char>.)"\s*,\s*(?P<index>\d+)\s*,?\s*$')
HEX_RE = re.compile(r"0x[0-9A-Fa-f]{1,2}")
C_IDENTIFIER_RE = re.compile(r"[^0-9A-Za-z_]")


def parse_font_txt(path: Path) -> list[tuple[str, list[str], int]]:
    """Parse glyph rows from font.txt and remove duplicate characters.

    If the same character appears multiple times, keep the first one and
    ignore the later duplicates.
    """
    glyphs: list[tuple[str, list[str], int]] = []
    seen_chars: set[str] = set()

    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = line.strip()
        if not line or not line.startswith("0x"):
            continue

        match = GLYPH_RE.match(line)
        if match is None:
            raise ValueError(f"line {line_no}: invalid glyph line: {line}")

        char = match.group("char")
        if char in seen_chars:
            print(f"warning: line {line_no}: duplicate character {char!r} skipped", file=sys.stderr)
            continue

        data = [value.upper().replace("X", "x") for value in HEX_RE.findall(match.group("data"))]
        if len(data) != 24:
            raise ValueError(f"line {line_no}: expected 24 bytes, got {len(data)}")

        glyphs.append((char, data, int(match.group("index"))))
        seen_chars.add(char)

    return glyphs


def make_include_guard(header_name: str) -> str:
    """Convert a header filename to a C include guard."""
    guard = C_IDENTIFIER_RE.sub("_", header_name.upper())
    guard = re.sub(r"_+", "_", guard).strip("_")
    if not guard:
        guard = "FONT_ARRAY_H"
    if guard[0].isdigit():
        guard = f"_{guard}"
    return guard


def make_macro_name(array_name: str) -> str:
    """Convert an array name to a C count macro name."""
    macro = C_IDENTIFIER_RE.sub("_", array_name.upper())
    macro = re.sub(r"_+", "_", macro).strip("_")
    if not macro:
        macro = "FONT_ARRAY"
    if macro[0].isdigit():
        macro = f"_{macro}"
    return f"{macro}_NUM"


def format_c_array(glyphs: list[tuple[str, list[str], int]], array_name: str, include_name: str) -> str:
    """Format glyphs as a typFNT_GB12 C array, one glyph per line."""
    lines = [f'#include "{include_name}"', "", f"const typFNT_GB12 {array_name}[] = {{"]

    for char, data, _index in glyphs:
        lines.append(f'    {{"{char}",{{{",".join(data)}}}}},')

    lines.append("};")
    return "\n".join(lines) + "\n"


def format_header(glyph_count: int, array_name: str, header_name: str, count_macro: str) -> str:
    """Format the header for the generated typFNT_GB12 array."""
    guard = make_include_guard(header_name)
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        '#include "Font.h"',
        "",
        f"#define {count_macro} {glyph_count}",
        f"extern const typFNT_GB12 {array_name}[{count_macro}];",
        "",
        f"#endif /* {guard} */",
    ]
    return "\n".join(lines) + "\n"


def default_header_path(output_path: Path | None, input_path: Path) -> Path:
    """Choose the default generated header path."""
    if output_path is not None:
        return output_path.with_suffix(".h")
    return input_path.with_suffix(".h")


def write_batch_files(
    glyphs: list[tuple[str, list[str], int]],
    output_path: Path,
    array_name: str,
    batch_size: int,
) -> list[Path]:
    """Write split font C/H files and return generated C paths."""
    generated_c_files: list[Path] = []
    stem = output_path.stem
    suffix = output_path.suffix or ".c"

    for batch_index, start in enumerate(range(0, len(glyphs), batch_size), start=1):
        batch_glyphs = glyphs[start : start + batch_size]
        batch_array_name = f"{array_name}_{batch_index}"
        batch_count_macro = make_macro_name(batch_array_name)
        batch_c_path = output_path.with_name(f"{stem}_{batch_index}{suffix}")
        batch_h_path = output_path.with_name(f"{stem}_{batch_index}.h")

        batch_c_output = format_c_array(batch_glyphs, batch_array_name, batch_h_path.name)
        batch_h_output = format_header(len(batch_glyphs), batch_array_name, batch_h_path.name, batch_count_macro)
        batch_c_path.write_text(batch_c_output, encoding="utf-8", newline="\n")
        batch_h_path.write_text(batch_h_output, encoding="utf-8", newline="\n")
        generated_c_files.append(batch_c_path)
        print(f"generated batch -> {batch_c_path}, {batch_h_path}", file=sys.stderr)

    return generated_c_files


def write_master_header(output_path: Path, array_name: str, glyph_count: int, batch_count: int) -> Path:
    """Write a master header with global font batch settings."""
    header_path = output_path.with_suffix(".h")
    guard = make_include_guard(header_path.name)
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#define WRITE_FONT_GB12_BATCH_NONE 0",
        "#define WRITE_FONT_GB12_BATCH_1 1",
        "#define WRITE_FONT_GB12_BATCH_2 2",
        "#define WRITE_FONT_GB12_BATCH_3 3",
        "#define WRITE_FONT_GB12_BATCH_4 4",
        "#define WRITE_FONT_GB12_BATCH_5 5",
        "",
        f"#define {make_macro_name(array_name)} {glyph_count}",
        f"#define {make_macro_name(array_name)}_BATCH_COUNT {batch_count}",
        "",
        "#endif",
    ]
    header_path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    print(f"generated master header -> {header_path}", file=sys.stderr)
    return header_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert font.txt to a typFNT_GB12 C array")
    parser.add_argument("input", type=Path, help="input font.txt path")
    parser.add_argument("-o", "--output", type=Path, help="output .c file path; default: print C array to stdout")
    parser.add_argument("--array-name", default="tfont12", help="C array name, default: tfont12")
    parser.add_argument("--header", type=Path, help="generated .h file path; default: output path with .h suffix")
    parser.add_argument("--no-header", action="store_true", help="do not generate the header file")
    parser.add_argument("--batch-size", type=int, help="split glyphs into multiple C/H files, each batch has this many glyphs")
    args = parser.parse_args()

    glyphs = parse_font_txt(args.input)

    if args.batch_size is not None:
        if args.output is None:
            raise ValueError("--batch-size requires -o/--output")
        if args.batch_size <= 0:
            raise ValueError("--batch-size must be greater than 0")
        write_batch_files(glyphs, args.output, args.array_name, args.batch_size)
        if not args.no_header:
            batch_count = (len(glyphs) + args.batch_size - 1) // args.batch_size
            write_master_header(args.output, args.array_name, len(glyphs), batch_count)
        return 0

    header_path = args.header or default_header_path(args.output, args.input)
    output = format_c_array(glyphs, args.array_name, header_path.name)

    if args.output is None:
        print(output, end="")
    else:
        args.output.write_text(output, encoding="utf-8", newline="\n")

    if not args.no_header:
        header_output = format_header(len(glyphs), args.array_name, header_path.name, make_macro_name(args.array_name))
        header_path.write_text(header_output, encoding="utf-8", newline="\n")
        print(f"generated header -> {header_path}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
